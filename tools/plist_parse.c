/* ========================================================================= */
/**
 * @file plist_parse.c
 *
 * @copyright
 * Copyright 2024 Google LLC
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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <libbase/libbase.h>
#include <libbase/plist.h>

/*** Initial size for the output dynbuffer. */
static const size_t min_size = 1 << 16;

static uint32_t               indentation = 4;

/** Specification of commandline arguments. */
bs_arg_t args[] = {
    BS_ARG_UINT32(
        "indentation",
        "Indentation to use when writing the parsed plist. Default: 4.",
        4, 0, INT32_MAX, &indentation),
    BS_ARG_SENTINEL(),
};

/** Main program, runs the unit tests. */
int main(int argc, const char **argv)
{
    if (!bs_arg_parse(args, BS_ARG_MODE_EXTRA_VALUES, &argc, argv)) {
        bs_arg_print_usage(stderr, args);
        return EXIT_FAILURE;
    }

    if (2 != argc) {
        fprintf(stderr, "Usage: %s PLIST_FILE\n", argv[0]);
        return EXIT_FAILURE;
    }

    bspl_object_t *object_ptr = bspl_create_object_from_plist_file(argv[1]);
    if (NULL == object_ptr) {
        fprintf(stderr, "Failed bspl_create_object_from_plist_file(\"%s\")\n",
                argv[1]);
        return EXIT_FAILURE;
    }

    bs_dynbuf_t dynbuf;
    if (!bs_dynbuf_init(&dynbuf, min_size, SIZE_MAX)) {
        fprintf(stderr, "Failed bs_dynbuf_init(%p, %zu, %zu). "
                "Insufficient memory?\n", &dynbuf, min_size, SIZE_MAX);
        bspl_object_unref(object_ptr);
        return EXIT_FAILURE;
    }

    bool rv = bspl_object_write_indented(object_ptr, &dynbuf, indentation, 0);
    bspl_object_unref(object_ptr);
    if (!rv) {
        fprintf(stderr, "Failed bspl_object_write(%p, %p). "
                "Insufficient memory?\n", object_ptr, &dynbuf);
        bs_dynbuf_fini(&dynbuf);
        return EXIT_FAILURE;
    }

    size_t written = fwrite(dynbuf.data_ptr, dynbuf.length, 1, stdout);
    if (1 != written) {
        fprintf(stderr, "Failed fwrite(%p, %zu, 1, stdout)\n",
                dynbuf.data_ptr, dynbuf.length);
        bs_dynbuf_fini(&dynbuf);
        return EXIT_FAILURE;
    } else {
        fprintf(stdout, "\n");
    }

    bs_dynbuf_fini(&dynbuf);
    return EXIT_SUCCESS;
}
/* == End of plist_parse.c ================================================= */
