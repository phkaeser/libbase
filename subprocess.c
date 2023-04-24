/* ========================================================================= */
/**
 * @file subprocess.c
 *
 * @copyright
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/// kill(2) is a POSIX extension, and we can't do IPC without it.
#define _POSIX_C_SOURCE 200809L

#include "subprocess.h"

#include <inttypes.h>
#include <limits.h>
#include <errno.h>
#include <poll.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "assert.h"
#include "log.h"
#include "log_wrappers.h"
#include "sock.h"

#undef _POSIX_C_SOURCE

/* == Definitions ========================================================== */

/** Local type, describes an environment variable. Non-const. */
typedef struct {
    /** Name of the variable. */
    char *name_ptr;
    /** Value of the variable. */
    char *value_ptr;
} _env_var_t;

/** Subprocess handle. */
struct _bs_subprocess_t {
    /** Name of the executable to execute in the subprocess. */
    char                      *file_ptr;
    /**
     * A NULL-terminated array of strings, holding the arguments.
     * Note: argv_ptr[0] points to file_ptr.
     */
    char                      **argv_ptr;
    /** Environment variables. */
    _env_var_t                 *env_vars_ptr;

    /** Will be non-zero when a child process is running. */
    pid_t                     pid;

    /** Will hold the exit status, if the process terminated normally. */
    int                       exit_status;
    /** Will hold the signal number, if the process was killed by a signal. */
    int                       signal_number;

    /** File descriptor for stdin (write) */
    int                       stdin_write;
    /** File descriptor for stdout (read) */
    int                       stdout_read;
    /** File descriptor for stderr (read) */
    int                       stderr_read;
    /** Points to the buffer for stdout */
    char                      *stdout_buf_ptr;
    /** Position within `stdout_buf_ptr`. */
    size_t                    stdout_pos;
    /** Size of `stdout_buf_ptr`. */
    size_t                    stdout_size;
    /** Points to the buffer for stderr */
    char                      *stderr_buf_ptr;
    /** Position within `stderr_buf_ptr`. */
    size_t                    stderr_pos;
    /** Size of `stderr_buf_ptr`. */
    size_t                    stderr_size;
};

/** Deterministic Finite Automation transition. */
typedef struct {
    /** Next state, for the automaton. */
    int8_t                     next_state;
    /** Outcome: Add char to token (1), ignore (0) or terminate (-1). */
    int8_t                     output;
} dfa_transition_t;

static bs_subprocess_t *_subprocess_create_argv(
    char **argv_ptr,
    _env_var_t *env_vars_ptr);
static int _waitpid_nointr(bs_subprocess_t *subprocess_ptr, bool wait);
static void _close_fd(int *fd_ptr);
static bool _create_pipe_fds(int *read_fd_ptr, int *write_fd_ptr);
static void _flush_stdout_stderr(bs_subprocess_t *subprocess_ptr);
static void _subprocess_child(
    bs_subprocess_t *subprocess_ptr,
    int stdin_read, int stdout_write, int stderr_write);

static char **_split_command(
    const char *cmd_ptr,
    _env_var_t **env_var_ptr_ptr);
static char *_split_next_token(const char *word_ptr, const char **next_ptr);
static void _free_argv_list(char **argv_ptr);
static void _free_env_var_list(_env_var_t *env_var_ptr);
static bool _populate_env_var(
    _env_var_t *env_var_ptr,
    const char *name_ptr,
    size_t name_len,
    const char *value_ptr);
static bool _is_variable_assignment(
    const char *word_ptr,
    _env_var_t *env_variable_ptr);

/* == Data ================================================================ */

/** character types, for the DFA table. */
static const int8_t            char_type_alpha = 0;
static const int8_t            char_type_blank = 1;
static const int8_t            char_type_escape = 2;
static const int8_t            char_type_double_quote = 3;
static const int8_t            char_type_end_of_string = 4;
static const int8_t            char_type_single_quote = 5;

