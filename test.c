/* ========================================================================= */
/**
 * @file test.c
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

#include <curses.h>
#include <stdarg.h>
#include <fnmatch.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <term.h>
#include <threads.h>

#include "arg.h"
#include "assert.h"
#include "dllist.h"
#include "file.h"
#include "log_wrappers.h"
#include "test.h"

/** Information on current test case. */
struct _bs_test_t {
    /** Index of current test case (for information only). */
    int                       case_idx;
    /** Current test case descriptor. */
    const bs_test_case_t      *case_ptr;

    /** Test outcome: Failed? */
    bool                      failed;
    /** Test report summary (through bs_test_succeed, bs_test_fail) */
    char                      report[256];
};

/** Terminal codes for commands we're requiring. */
struct _bs_test_tcode {
    /** Buffer for storing all codes. */
    char                      all_codes[2048];
    /** Code for setting the foreground color. */
    const char                *code_set_foreground;
    /** Code for setting the background color. */
    const char                *code_set_background;
    /** Sets default colour-pair to the original one. */
    const char                *code_orig_pair;
    /** Code for entering bold mode. */
    const char                *code_enter_bold_mode;
    /** Code for clearing attributes. */
    const char                *code_exit_attribute_mode;
};

/** Attributes for setting terminal attributes. */
enum _bs_test_attributes {
    BS_TEST_ATTR_SKIP = 0x0201,
    BS_TEST_ATTR_FAIL = 0x020C,
    BS_TEST_ATTR_SUCCESS = 0x020A,
    BS_TEST_ATTR_RESET = -1,
};

/** Double-linked-list node, information about a failed test case. */
struct bs_test_fail_node {
    /** Node of the double-linked list. */
    bs_dllist_node_t          dlnode;
    /** Full name of the test (including test suite)*. */
    char                      *full_name_ptr;
};

/** Summary on a test run (either a case or a set). */
struct bs_test_report {
    /** Number of failed tests. */
    int                      failed;
    /** Number of succeeded tests. */
    int                      succeeded;
    /** Number of skipped tests. */
    int                      skipped;
    /** Total number of tests. */
    int                      total;
};

/* Other helpers */
static void bs_test_tcode_init(void);
static int bs_test_putc(int c);
static void bs_test_puts(const char *fmt_ptr, ...) __ARG_PRINTF__(1, 2);
static void bs_test_attr(int attr);

/* Set helpers. */
static void bs_test_set_report(const bs_test_set_t *set_ptr,
                               const struct bs_test_report *set_report_ptr);
static int bs_test_set(const bs_test_set_t *set_ptr,
                       const char *pattern_ptr,
                       bs_dllist_t *failed_tests_ptr);

/* Testcase helpers. */
static void bs_test_case_prepare(bs_test_t *test_ptr,
                                 const bs_test_set_t *set_ptr,
                                 int case_idx);
static void bs_test_case_report(const bs_test_t *test_ptr, bool enabled);

static struct bs_test_fail_node *bs_test_case_fail_node_create(
    char *full_name_ptr);
static void bs_test_case_fail_node_destroy(
    struct bs_test_fail_node *fnode_ptr);
static char *bs_test_case_create_full_name(
    const bs_test_set_t *set_ptr,
    const bs_test_case_t *case_ptr);

/* == Data ================================================================= */

/** Terminal codes. */
static struct _bs_test_tcode  bs_test_tcode;
/** Line separator between test cases. */
static const char             *bs_test_linesep_ptr =
    "----------------------------------------"
    "---------------------------------------";
/** Line separator between test sets. */
static const char             *bs_test_report_separator_ptr =
    "========================================"
    "=======================================";

static char                  *bs_test_filter_ptr = NULL;
static char                  *bs_test_data_dir_ptr = NULL;

static const bs_arg_t        bs_test_args[] = {
    BS_ARG_STRING(
        "test_filter",
        "Filter to apply for selecting tests. Uses fnmatch on the full name.",
        "*",
        &bs_test_filter_ptr),
    BS_ARG_STRING(
        "test_data_directory",
        "Directory to use for test data. Setting this flag takes precedence "
        "over the parameter specified through the `bs_test_param_t` arg to "
        "bs_test().",
        NULL,
        &bs_test_data_dir_ptr),
    BS_ARG_SENTINEL()
};

