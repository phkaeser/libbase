/* ========================================================================= */
/**
 * @file subprocess.h
 * Methods to conveniently create sub-processes and handle I/O in a non-
 * blocking fashion.
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
#ifndef __LIBBASE_SUBPROCESS_H__
#define __LIBBASE_SUBPROCESS_H__

#include "test.h"

#include <stdbool.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/** Handle for a sub-process */
typedef struct _bs_subprocess_t bs_subprocess_t;

/** Descriptor for an environment variable. */
typedef struct {
    /** Name for the environment variable. */
    const char                *name_ptr;
    /** Value of the environment variable. */
    const char                *value_ptr;
} bs_subprocess_environment_variable_t;

/**
 * Creates a sub-process. Does not start it.
 *
 * @param file_ptr Name of the executable file. The subprocess module will
 *     invoke execvp() for execution, hence this will lookup the executable
 *     from PATHs as the shell would.
 * @param argv_ptr Pointer to a NULL-terminated list of args.
 *     Note: Unlike the convention to execvp(), this expects the real sequence
 *     of arguments. file_ptr will be inserted in position [0] when calling
 *     execvp().
 * @param env_vars_ptr Points to an array of @ref bs_subprocess_environment_variable_t
 *     describing additional environment variables to set for the sub-process.
 *     The array must be concluded with a sentinel element, where key_ptr is
 *     NULL. `env_vars_ptr` may be NULL to indicate no envinroment variables
 *     need to be set.
 *
 * @return A handle to the subprocess, to be cleaned by calling
 *     |bs_subprocess_destroy|.
 */
bs_subprocess_t *bs_subprocess_create(
    const char *file_ptr,
    const char *const *argv_ptr,
    const bs_subprocess_environment_variable_t *env_vars_ptr);

/**
 * Creates a sub-process. Does not start it.
 *
 * @param cmdline_ptr Commandline to execute. Will be tokenized. The leading
 *     tokens may be environment variables. The first token thereafter is
 *     is expected to identify an executable file; the rest of the tokens
 *     will be handled as arguments.
 *
 * @return A handle to the subprocess, to be cleaned by calling
 *     |bs_subprocess_destroy|.
 */
bs_subprocess_t *bs_subprocess_create_cmdline(
    const char *cmdline_ptr);

/**
 * Destroys a sub-process. Will stop (|bs_subprocess_stop|), if still running.
 *
 * @param subprocess_ptr
 */
void bs_subprocess_destroy(bs_subprocess_t *subprocess_ptr);

/**
 * Starts the sub-process.
 *
 * A started sub-process can be stopped by calling |bs_subprocess_stop| (and
 * then can be started again), or by |bs_subprocess_destroy|.
 *
 * @param subprocess_ptr
 *
 * @return Whether the call succeeded.
 */
bool bs_subprocess_start(bs_subprocess_t *subprocess_ptr);

/**
 * Stops the sub-process. Will SIGKILL the process, if not terminated
 * already.
 *
 * @param subprocess_ptr
 */
void bs_subprocess_stop(bs_subprocess_t *subprocess_ptr);

/**
 * Checks whether |subprocess_ptr| has terminated, and provide the information
 * in |*exit_status_ptr|, respectively |*signal_number_ptr|.
 *
 * @param subprocess_ptr
 * @param exit_status_ptr If the process terminated normally, this will hold
 *     the exit status of the program. It will be INT_MIN, if the process
 *     terminated abnormally.
 * @param signal_number_ptr If the process terminated abnormally, this will
 *     hold the number of the signal that caused the abnormal termination. A
 *     value of 0 indicates normal termination.
 *
 * @return Whether the process has terminated.
 */
bool bs_subprocess_terminated(bs_subprocess_t *subprocess_ptr,
                              int *exit_status_ptr,
                              int *signal_number_ptr);

/**
 * Retrieves the parent's file descriptors for stdin, stdout and stderr.
 *
 * @param subprocess_ptr
 * @param stdin_write_fd_ptr  May be NULL.
 * @param stdout_read_fd_ptr  May be NULL.
 * @param stderr_read_fd_ptr  May be NULL.
 */
void bs_subprocess_get_fds(bs_subprocess_t *subprocess_ptr,
                           int *stdin_write_fd_ptr,
                           int *stdout_read_fd_ptr,
                           int *stderr_read_fd_ptr);

/**
 * Returns the PID of the given subprocess. Will be 0 if not started or
 * terminated.
 *
 * @param subprocess_ptr
 *
 * @return PID, or 0.
 */
pid_t bs_subprocess_pid(bs_subprocess_t *subprocess_ptr);

/** Unit tests. */
extern const bs_test_case_t   bs_subprocess_test_cases[];

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif /* __LIBBASE_SUBPROCESS_H__ */
/* == End of subprocess.h ================================================== */