/*
 * Transition table and parsing is modelled closely after libdockapp.
 *
 * From https://repo.or.cz/dockapps.git/blob_plain/HEAD:/libdockapp/COPYING:
 *
 * Copyright (c) 1999 Alfredo K. Kojima
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
static const dfa_transition_t state_transition_table[9][7] = {
    /* alpha     blank     escape   dblquote   NUL      sglquote */
    /* initial state, token has not started yet */
    { { 3, 1 }, { 0, 0 }, { 4, 0 }, { 1, 0 }, { 8, 0 }, { 6, 0 } },
    /* double-quoted */
    { { 1, 1 }, { 1, 1 }, { 2, 0 }, { 3, 0 }, { 5, 0 }, { 1, 1 } },
    /* escaped while double-quoted */
    { { 1, 1 }, { 1, 1 }, { 1, 1 }, { 1, 1 }, { 5, 0 }, { 1, 1 } },
    /* processing token (has started) */
    { { 3, 1 }, { 5, 0 }, { 4, 0 }, { 1, 0 }, { 5, 0 }, { 6, 0 } },
    /* escaped, part of token */
    { { 3, 1 }, { 3, 1 }, { 3, 1 }, { 3, 1 }, { 5, 0 }, { 3, 1 } },
    /* final state */
    { {-1,-1 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
    /* single-quoted */
    { { 6, 1 }, { 6, 1 }, { 7, 0 }, { 6, 1 }, { 5, 0 }, { 3, 0 } },
    /* escaped while single-quoted */
    { { 6, 1 }, { 6, 1 }, { 6, 1 }, { 6, 1 }, { 5, 0 }, { 6, 1 } },
    /* also final. */
    { {-1,-1 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } }, // final.
};

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bs_subprocess_t *bs_subprocess_create(
    const char *file_ptr,
    const char *const *argv_ptr,
    const bs_subprocess_environment_variable_t *env_vars_ptr)
{
    char **full_argv_ptr;

    size_t argv_size = 0;
    while (NULL != argv_ptr[argv_size]) ++argv_size;
    // Don't forget to alloc space for the sentinel NULL and file_ptr.
    full_argv_ptr = (char**)logged_calloc(argv_size + 2, sizeof(char*));
    if (NULL == full_argv_ptr) return NULL;

    full_argv_ptr[0] = logged_strdup(file_ptr);
    if (NULL == full_argv_ptr[0]) {
        _free_argv_list(full_argv_ptr);
        return NULL;
    }

    for (size_t i = 0; i < argv_size; ++i) {
        full_argv_ptr[i + 1] = logged_strdup(argv_ptr[i]);
        if (NULL == full_argv_ptr[i + 1]) {
            _free_argv_list(full_argv_ptr);
            return NULL;
        }
    }

    _env_var_t *env_var_ptr = NULL;
    if (NULL != env_vars_ptr) {
        size_t env_var_size = 0;
        while (NULL != env_vars_ptr[env_var_size].name_ptr) ++env_var_size;
        env_var_ptr = logged_calloc(env_var_size + 1, sizeof(_env_var_t));
        if (NULL == env_var_ptr) {
            _free_argv_list(full_argv_ptr);
            return NULL;
        }

        for (size_t i = 0; i < env_var_size; ++i) {
            if (!_populate_env_var(
                    env_var_ptr + i,
                    env_vars_ptr[i].name_ptr,
                    strlen(env_vars_ptr[i].name_ptr),
                    env_vars_ptr[i].value_ptr)) {
                _free_env_var_list(env_var_ptr);
                _free_argv_list(full_argv_ptr);
                return NULL;
            }
        }
    }


    return _subprocess_create_argv(full_argv_ptr, env_var_ptr);
}

/* ------------------------------------------------------------------------- */
bs_subprocess_t *bs_subprocess_create_cmdline(
    const char *cmdline_ptr)
{
    _env_var_t *env_var_ptr = NULL;
    char **argv_ptr = _split_command(cmdline_ptr, &env_var_ptr);
    if (NULL == argv_ptr) {
        bs_log(BS_ERROR, "Failed _split_command(%s)", cmdline_ptr);
        return NULL;
    }

    return _subprocess_create_argv(argv_ptr, env_var_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_subprocess_destroy(bs_subprocess_t *subprocess_ptr)
{
    if (0 != subprocess_ptr->pid) {
        bs_subprocess_stop(subprocess_ptr);
    }

    if (NULL != subprocess_ptr->stderr_buf_ptr) {
        free(subprocess_ptr->stderr_buf_ptr);
    }
    if (NULL != subprocess_ptr->stdout_buf_ptr) {
        free(subprocess_ptr->stdout_buf_ptr);
    }

    if (NULL != subprocess_ptr->argv_ptr) {
        _free_argv_list(subprocess_ptr->argv_ptr);
        // If argv_ptr was created, then file_ptr was argv_ptr[0], and thus it
        // was already free()-ed.
        subprocess_ptr->file_ptr = NULL;
    }

    if (NULL != subprocess_ptr->file_ptr) {
        free(subprocess_ptr->file_ptr);
    }

    free(subprocess_ptr);
}

/* ------------------------------------------------------------------------- */
bool bs_subprocess_start(bs_subprocess_t *subprocess_ptr)
{
    int stdin_read = -1, stdout_write = -1, stderr_write = -1;

    if (0 != subprocess_ptr->pid) {
        bs_log(BS_ERROR, "Already a running subprocess, pid %d",
               subprocess_ptr->pid);
        return false;
    }

    if (_create_pipe_fds(&stdin_read, &subprocess_ptr->stdin_write) &&
        _create_pipe_fds(&subprocess_ptr->stdout_read, &stdout_write) &&
        _create_pipe_fds(&subprocess_ptr->stderr_read, &stderr_write)) {

        subprocess_ptr->pid = fork();
        if (0 == subprocess_ptr->pid) {
            _subprocess_child(subprocess_ptr,
                              stdin_read, stdout_write, stderr_write);

            // Will never return, hence not reach this line.
            abort();
        }
    }

    // No matter what, we clean up descriptors for the child side.
    if (-1 != stdin_read) _close_fd(&stdin_read);
    if (-1 != stdout_write) _close_fd(&stdout_write);
    if (-1 != stderr_write) _close_fd(&stderr_write);

    subprocess_ptr->stdout_pos = 0;
    subprocess_ptr->stderr_pos = 0;

    // All worked -- go ahead.
    if (0 < subprocess_ptr->pid) {
        bs_sock_set_blocking(subprocess_ptr->stdout_read, false);
        bs_sock_set_blocking(subprocess_ptr->stderr_read, false);
        return true;
    }

    // There was a failure. Clean up descriptors on the parent side.
    if (-1 != subprocess_ptr->stdin_write) {
        _close_fd(&subprocess_ptr->stdin_write);
    }
    if (-1 != subprocess_ptr->stdout_read) {
        _close_fd(&subprocess_ptr->stdout_read);
    }
    if (-1 != subprocess_ptr->stderr_read) {
        _close_fd(&subprocess_ptr->stderr_read);
    }
    return false;
}

/* ------------------------------------------------------------------------- */
void bs_subprocess_stop(bs_subprocess_t *subprocess_ptr)
{
    if (0 != subprocess_ptr->pid) {
        // The child process is apparently still up. Send a KILL signal.
        if (0 != kill(subprocess_ptr->pid, SIGKILL)) {
            bs_log(BS_ERROR | BS_ERRNO, "Failed kill(%d, SIGKILL)",
                   subprocess_ptr->pid);
        }

        // Now, wait for the child process to terminate. Shouldn't last long.
        int rv = _waitpid_nointr(subprocess_ptr, true);
        if (0 == rv) {
            bs_log(BS_ERROR, "Process %d did unexpecedly NOT change status",
                   subprocess_ptr->pid);
        } else if (subprocess_ptr->pid != rv) {
            bs_log(BS_ERROR, "Unexpected return value %d for waitpid(%d, ...)",
                   rv, subprocess_ptr->pid);
        }
        subprocess_ptr->pid = 0;
    }

    // Here, the process is gone. Flush stdout & stderr, close sockets.
    _flush_stdout_stderr(subprocess_ptr);
    _close_fd(&subprocess_ptr->stdin_write);
    _close_fd(&subprocess_ptr->stdout_read);
    _close_fd(&subprocess_ptr->stderr_read);
}

/* ------------------------------------------------------------------------- */
bool bs_subprocess_terminated(bs_subprocess_t *subprocess_ptr,
                              int *exit_status_ptr,
                              int *signal_number_ptr)
{
    if (0 != subprocess_ptr->pid) {
        _flush_stdout_stderr(subprocess_ptr);
        if (0 >= _waitpid_nointr(subprocess_ptr, false)) {
            // An error or no signal. Assume it's still up.
            return false;
        }
        subprocess_ptr->pid = 0;
    }

    if (NULL != exit_status_ptr) {
        *exit_status_ptr = subprocess_ptr->exit_status;
    }
    if (NULL != signal_number_ptr) {
        *signal_number_ptr = subprocess_ptr->signal_number;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
void bs_subprocess_get_fds(bs_subprocess_t *subprocess_ptr,
                           int *stdin_write_fd_ptr,
                           int *stdout_read_fd_ptr,
                           int *stderr_read_fd_ptr)
{
    if (NULL != stdin_write_fd_ptr) {
        *stdin_write_fd_ptr = subprocess_ptr->stdin_write;
    }
    if (NULL != stdout_read_fd_ptr) {
        *stdout_read_fd_ptr = subprocess_ptr->stdout_read;
    }
    if (NULL != stderr_read_fd_ptr) {
        *stderr_read_fd_ptr = subprocess_ptr->stderr_read;
    }
}

/* ------------------------------------------------------------------------- */
pid_t bs_subprocess_pid(bs_subprocess_t *subprocess_ptr)
{
    return subprocess_ptr->pid;
}

/* ------------------------------------------------------------------------- */
const char *bs_subprocess_stdout(const bs_subprocess_t *subprocess_ptr)
{
    return subprocess_ptr->stdout_buf_ptr;
}

/* ------------------------------------------------------------------------- */
const char *bs_subprocess_stderr(const bs_subprocess_t *subprocess_ptr)
{
    return subprocess_ptr->stderr_buf_ptr;
}

/* == Local methods ======================================================== */

/* ------------------------------------------------------------------------- */
/** Creates the subprocess from |argv_ptr|. argv_ptr[0] is the executable. */
bs_subprocess_t *_subprocess_create_argv(
    char **argv_ptr,
    _env_var_t *env_vars_ptr)
{
    bs_subprocess_t *subprocess_ptr;

    subprocess_ptr = (bs_subprocess_t*)logged_calloc(
        1, sizeof(bs_subprocess_t));
    if (NULL == subprocess_ptr) return NULL;
    subprocess_ptr->file_ptr = argv_ptr[0];
    subprocess_ptr->argv_ptr = argv_ptr;
    subprocess_ptr->env_vars_ptr = env_vars_ptr;

    // A buffer for stdout and stderr. Keep a char for a terminating NUL.
    size_t buf_size = 4096;
    subprocess_ptr->stdout_buf_ptr = malloc(buf_size + 1);
    if (NULL == subprocess_ptr->stdout_buf_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed malloc(%zu)", buf_size + 1);
        bs_subprocess_destroy(subprocess_ptr);
        return NULL;
    }
    subprocess_ptr->stdout_size = buf_size;
    subprocess_ptr->stderr_buf_ptr = malloc(buf_size + 1);
    if (NULL == subprocess_ptr->stderr_buf_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed malloc(%zu)", buf_size + 1);
        bs_subprocess_destroy(subprocess_ptr);
        return NULL;
    }
    subprocess_ptr->stderr_size = buf_size;

    subprocess_ptr->stdin_write = -1;
    subprocess_ptr->stdout_read = -1;
    subprocess_ptr->stderr_read = -1;

    return subprocess_ptr;

}

/* ------------------------------------------------------------------------- */
/**
 * Runs waitpid() and updates |exit_status| and |signal_number| of
 * |subprocess_ptr|. Waits for the job's termination if |wait| is set.
 * Will re-try waitpid() when encountering EINTR, and logs all errors.
 *
 * @param subprocess_ptr
 * @param wait
 *
 * @return See waitpid().
 */
int _waitpid_nointr(bs_subprocess_t *subprocess_ptr, bool wait)
{
    int options = 0, status = 0, rv = 0;
    if (!wait) options |= WNOHANG;

    // Catch EINTR and keep calling.
    do {
        rv = waitpid(subprocess_ptr->pid, &status, options);
    } while (0 > rv && EINTR == errno);

    if (0 > rv) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed waitpid(%d, %p, 0x%x)",
               subprocess_ptr->pid, &status, options);
    } else if (0 < rv) {
        // Process did terminate. Update status accordingly.
        BS_ASSERT(rv == subprocess_ptr->pid);
        if (WIFEXITED(status)) {
            subprocess_ptr->exit_status = WEXITSTATUS(status);
            subprocess_ptr->signal_number = 0;
        } else if (WIFSIGNALED(status)) {
            subprocess_ptr->exit_status = INT_MIN;
            subprocess_ptr->signal_number = WTERMSIG(status);
        } else {
            bs_log(BS_FATAL, "Unhandled waitpid() status for %d: 0x%x",
                   subprocess_ptr->pid, status);
        }
    }
    return rv;
}

/* ------------------------------------------------------------------------- */
/** Helper: Close socket, log error, and invalidate |*fd_ptr|. */
void _close_fd(int *fd_ptr)
{
    if (0 != close(*fd_ptr)) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed close(%d)", *fd_ptr);
    }
    *fd_ptr = -1;
}

/* ------------------------------------------------------------------------- */
/** Helper: Reads all of stdout and stderr into the buffers. */
void _flush_stdout_stderr(bs_subprocess_t *subprocess_ptr)
{
    ssize_t read_bytes;

    read_bytes = read(
        subprocess_ptr->stdout_read,
        subprocess_ptr->stdout_buf_ptr + subprocess_ptr->stdout_pos,
        subprocess_ptr->stdout_size - subprocess_ptr->stdout_pos);
    if (0 > read_bytes && (EAGAIN != errno || EWOULDBLOCK != errno)) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed read(%d, %p, %zu)",
               subprocess_ptr->stdout_read,
               subprocess_ptr->stdout_buf_ptr + subprocess_ptr->stdout_pos,
               subprocess_ptr->stdout_size - subprocess_ptr->stdout_pos);
    } else if (0 <= read_bytes) {
        subprocess_ptr->stdout_pos += read_bytes;
        subprocess_ptr->stdout_buf_ptr[subprocess_ptr->stdout_pos] = '\0';
    }
    read_bytes = read(
        subprocess_ptr->stderr_read,
        subprocess_ptr->stderr_buf_ptr + subprocess_ptr->stderr_pos,
        subprocess_ptr->stderr_size - subprocess_ptr->stderr_pos);
    if (0 > read_bytes && (EAGAIN != errno || EWOULDBLOCK != errno)) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed read(%d, %p, %zu)",
               subprocess_ptr->stderr_read,
               subprocess_ptr->stderr_buf_ptr + subprocess_ptr->stderr_pos,
               subprocess_ptr->stderr_size - subprocess_ptr->stderr_pos);
    } else if (0 <= read_bytes) {
        subprocess_ptr->stderr_pos += read_bytes;
        subprocess_ptr->stderr_buf_ptr[subprocess_ptr->stderr_pos] = '\0';
    }
}