/* == Exported Functions =================================================== */

/* ------------------------------------------------------------------------- */
int bs_test(
    const bs_test_set_t *test_sets,
    int argc,
    const char **argv,
    const bs_test_param_t *param_ptr)
{
    struct bs_test_report     report;
    int                       i;
    bool                      run_set;
    bs_dllist_t               failed_tests;

    if (NULL == test_sets) return 0;
    if (!bs_arg_parse(bs_test_args, BS_ARG_MODE_NO_EXTRA, &argc, argv)) {
        bs_arg_print_usage(stderr, bs_test_args);
        return -1;
    }

    if (NULL == bs_test_data_dir_ptr &&
        NULL != param_ptr &&
        NULL != param_ptr->test_data_dir_ptr) {
        bs_test_data_dir_ptr = logged_strdup(param_ptr->test_data_dir_ptr);
        if (NULL == bs_test_data_dir_ptr) return -1;
    }

    bs_test_tcode_init();

    memset(&report, 0, sizeof(report));
    memset(&failed_tests, 0, sizeof(failed_tests));
    /** List of all failed tests. */
    while (NULL != test_sets->name_ptr) {

        /* If any args are given, check if the name matches. */
        run_set = 1 >= argc;
        for (i = 1; i < argc; i++) {
            if (0 == strcmp(argv[i], test_sets->name_ptr)) {
                run_set = true;;
            }
        }

        if (run_set && test_sets->enabled) {
            if (bs_test_set(test_sets, bs_test_filter_ptr, &failed_tests)) {
                report.failed++;
            } else {
                report.succeeded++;
            }
        } else {
            report.skipped++;
        }

        report.total++;
        test_sets++;
    }

    if (report.failed) {
        bs_test_attr(BS_TEST_ATTR_FAIL);
        bs_test_puts("FAILED: % 66d/% 3d\n",
                     report.failed, report.total);

        /* Print all failed tests. */
        struct bs_test_fail_node *fnode_ptr;
        while (NULL != (fnode_ptr = (struct bs_test_fail_node*)
                        bs_dllist_pop_front(&failed_tests))) {
            bs_test_puts(" %s\n", fnode_ptr->full_name_ptr);
            bs_test_case_fail_node_destroy(fnode_ptr);
        }
        bs_test_attr(BS_TEST_ATTR_RESET);
    } else if (report.succeeded) {
        bs_test_attr(BS_TEST_ATTR_SUCCESS);
        bs_test_puts("SUCCESS: % 65d/% 3d\n",
                     report.succeeded, report.total);
        bs_test_attr(BS_TEST_ATTR_RESET);
    }
    if (report.skipped) {
        bs_test_attr(BS_TEST_ATTR_SKIP);
        bs_test_puts("SKIPPED: % 65d/% 3d\n", report.skipped, report.total);
        bs_test_attr(BS_TEST_ATTR_RESET);
    }

    BS_ASSERT(0 == bs_dllist_size(&failed_tests));
    bs_arg_cleanup(bs_test_args);
    return report.failed;
}


/* ------------------------------------------------------------------------- */
void bs_test_succeed(bs_test_t *test, const char *fmt_ptr, ...)
{
    va_list                   ap;

    if ((!test->failed) && ('\0' == test->report[0])) {
        va_start(ap, fmt_ptr);
        vsnprintf(test->report, sizeof(test->report), fmt_ptr, ap);
        va_end(ap);
    }
}

/* ------------------------------------------------------------------------- */
void bs_test_fail_at(
    bs_test_t *test,
    const char *fname_ptr,
    int line,
    const char *fmt_ptr, ...)
{
    va_list                   ap;

    if (!test->failed) {
        test->failed = true;
        int pos = snprintf(&test->report[0], sizeof(test->report),
                           "%s(%d): ", fname_ptr, line);
        if (0 > pos || (size_t)pos >= sizeof(test->report)) return;

        va_start(ap, fmt_ptr);
        vsnprintf(&test->report[pos], sizeof(test->report) - pos, fmt_ptr, ap);
        va_end(ap);
    }
}