/* ------------------------------------------------------------------------- */
/** Helper: Creates a pipe, with errors logged & file descriptors stored */
bool _create_pipe_fds(int *read_fd_ptr, int *write_fd_ptr) {
    int fds[2];

    if (0 != pipe(fds)) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed pipe(%p)", fds);
        return false;
    }
    *read_fd_ptr = fds[0];
    *write_fd_ptr = fds[1];
    return true;
}

/* ------------------------------------------------------------------------- */
/** Wraps the child process: setup stdin, stdout, stderr and launches job. */
void _subprocess_child(
    bs_subprocess_t *subprocess_ptr,
    int stdin_read, int stdout_write, int stderr_write)
{
    // Close the parent's descriptors for stdin, stdout & stderr.
    _close_fd(&subprocess_ptr->stdin_write);
    _close_fd(&subprocess_ptr->stdout_read);
    _close_fd(&subprocess_ptr->stderr_read);

    // Setup stdin, stderr and stdout proper.
    if (0 != dup2(stdin_read, 0)) {
        bs_log(BS_FATAL | BS_ERRNO, "Failed dup2(%d, 0)", stdin_read);
    }
    if (0 != close(stdin_read)) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed close(%d)", stdin_read);
    }
    if (1 != dup2(stdout_write, 1)) {
        bs_log(BS_FATAL | BS_ERRNO, "Failed dup2(%d, 1)", stdout_write);
    }
    if (0 != close(stdout_write)) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed close(%d)", stdout_write);
    }
    if (2 != dup2(stderr_write, 2)) {
        bs_log(BS_FATAL | BS_ERRNO, "Failed dup2(%d, 2)", stderr_write);
    }
    if (0 != close(stderr_write)) {
        bs_log(BS_WARNING | BS_ERRNO, "Failed close(%d)", stderr_write);
    }

    for (const _env_var_t *var_ptr = subprocess_ptr->env_vars_ptr;
         NULL != var_ptr && NULL != var_ptr->name_ptr;
         ++var_ptr) {
        if (0 != setenv(var_ptr->name_ptr, var_ptr->value_ptr, 1)) {
            bs_log(BS_WARNING | BS_ERRNO, "Failed setenv(\"%s\", \"%s\")",
                   var_ptr->name_ptr, var_ptr->value_ptr);
            abort();
        }
    }

    if (0 > execvp(subprocess_ptr->file_ptr, subprocess_ptr->argv_ptr)) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed execvp(%s, %p)",
               subprocess_ptr->file_ptr, subprocess_ptr->argv_ptr);
    }
    // We only get here on error, and cannot do anything about.
    abort();
}