/* ------------------------------------------------------------------------- */
bool bs_test_failed(bs_test_t *test_ptr)
{
    return test_ptr->failed;
}

/* ------------------------------------------------------------------------- */
void bs_test_verify_streq_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    const int line,
    const char *a_ptr,
    const char *hash_a_ptr,
    const char *b_ptr,
    const char *hash_b_ptr)
{
    if (0 == strcmp(a_ptr, b_ptr)) {
        // TODO: report bs_test_succeed.
        return;
    }

    size_t pos = 0;
    while (a_ptr[pos] && b_ptr[pos] && a_ptr[pos] == b_ptr[pos]) {
        ++pos;
    }
    bs_test_fail_at(
        test_ptr, fname_ptr, line,
        "%s (\"%s\") not equal %s (\"%s\") at %zu (0x%02x != 0x%02x)",
        hash_a_ptr, a_ptr, hash_b_ptr, b_ptr, pos, a_ptr[pos], b_ptr[pos]);
}

/* ------------------------------------------------------------------------- */
void bs_test_verify_strmatch_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    const int line,
    const char *a_ptr,
    const char *hash_a_ptr,
    const char *regex_ptr)
{
    regex_t regex;
    int rv = regcomp(&regex, regex_ptr, REG_EXTENDED);
    if (0 != rv) {
        char err_buf[512];
        regerror(rv, &regex, err_buf, sizeof(err_buf));
        BS_TEST_FAIL(test_ptr, "Failed regcomp(\"%s\"): %s",
                     regex_ptr, err_buf);
        return;
    }

    regmatch_t matches[1];
    rv = regexec(&regex, a_ptr, 1, matches, 0);
    regfree(&regex);
    if (REG_NOMATCH == rv) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "%s (\"%s\") does not match \"%s\".",
            hash_a_ptr, a_ptr, regex_ptr);
    }
}

/* ------------------------------------------------------------------------- */
void bs_test_verify_memeq_at(
    bs_test_t *test_ptr,
    const char *fname_ptr,
    const int line,
    const void *a_ptr,
    const char *hash_a_ptr,
    const void *b_ptr,
    const char *hash_b_ptr,
    const size_t size)
{
    if (0 != memcmp(a_ptr, b_ptr, size)) {
        bs_test_fail_at(
            test_ptr, fname_ptr, line,
            "Buffer at %p (%s) != at %p (%s) for %zu",
            a_ptr, hash_a_ptr, b_ptr, hash_b_ptr, size);
    }
}

/* ------------------------------------------------------------------------- */
const char *bs_test_resolve_path(const char *fname_ptr)
{
    // POSIX doesn't really say whether the terminating NUL would fit.
    // Add a spare byte for that.
    static thread_local char resolved_path[PATH_MAX + 1];
    return bs_file_join_realpath(
        bs_test_data_dir_ptr, fname_ptr, resolved_path);
}
/* == Static (Local) Functions ============================================= */

/* ------------------------------------------------------------------------- */
/**
 * Initializes terminal escape codes.
 *
 * @param tcode_ptr
 */