/* ------------------------------------------------------------------------- */
/** Splits the commandline into a NULL-terminated list of strings. */
char **_split_command(const char *cmd_ptr, _env_var_t **env_var_ptr_ptr)
{
    char *token_ptr;
    const char*line_ptr;
    char **argv_ptr;

    size_t argv_size = 0;
    argv_ptr = (char**)logged_calloc(1, (argv_size + 1) * sizeof(char*));
    if (NULL == argv_ptr) return NULL;

    _env_var_t *env_var_ptr;
    env_var_ptr = logged_calloc(1, sizeof(_env_var_t));
    size_t env_var_size = 0;

    line_ptr = cmd_ptr;
    do {
        token_ptr = _split_next_token(line_ptr, &line_ptr);
        if (NULL != token_ptr) {

            if (0 == argv_size &&
                _is_variable_assignment(token_ptr, &env_var_ptr[env_var_size])) {

                void *tmp_ptr = realloc(
                    env_var_ptr,
                    (env_var_size + 2) * sizeof(_env_var_t));
                if (NULL == tmp_ptr) {
                    bs_log(BS_ERROR | BS_ERRNO, "Failed realloc(%p, %zu)",
                           env_var_ptr,
                           (env_var_size + 2) * sizeof(_env_var_t));
                    _free_env_var_list(env_var_ptr);
                    _free_argv_list(argv_ptr);
                    return NULL;
                }
                env_var_ptr = tmp_ptr;
                ++env_var_size;
                memset(&env_var_ptr[env_var_size], 0, sizeof(_env_var_t));
            } else {
                argv_ptr[argv_size++] = token_ptr;
                void *tmp_ptr = realloc(argv_ptr, (argv_size + 1) * sizeof(char*));
                if (NULL == tmp_ptr) {
                    bs_log(BS_ERROR | BS_ERRNO, "Failed realloc(%p, %zu)",
                           argv_ptr, (argv_size + 1) * sizeof(char*));
                    _free_env_var_list(env_var_ptr);
                    _free_argv_list(argv_ptr);
                    return NULL;
                }
                argv_ptr = (char**)tmp_ptr;
                argv_ptr[argv_size] = NULL;
            }
        }
    } while (NULL != token_ptr && NULL != line_ptr);

    *env_var_ptr_ptr = env_var_ptr;
    return argv_ptr;
}

/* ------------------------------------------------------------------------- */
/** Returns a copy of the next token from |word_ptr|. Must be free()-ed. */
char *_split_next_token(const char *word_ptr, const char **next_ptr)
{
    char *token_ptr, *orig_token_ptr;
    int state, char_type;

    if (*word_ptr == '\0') return NULL;
    token_ptr = malloc(strlen(word_ptr) + 1);
    if (NULL == token_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed malloc(%zu)", strlen(word_ptr) + 1);
        return NULL;
    }
    orig_token_ptr = token_ptr;

    state = 0;
    *token_ptr = '\0';
    while (true) {
        switch (*word_ptr) {
        case '\0': char_type = char_type_end_of_string; break;
        case '\\': char_type = char_type_escape; break;
        case '"' : char_type = char_type_double_quote; break;
        case '\'': char_type = char_type_single_quote; break;
        case ' ' :
        case '\t': char_type = char_type_blank; break;
        default: char_type = char_type_alpha; break;
        }

        if (state_transition_table[state][char_type].output) {
            *token_ptr = *word_ptr;
            ++token_ptr;
            *token_ptr = '\0';
        }
        state = state_transition_table[state][char_type].next_state;
        word_ptr++;
        if (0 > state_transition_table[state][0].output) break;
    }

    // The token might be shorter than what we had reserved, update that.
    token_ptr = logged_strdup(orig_token_ptr);
    free(orig_token_ptr);

    if (char_type == char_type_end_of_string) {
        *next_ptr = NULL;
    } else {
        *next_ptr = word_ptr;
    }

    return token_ptr;
}

/* ------------------------------------------------------------------------- */
/** Frees up strings and the list of a NULL-terminated list of strings. */
void _free_argv_list(char **argv_ptr) {
    for (char **iter_ptr = argv_ptr; *iter_ptr != NULL; ++iter_ptr) {
        free(*iter_ptr);
    }
    free(argv_ptr);
}