void bs_test_tcode_init(void)
{
    char                      *code_buf_ptr;
    char                      tc_buf[4096];

    char *term_ptr = getenv("TERM");
    if (NULL == term_ptr) {
        // Fallback: revert to ANSI, a likely common denominator.
        term_ptr = "ansi";
    } else if (0 == strcmp(term_ptr, "xterm-color") ||
               0 == strcmp(term_ptr, "xterm-256color")) {
        // Hack: Some Unixes use xterm-color, but fail to provide a termcap.
        term_ptr = "xterm";
    }
    BS_ASSERT(0 < tgetent(tc_buf, term_ptr));

    memset(&bs_test_tcode, 0, sizeof(struct _bs_test_tcode));
    code_buf_ptr = bs_test_tcode.all_codes;

    // https://pubs.opengroup.org/onlinepubs/007908799/xcurses/terminfo.html.
    bs_test_tcode.code_set_background = tgetstr((char*)"Sb", &code_buf_ptr);
    bs_test_tcode.code_set_foreground = tgetstr((char*)"Sf", &code_buf_ptr);
    bs_test_tcode.code_orig_pair = tgetstr((char*)"op", &code_buf_ptr);

    bs_test_tcode.code_enter_bold_mode =
        tgetstr((char*)"md", &code_buf_ptr);
    bs_test_tcode.code_exit_attribute_mode =
        tgetstr((char*)"me", &code_buf_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Print a character to terminal. Wrapper for terminal-code functions.
 *
 * @param c
 *
 * @return 0 on success.
 */
int bs_test_putc(int c)
{
    char x = c;
    /* return 0 on success */
    return 1 - write(1, &x, 1);
}

/* ------------------------------------------------------------------------- */
/**
 * Print to terminal.
 *
 * @param fmt_ptr             Format string, as vsnprintf takes.
 * @param ...                 Additional arguments.
 */
void bs_test_puts(const char *fmt_ptr, ...)
{
    char                      line_buf[1024];
    int                       len;
    va_list                   ap;

    va_start(ap, fmt_ptr);
    len = vsnprintf(line_buf, sizeof(line_buf), fmt_ptr, ap);
    if ((size_t)len > sizeof(line_buf)) len = sizeof(line_buf);
    va_end(ap);

    int written_len = 0;
    for (written_len = 0;
         written_len < len;
         written_len = write(1, &line_buf[written_len], len)) ;
}

/* ------------------------------------------------------------------------- */
/**
 * Set terminal attributes.
 *
 * @param attr:               - bit 0..2 : 7-bit foreground
 *                            - bit 3 : toggle bold mode
 *                            - bit 4..6 : 7-bit background
 *                            - bit 7 : ignored.
 *                            - bit 8 : do not set foreground
 *                            - bit 9 : do not set background
 *                            - a value of -1 resets attributes
 */
void bs_test_attr(int attr)
{
    if (-1 == attr) {
        tputs(bs_test_tcode.code_orig_pair, 0, bs_test_putc);
        tputs(bs_test_tcode.code_exit_attribute_mode, 0, bs_test_putc);
        return;
    }

    if (0x100 != (0x100 & attr)) {
        tputs(tparm((char*)bs_test_tcode.code_set_foreground,
                    attr & 0x07, 0, 0, 0, 0, 0, 0, 0, 0),
              0, bs_test_putc);
    }
    if (attr & 0x08) {
        tputs(bs_test_tcode.code_enter_bold_mode, 0, bs_test_putc);
    }

    if (0x0200 != (0x0200 & attr)) {
        tputs(tparm((char*)bs_test_tcode.code_set_background,
                    (attr & 0x70) >> 4, 0, 0, 0, 0, 0, 0, 0, 0),
              0, bs_test_putc);
    }
}

/* ------------------------------------------------------------------------- */
/**
 * Print summary of test set.
 *
 * @param test_ptr            Test state.
 * @param set_report_ptr      Report.
 */
void bs_test_set_report(const bs_test_set_t *set_ptr,
                        const struct bs_test_report *set_report_ptr)
{
    bs_test_puts("%s\n %-65.65s ",
                 bs_test_report_separator_ptr,
                 set_ptr->name_ptr);
    bs_test_puts("OK");
    bs_test_puts(": % 3d/% 3d\n%s\n",
                 set_report_ptr->succeeded,
                 set_report_ptr->total,
                 bs_test_report_separator_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Runs the test cases of the specified test set.
 *
 * @param test_ptr            Test state.
 * @param pattern_ptr         Which tests to run, using fnmatch.
 * @param set_ptr             Set to run.
 *
 * @return 0, if all tests of the test reported success, or the number of
 *     failed test cases.
 */
int bs_test_set(const bs_test_set_t *set_ptr,
                const char *pattern_ptr,
                bs_dllist_t *failed_tests_ptr)
{
    const bs_test_case_t      *case_ptr;
    struct bs_test_fail_node  *fnode_ptr;
    struct bs_test_report     set_report;
    int                       case_idx;
    bs_test_t                 test;

    bs_test_puts("%s\n Set: %-73.73s\n",
                 bs_test_report_separator_ptr,
                 set_ptr->name_ptr);
    memset(&set_report, 0, sizeof(set_report));

    /* step through descriptors and run each test */
    for (case_idx = 0;
         NULL != set_ptr->case_ptr[case_idx].name_ptr;
         set_report.total++, case_idx++) {

        case_ptr = set_ptr->case_ptr + case_idx;
        bs_test_puts("%s\n", bs_test_linesep_ptr);
        bs_test_case_prepare(&test, set_ptr, case_idx);
        bool enabled = case_ptr->enabled;

        char *full_name_ptr = bs_test_case_create_full_name(set_ptr, case_ptr);
        if (NULL == full_name_ptr) {
            test.failed = true;
        } else {
            if (fnmatch(pattern_ptr, full_name_ptr, 0)) {
                enabled = false;
            }
            if (enabled) {
                case_ptr->test_fn(&test);
            }
        }

        if (!enabled) {
            set_report.skipped++;
        } else if (!test.failed) {
            set_report.succeeded++;
        } else {
            set_report.failed++;
            fnode_ptr = bs_test_case_fail_node_create(full_name_ptr);
            if (fnode_ptr) {
                bs_dllist_push_back(failed_tests_ptr, &fnode_ptr->dlnode);
                full_name_ptr = NULL;
            }
        }

        if (NULL != full_name_ptr) free(full_name_ptr);

        bs_test_case_report(&test, enabled);
    }

    bs_test_set_report(set_ptr, &set_report);
    return set_report.failed;
}

/* ------------------------------------------------------------------------- */
/**
 * Prepares test_ptr for this test case.
 *
 * @param test_ptr            Test state.
 * @param set_ptr             Test set.
 * @param case_idx            Index of the test case within the set.
 */
void bs_test_case_prepare(bs_test_t *test_ptr,
                          const bs_test_set_t *set_ptr,
                          int case_idx)
{
    test_ptr->case_idx = case_idx;
    test_ptr->case_ptr = set_ptr->case_ptr + case_idx;
    test_ptr->failed = false;
    memset(test_ptr->report, 0, sizeof(test_ptr->report));
}

/* ------------------------------------------------------------------------- */
/**
 * Reports the outcome of this particular testcase.
 *
 * @param test_ptr
 */
void bs_test_case_report(const bs_test_t *test_ptr, bool enabled)
{
    const char                *outcome_ptr;
    int                       attr;

    if (!enabled) {
        attr = BS_TEST_ATTR_SKIP;
        outcome_ptr = "SKIP";
    } else if (test_ptr->failed) {
        attr = BS_TEST_ATTR_FAIL;
        outcome_ptr = "FAIL";
    } else {
        attr = BS_TEST_ATTR_SUCCESS;
        outcome_ptr = "SUCCESS";
    }

    bs_test_puts(" % 3d: %-63.63s ",
                 test_ptr->case_idx,
                 test_ptr->case_ptr->name_ptr);
    bs_test_attr(attr);
    bs_test_puts("%8.8s", outcome_ptr);
    bs_test_attr(BS_TEST_ATTR_RESET);
    bs_test_puts("\n");

    if ((enabled) && ('\0' != test_ptr->report[0])) {
        bs_test_puts("  %s\n", test_ptr->report);
    }
    bs_test_attr(BS_TEST_ATTR_RESET);
}

/* ------------------------------------------------------------------------- */
/**
 * Create a double-linked-list node with information on a failed test.
 *
 * @param full_name_ptr
 *
 * @return Pointer to the node or NULL on error.
 */
struct bs_test_fail_node *bs_test_case_fail_node_create(char *full_name_ptr)
{
    struct bs_test_fail_node  *fnode_ptr;

    fnode_ptr = calloc(1, sizeof(struct bs_test_fail_node));
    if (NULL == fnode_ptr) return NULL;

    fnode_ptr->full_name_ptr = full_name_ptr;
    return fnode_ptr;
}

/* ------------------------------------------------------------------------- */
/**
 * Destroy a double-linked-list node with failed-test information.
 *
 * @see bs_test_case_fail_node_create
 *
 * @param fnode_ptr
 */
void bs_test_case_fail_node_destroy(struct bs_test_fail_node *fnode_ptr)
{
    if (NULL != fnode_ptr->full_name_ptr) {
        free(fnode_ptr->full_name_ptr);
    }
    free(fnode_ptr);
}

/* ------------------------------------------------------------------------- */
/**
 * Creates the fully-qualified test name from the set and the case.
 *
 * @param set_ptr
 * @param case_ptr
 *
 * @return Pointer to the full name, or NULL on error.
 */
char *bs_test_case_create_full_name(
    const bs_test_set_t *set_ptr,
    const bs_test_case_t *case_ptr)
{
    char *full_name_ptr = logged_malloc(
        strlen(set_ptr->name_ptr) + strlen(case_ptr->name_ptr) + 2);
    if (NULL == full_name_ptr) return NULL;

    strcpy(full_name_ptr, set_ptr->name_ptr);
    full_name_ptr[strlen(set_ptr->name_ptr)] = '.';
    strcpy(full_name_ptr + strlen(set_ptr->name_ptr) + 1, case_ptr->name_ptr);
    return full_name_ptr;
}

/* == Unit self-tests ====================================================== */
/** @cond TEST */

static void bs_test_test_report(bs_test_t *test_ptr);
static void bs_test_eq_neq_tests(bs_test_t *test_ptr);

const bs_test_case_t bs_test_test_cases[] = {
    { 1, "succeed/fail reporting", bs_test_test_report },
    { 1, "eq/neq tests", bs_test_eq_neq_tests },
    { 0, NULL, NULL }  /* sentinel. */
};

void bs_test_test_fail(bs_test_t *test_ptr)
{
    BS_TEST_FAIL(test_ptr, "fail");
}

void bs_test_test_succeed(bs_test_t *test_ptr)
{
    bs_test_succeed(test_ptr, "success");
}

void bs_test_test_succeed_fail(bs_test_t *test_ptr)
{
    bs_test_succeed(test_ptr, "success");
    BS_TEST_FAIL(test_ptr, "fail");
}

void bs_test_test_fail_succeed(bs_test_t *test_ptr)
{
    BS_TEST_FAIL(test_ptr, "fail");
    bs_test_succeed(test_ptr, "success");
}

/**
 * Tests bs_test_fail and bs_test_succeed.
 */
void bs_test_test_report(bs_test_t *test_ptr)
{
    bs_test_t                 sub_test;

    memset(&sub_test, 0, sizeof(bs_test_t));
    bs_test_test_fail(&sub_test);
    BS_TEST_VERIFY_TRUE(test_ptr, sub_test.failed);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_test_failed(&sub_test));
    memset(&sub_test, 0, sizeof(bs_test_t));
    bs_test_test_succeed(&sub_test);
    BS_TEST_VERIFY_FALSE(test_ptr, sub_test.failed);
    BS_TEST_VERIFY_FALSE(test_ptr, bs_test_failed(&sub_test));

    /** bs_test_fail takes precedence over bs_test_succeed. */
    memset(&sub_test, 0, sizeof(bs_test_t));
    bs_test_test_succeed_fail(&sub_test);
    BS_TEST_VERIFY_TRUE(test_ptr, sub_test.failed);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_test_failed(&sub_test));
    memset(&sub_test, 0, sizeof(bs_test_t));
    bs_test_test_fail_succeed(&sub_test);
    BS_TEST_VERIFY_TRUE(test_ptr, sub_test.failed);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_test_failed(&sub_test));
}

/**
 * Tests the EQ / NEQ macros.
 */
void bs_test_eq_neq_tests(bs_test_t *test_ptr)
{
    BS_TEST_VERIFY_EQ(test_ptr, 1, 1);
    BS_TEST_VERIFY_NEQ(test_ptr, 1, 2);

    BS_TEST_VERIFY_STREQ(test_ptr, "a", "a");
    BS_TEST_VERIFY_STRMATCH(test_ptr, "asdf", "^[a-z]+$");

    BS_TEST_VERIFY_MEMEQ(test_ptr, "asdf", "asdf", 4);
}

/** @endcond */
/* == End of test.c ======================================================== */