/* ------------------------------------------------------------------------- */
/** Frees up the variable names and values, as well as the list itself. */
void _free_env_var_list(_env_var_t *env_var_ptr)
{
    for (_env_var_t *iter_ptr = env_var_ptr;
         iter_ptr->name_ptr != NULL;
         ++iter_ptr) {
        free(iter_ptr->name_ptr);
        free(iter_ptr->value_ptr);
    }
    free(env_var_ptr);
}

/* ------------------------------------------------------------------------- */
/** Populates the entry from name and value. */
bool _populate_env_var(_env_var_t *env_var_ptr,
                       const char *name_ptr,
                       size_t name_len,
                       const char *value_ptr)
{
    env_var_ptr->name_ptr = logged_malloc(name_len + 1);
    if (NULL == env_var_ptr->name_ptr) return false;

    memcpy(env_var_ptr->name_ptr, name_ptr, name_len);
    env_var_ptr->name_ptr[name_len] = '\0';

    env_var_ptr->value_ptr = logged_strdup(value_ptr);
    if (NULL == env_var_ptr->value_ptr) {
        free(env_var_ptr->name_ptr);
        env_var_ptr->name_ptr = NULL;
        return false;
    }
    return true;
}

/* ------------------------------------------------------------------------- */
/**
 * Returns if `word_ptr` is a variable assignmend and gives name and value.
 *
 * A variable assignment will start with the variable name, located at the
 * beginning of `word_ptr`. The variable name must start with a letter or
 * underscore, and may only contain only letters, numbers or underscore.
 * The variable name must be succeeded by a '=' sign. All that follows will be
 * considered the value.
 *
 * @param word_ptr            The token to analyse.
 * @param name_ptr_ptr        If `word_ptr` is a variable assignment, then
 *     *name_ptr_ptr will be set to point to a newly allocated copy of the
 *     variable name. Will only be set if the function returns true. If so,
 *     the allocated memory must be free()-ed.
 * @param value_ptr_ptr       If `word_ptr` is a variable assignent, then
 *     *value_ptr_ptr will be set to point to the variable's value. It points
 *     into `word_ptr`, and does not need to be free()-ed.
 *
 * @return true if `word_ptr` is a variable, and false if it's not a varaible
 *     or if an error occurred. *name_ptr_ptr and *value_ptr_ptr will be set
 *     only on success.
 */
bool _is_variable_assignment(
    const char *word_ptr,
    _env_var_t *env_variable_ptr)
{
    static const char *name_regexp_ptr = "^[a-zA-Z_][a-zA-Z_0-9]*=";
    regex_t regex;
    regmatch_t matches[1];
    int rv;

    if (0 != (rv = regcomp(&regex, name_regexp_ptr, REG_EXTENDED))) {
        bs_log(BS_ERROR, "Failed regcomp(%p, \"%s\", REG_EXTENDED): %d",
               &regex, name_regexp_ptr, rv);
        return false;
    }

    rv = regexec(&regex, word_ptr, 1, matches, 0);
    regfree(&regex);
    if (REG_NOMATCH == rv) return false;
    if (0 != rv) {
        bs_log(BS_ERROR, "Failed regexec(%p, \"%s\", 1, %p): %d",
               &regex, word_ptr, matches, rv);
        return false;
    }

    // A valid match must start at the beginning of the word, and it must at
    // least hold two chars: the variable name, and the '=' sign.
    if (matches[0].rm_so != 0 || matches[0].rm_eo < 2) return false;

    if (!_populate_env_var(env_variable_ptr,
                           word_ptr,
                           matches[0].rm_eo - matches[0].rm_so - 1,
                           word_ptr + matches[0].rm_eo)) {
        return false;
    }
    return true;
}

/* == Unit tests =========================================================== */
/** @cond TEST */

static void test_is_variable_assignment(bs_test_t *test_ptr);
static void test_split_command(bs_test_t *test_ptr);
static void test_failure(bs_test_t *test_ptr);
static void test_hang(bs_test_t *test_ptr);
static void test_nonexisting(bs_test_t *test_ptr);
static void test_sigpipe(bs_test_t *test_ptr);
static void test_success(bs_test_t *test_ptr);
static void test_success_cmdline(bs_test_t *test_ptr);
static void test_success_twice(bs_test_t *test_ptr);

const bs_test_case_t          bs_subprocess_test_cases[] = {
    { 1, "is_variable_assignment", test_is_variable_assignment },
    { 1, "split_command", test_split_command },
    { 1, "failure", test_failure },
    { 1, "hang", test_hang },
    { 1, "nonexisting", test_nonexisting },
    { 1, "sigpipe", test_sigpipe },
    { 1, "success", test_success },
    { 1, "success_cmdline", test_success_cmdline },
    { 1, "success_twice", test_success_twice },
    { 0, NULL, NULL }
};

const char                    *test_args[] = { "alpha", NULL };

/* ------------------------------------------------------------------------- */
/** Helper: Verify two NULL-terminated pointer string lists are equal. */
void test_verify_eq_arglist(
    bs_test_t *test_ptr, char **a1_ptr, char **a2_ptr)
{
    for (; *a1_ptr != NULL && *a2_ptr != NULL; a1_ptr++, a2_ptr++) {
        BS_TEST_VERIFY_STREQ(test_ptr, *a1_ptr, *a2_ptr);
    }
    BS_TEST_VERIFY_EQ(test_ptr, *a1_ptr, *a2_ptr);
}

/* ------------------------------------------------------------------------- */
/** Helper: Verify two env variable lists are equal. */
void test_verify_eq_envlist(
    bs_test_t *test_ptr,
    const bs_subprocess_environment_variable_t *exp_ptr,
    _env_var_t *var_ptr)
{
    for (;
         exp_ptr->name_ptr != NULL && var_ptr->name_ptr != NULL;
         exp_ptr++, var_ptr++) {
        BS_TEST_VERIFY_STREQ(test_ptr, exp_ptr->value_ptr, var_ptr->value_ptr);
        BS_TEST_VERIFY_STREQ(test_ptr, exp_ptr->value_ptr, var_ptr->value_ptr);
    }
    BS_TEST_VERIFY_EQ(test_ptr, exp_ptr->name_ptr, var_ptr->name_ptr);
}

/* ------------------------------------------------------------------------- */
void test_is_variable_assignment(bs_test_t *test_ptr)
{
    _env_var_t var;
    BS_TEST_VERIFY_TRUE(test_ptr, _is_variable_assignment("a=value", &var));
    BS_TEST_VERIFY_STREQ(test_ptr, var.name_ptr, "a");
    BS_TEST_VERIFY_STREQ(test_ptr, var.value_ptr, "value");
    free(var.name_ptr);
    free(var.value_ptr);

    BS_TEST_VERIFY_TRUE(test_ptr, _is_variable_assignment("a1=value", &var));
    BS_TEST_VERIFY_STREQ(test_ptr, var.name_ptr, "a1");
    BS_TEST_VERIFY_STREQ(test_ptr, var.value_ptr, "value");
    free(var.name_ptr);
    free(var.value_ptr);

    BS_TEST_VERIFY_TRUE(test_ptr, _is_variable_assignment("_=value", &var));
    BS_TEST_VERIFY_STREQ(test_ptr, var.name_ptr, "_");
    BS_TEST_VERIFY_STREQ(test_ptr, var.value_ptr, "value");
    free(var.name_ptr);
    free(var.value_ptr);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _is_variable_assignment("SILLY_2_LONG_VARIABLE_42=value", &var));
    BS_TEST_VERIFY_STREQ(test_ptr, var.name_ptr, "SILLY_2_LONG_VARIABLE_42");
    BS_TEST_VERIFY_STREQ(test_ptr, var.value_ptr, "value");
    free(var.name_ptr);
    free(var.value_ptr);

    BS_TEST_VERIFY_TRUE(
        test_ptr,
        _is_variable_assignment("a=value\" with more\"", &var));
    BS_TEST_VERIFY_STREQ(test_ptr, var.name_ptr, "a");
    BS_TEST_VERIFY_STREQ(test_ptr, var.value_ptr, "value\" with more\"");
    free(var.name_ptr);
    free(var.value_ptr);

    BS_TEST_VERIFY_TRUE(test_ptr, _is_variable_assignment("a= value", &var));
    BS_TEST_VERIFY_STREQ(test_ptr, var.name_ptr, "a");
    BS_TEST_VERIFY_STREQ(test_ptr, var.value_ptr, " value");
    free(var.name_ptr);
    free(var.value_ptr);

    BS_TEST_VERIFY_TRUE(test_ptr, _is_variable_assignment("a=", &var));
    BS_TEST_VERIFY_STREQ(test_ptr, var.name_ptr, "a");
    BS_TEST_VERIFY_STREQ(test_ptr, var.value_ptr, "");

    BS_TEST_VERIFY_FALSE(test_ptr, _is_variable_assignment("a", &var));
    BS_TEST_VERIFY_FALSE(test_ptr, _is_variable_assignment("1a=b", &var));
}


/* ------------------------------------------------------------------------- */
void test_split_command(bs_test_t *test_ptr)
{
    char **argv_ptr;
    _env_var_t *env_var_ptr;

    char *expected1[] = {"command", "arg1", "arg2", NULL };
    argv_ptr = _split_command("command arg1 arg2", &env_var_ptr);
    test_verify_eq_arglist(test_ptr, expected1, argv_ptr);
    _free_argv_list(argv_ptr);
    _free_env_var_list(env_var_ptr);

    char *expected2[] = {"command", "arg1 arg2", "arg3", NULL };
    argv_ptr = _split_command("command \"arg1 arg2\" arg3", &env_var_ptr);
    test_verify_eq_arglist(test_ptr, expected2, argv_ptr);
    _free_argv_list(argv_ptr);
    _free_env_var_list(env_var_ptr);

    char *expected3[] = {"command", "arg1 arg2", "arg3", NULL };
    argv_ptr = _split_command("command \'arg1 arg2\' arg3", &env_var_ptr);
    test_verify_eq_arglist(test_ptr, expected3, argv_ptr);
    _free_argv_list(argv_ptr);
    _free_env_var_list(env_var_ptr);

    char *expected4[] = {"command", "arg1 \'arg2", "arg3", NULL };
    argv_ptr = _split_command("command \"arg1 \'arg2\" arg3\'", &env_var_ptr);
    test_verify_eq_arglist(test_ptr, expected4, argv_ptr);
    _free_argv_list(argv_ptr);
    _free_env_var_list(env_var_ptr);

    char *expected5[] = {"command", "\"arg1", "arg2\"", "arg3", NULL };
    argv_ptr = _split_command("command \\\"arg1 arg2\\\" arg3", &env_var_ptr);
    test_verify_eq_arglist(test_ptr, expected5, argv_ptr);
    _free_argv_list(argv_ptr);
    _free_env_var_list(env_var_ptr);

    char *expected6[] = {"command", "arg1", NULL };
    const bs_subprocess_environment_variable_t expected_env6[] = {
        { "var1", "1" }, { "var2", "2" }, { NULL, NULL }
    };
    argv_ptr = _split_command("var1=1 var2=2 command arg1", &env_var_ptr);
    test_verify_eq_arglist(test_ptr, expected6, argv_ptr);
    test_verify_eq_envlist(test_ptr, expected_env6, env_var_ptr);
    _free_argv_list(argv_ptr);
    _free_env_var_list(env_var_ptr);
}

/* ------------------------------------------------------------------------- */
void test_failure(bs_test_t *test_ptr)
{
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        "./subprocess_test_failure", test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, 42, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_hang(bs_test_t *test_ptr)
{
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        "./subprocess_test_hang", test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    BS_TEST_VERIFY_FALSE(
        test_ptr,
        bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number));
    bs_subprocess_stop(sp_ptr);
    BS_TEST_VERIFY_TRUE(
        test_ptr,
        bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number));
    BS_TEST_VERIFY_EQ(test_ptr, INT_MIN, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, SIGKILL, signal_number);
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_nonexisting(bs_test_t *test_ptr)
{
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        "./subprocess_test_does_not_exist", test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, INT_MIN, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, SIGABRT, signal_number);

    const char *expected_stderr =
        "(ERROR) Failed execvp(./subprocess_test_does_not_exist";
    BS_TEST_VERIFY_EQ(
        test_ptr, 0, strncmp(
            expected_stderr,
            bs_subprocess_stderr(sp_ptr) + 24 + 17,
            strlen(expected_stderr)));
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_sigpipe(bs_test_t *test_ptr)
{
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        "./subprocess_test_sigpipe", test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, INT_MIN, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, SIGPIPE, signal_number);
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_success(bs_test_t *test_ptr)
{
    const bs_subprocess_environment_variable_t envs[] = {
        { "SUBPROCESS_ENV", "WORKS" },
        { NULL, NULL }
    };
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        "./subprocess_test_success", test_args, envs);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, 0, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);
    BS_TEST_VERIFY_STREQ(
        test_ptr, "test stdout: ./subprocess_test_success\nenv: WORKS\n",
        bs_subprocess_stdout(sp_ptr));
    BS_TEST_VERIFY_STREQ(
        test_ptr, "test stderr: alpha\n",
        bs_subprocess_stderr(sp_ptr));
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_success_cmdline(bs_test_t *test_ptr)
{
    bs_subprocess_t *sp_ptr = bs_subprocess_create_cmdline(
        "SUBPROCESS_ENV=CMD ./subprocess_test_success alpha");
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

    int exit_status, signal_number;
    while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
        // Don't busy-loop -- just wait a little.
        poll(NULL, 0, 10);
    }
    BS_TEST_VERIFY_EQ(test_ptr, 0, exit_status);
    BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);
    BS_TEST_VERIFY_STREQ(
        test_ptr, "test stdout: ./subprocess_test_success\nenv: CMD\n",
        bs_subprocess_stdout(sp_ptr));
    BS_TEST_VERIFY_STREQ(
        test_ptr, "test stderr: alpha\n",
        bs_subprocess_stderr(sp_ptr));
    bs_subprocess_destroy(sp_ptr);
}

/* ------------------------------------------------------------------------- */
void test_success_twice(bs_test_t *test_ptr)
{
    bs_subprocess_t *sp_ptr = bs_subprocess_create(
        "./subprocess_test_success", test_args, NULL);
    BS_TEST_VERIFY_NEQ(test_ptr, NULL, sp_ptr);

    for (int i = 0; i < 2; ++i) {
        // Verify that starting the process again works.
        BS_TEST_VERIFY_TRUE(test_ptr, bs_subprocess_start(sp_ptr));

        int exit_status, signal_number;
        while (!bs_subprocess_terminated(sp_ptr, &exit_status, &signal_number)) {
            // Don't busy-loop -- just wait a little.
            poll(NULL, 0, 10);
        }
        BS_TEST_VERIFY_EQ(test_ptr, 0, exit_status);
        BS_TEST_VERIFY_EQ(test_ptr, 0, signal_number);
        BS_TEST_VERIFY_STREQ(
            test_ptr, "test stdout: ./subprocess_test_success\nenv: (null)\n",
            bs_subprocess_stdout(sp_ptr));
        BS_TEST_VERIFY_STREQ(
            test_ptr, "test stderr: alpha\n",
            bs_subprocess_stderr(sp_ptr));
    }
    bs_subprocess_destroy(sp_ptr);
}

/** @endcond */
/* == End of subprocess.c ================================================== */
