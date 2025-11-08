/* ========================================================================= */
/**
 * @file dynbuf.c
 *
 * @copyright
 * Copyright 2025 Google LLC
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

#include "libbase/dynbuf.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <libbase/assert.h>
#include <libbase/file.h>
#include <libbase/log.h>
#include <libbase/log_wrappers.h>
#include <libbase/strutil.h>
#include <libbase/test.h>

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
bool bs_dynbuf_init(
    bs_dynbuf_t *dynbuf_ptr,
    size_t initial_capacity,
    size_t max_capacity)
{
    *dynbuf_ptr = (bs_dynbuf_t){};

    if (0 >= initial_capacity ||
        initial_capacity > max_capacity ||
        0 >= max_capacity) {
        return false;
    }

    *dynbuf_ptr = (bs_dynbuf_t){
        .data_ptr = logged_calloc(1, initial_capacity),
        .capacity = initial_capacity,
        .max_capacity = max_capacity,
        .unmanaged = false
    };
    if (NULL == dynbuf_ptr->data_ptr) return false;
    return true;
}

/* ------------------------------------------------------------------------- */
void bs_dynbuf_init_unmanaged(
    bs_dynbuf_t *dynbuf_ptr,
    void *data_ptr,
    size_t capacity)
{
    *dynbuf_ptr = (bs_dynbuf_t){
        .data_ptr = data_ptr,
        .length = 0,
        .capacity = capacity,
        .max_capacity = capacity,
        .unmanaged = true
    };
}

/* ------------------------------------------------------------------------- */
void bs_dynbuf_fini(bs_dynbuf_t *dynbuf_ptr)
{
    if (NULL != dynbuf_ptr->data_ptr) {
        if (!dynbuf_ptr->unmanaged) free(dynbuf_ptr->data_ptr);
        dynbuf_ptr->data_ptr = NULL;
    }
    dynbuf_ptr->unmanaged = false;

    dynbuf_ptr->capacity = 0;
}

/* ------------------------------------------------------------------------- */
bs_dynbuf_t *bs_dynbuf_create(
    size_t initial_capacity,
    size_t max_capacity)
{
    bs_dynbuf_t *dynbuf_ptr = logged_malloc(sizeof(bs_dynbuf_t));
    if (NULL == dynbuf_ptr) return NULL;

    if (bs_dynbuf_init(dynbuf_ptr, initial_capacity, max_capacity)) {
        return dynbuf_ptr;
    }

    bs_dynbuf_destroy(dynbuf_ptr);
    return NULL;
}

/* ------------------------------------------------------------------------- */
void bs_dynbuf_destroy(bs_dynbuf_t *dynbuf_ptr)
{
    bs_dynbuf_fini(dynbuf_ptr);
    free(dynbuf_ptr);
}

/* ------------------------------------------------------------------------- */
bool bs_dynbuf_grow(bs_dynbuf_t *dynbuf_ptr)
{
    size_t new_capacity = dynbuf_ptr->max_capacity;
    if (dynbuf_ptr->capacity <= dynbuf_ptr->max_capacity >> 1) {
        new_capacity = dynbuf_ptr->capacity << 1;
    }
    if (dynbuf_ptr->capacity == new_capacity) return false;

    void *new_data_ptr = realloc(dynbuf_ptr->data_ptr, new_capacity);
    if (NULL == new_data_ptr) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed realloc(%p, %zu)",
               dynbuf_ptr->data_ptr, new_capacity);
        return false;
    }
    dynbuf_ptr->data_ptr = new_data_ptr;
    dynbuf_ptr->capacity = new_capacity;
    return true;
}

/* ------------------------------------------------------------------------- */
bool bs_dynbuf_full(bs_dynbuf_t *dynbuf_ptr)
{
    return dynbuf_ptr->length >= dynbuf_ptr->capacity;
}

/* ------------------------------------------------------------------------- */
void bs_dynbuf_clear(bs_dynbuf_t *dynbuf_ptr)
{
    dynbuf_ptr->length = 0;
}

/* ------------------------------------------------------------------------- */
int bs_dynbuf_read(bs_dynbuf_t *dynbuf_ptr, int fd)
{
    if (bs_dynbuf_full(dynbuf_ptr)) {
        if (!bs_dynbuf_grow(dynbuf_ptr)) return -1;
    }
    BS_ASSERT(dynbuf_ptr->capacity > dynbuf_ptr->length);

    ssize_t read_bytes = read(
        fd,
        (uint8_t*)dynbuf_ptr->data_ptr + dynbuf_ptr->length,
        dynbuf_ptr->capacity - dynbuf_ptr->length);
    if (0 > read_bytes) {
        if (EAGAIN == errno || EWOULDBLOCK == errno) return 1;
        bs_log(BS_ERROR | BS_ERRNO, "Failed read(%d, %p, %zu)",
               fd,
               (uint8_t*)dynbuf_ptr->data_ptr + dynbuf_ptr->length,
               dynbuf_ptr->capacity - dynbuf_ptr->length);
        return -1;
    } else if (0 == read_bytes) {
        return 0;
    }

    dynbuf_ptr->length += read_bytes;
    return bs_dynbuf_read(dynbuf_ptr, fd);
}

/* ------------------------------------------------------------------------- */
bool bs_dynbuf_write_file(
    bs_dynbuf_t *dynbuf_ptr,
    const char *fname_ptr,
    int mode)
{
    struct stat sbuf;

    // Create a temporary file in the target's directory.
    char *fname_copy_ptr = logged_strdup(fname_ptr);
    if (NULL == fname_copy_ptr) return false;
    char *dir = dirname(fname_copy_ptr);

    if (0 != stat(dir, &sbuf)) {
        if (ENOENT != errno) {
            bs_log(BS_ERROR | BS_ERRNO, "Failed stat(\"%s\", %p)", dir, &sbuf);
            free(dir);
            return false;
        }
        if (!bs_file_mkdir_p(dir, mode | S_IXUSR)) {
            free(dir);
            return false;
        }
    }

    char *tmp_fname_ptr = bs_strdupf("%s/dynbuf-XXXXXX", dir);
    free(fname_copy_ptr);
    if (NULL == tmp_fname_ptr) {
        bs_log(BS_ERROR, "Failed bs_strdupf(\"<dir>/dynbuf-XXXXXX\") for %s",
               fname_ptr);
        return false;
    }

    mode_t old_mask = umask((S_IRWXU | S_IRWXG | S_IRWXO) ^ mode);
    int fd = mkstemp(tmp_fname_ptr);
    umask(old_mask);
    if (0 > fd) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed mkstemp(%s)", tmp_fname_ptr);
        free(tmp_fname_ptr);
        return false;
    }

    // Write contents.
    ssize_t written = write(fd, dynbuf_ptr->data_ptr, dynbuf_ptr->length);
    if (written < 0 || (size_t)written != dynbuf_ptr->length) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed write(%d, %p, %zu): %zd",
               fd, dynbuf_ptr->data_ptr, dynbuf_ptr->length, written);
        close(fd);
        goto error;
    }
    close(fd);

    // If the target exists already: Verify it's writable. Create bakcup.
    if (0 == stat(fname_ptr, &sbuf)) {
        if ((sbuf.st_mode & S_IWUSR) != S_IWUSR) {
            bs_log(BS_ERROR, "File \"%s\" mode %o not user-writable",
                   fname_ptr, sbuf.st_mode);
            goto error;
        }

        char *backup_fname_ptr = bs_strdupf("%s.old", fname_ptr);
        if (NULL == backup_fname_ptr) return false;
        unlink(backup_fname_ptr);
        int rv = link(fname_ptr, backup_fname_ptr);
        free(backup_fname_ptr);
        if (0 != rv) {
            bs_log(BS_ERROR | BS_ERRNO, "Failed link(\"%s\", \"%s.old\")",
                   fname_ptr, fname_ptr);
            goto error;
        }
    }

    // Last: Atomically replace target with the written temp file.
    int rv = rename(tmp_fname_ptr, fname_ptr);
    if (0 != rv) {
        bs_log(BS_ERROR | BS_ERRNO, "Failed rename(..., \"%s\")", fname_ptr);
        goto error;
    }

    free(tmp_fname_ptr);
    return true;

error:
    unlink(tmp_fname_ptr);
    free(tmp_fname_ptr);
    return false;
}

/* ------------------------------------------------------------------------- */
bool bs_dynbuf_append(
    bs_dynbuf_t *dynbuf_ptr,
    const void *data_ptr,
    size_t len)
{
    if (len > dynbuf_ptr->capacity ||
        len + dynbuf_ptr->length > dynbuf_ptr->capacity) return false;

    memcpy((uint8_t*)dynbuf_ptr->data_ptr + dynbuf_ptr->length,
           data_ptr,
           len);
    dynbuf_ptr->length += len;
    return true;
}

/* ------------------------------------------------------------------------- */
bool bs_dynbuf_append_char(
    bs_dynbuf_t *dynbuf_ptr,
    char c)
{
    if (dynbuf_ptr->length + 1 > dynbuf_ptr->capacity) return false;
    ((char*)dynbuf_ptr->data_ptr)[dynbuf_ptr->length++] = c;
    return true;
}


/* == Tests ================================================================ */

static void test_dynbuf_ctor_dtor(bs_test_t *test_ptr);
static void test_dynbuf_read(bs_test_t *test_ptr);
static void test_dynbuf_read_capped(bs_test_t *test_ptr);
static void test_dynbuf_write(bs_test_t *test_ptr);
static void test_dynbuf_append(bs_test_t *test_ptr);

const bs_test_case_t          bs_dynbuf_test_cases[] = {
    { 1, "ctor_dtor", test_dynbuf_ctor_dtor },
    { 1, "read", test_dynbuf_read },
    { 1, "read_capped", test_dynbuf_read_capped },
    { 1, "write", test_dynbuf_write },
    { 1, "append", test_dynbuf_append },
    { 0, NULL, NULL },
};

/* ------------------------------------------------------------------------- */
void test_dynbuf_ctor_dtor(bs_test_t *test_ptr)
{
    bs_dynbuf_t *dynbuf_ptr = bs_dynbuf_create(1, SIZE_MAX);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, dynbuf_ptr);
    bs_dynbuf_destroy(dynbuf_ptr);

    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_dynbuf_create(0, SIZE_MAX));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_dynbuf_create(1, 0));
    BS_TEST_VERIFY_EQ(test_ptr, NULL, bs_dynbuf_create(2, 1));
}

/* ------------------------------------------------------------------------- */
void test_dynbuf_read(bs_test_t *test_ptr)
{
    bs_dynbuf_t d;
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, bs_dynbuf_init(&d, 1, SIZE_MAX));

    int fd = open(bs_test_resolve_path("data/abcd.txt"), 0);
    if (0 >= fd) {
        BS_TEST_FAIL(test_ptr, "Failed open(\"%s\", 0)",
                     bs_test_resolve_path("data/string.plist"));
        return;
    }
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, 0 == bs_dynbuf_read(&d, fd));
    close(fd);

    BS_TEST_VERIFY_EQ_OR_RETURN(test_ptr, 5, d.length);
    BS_TEST_VERIFY_EQ(test_ptr, 0, memcmp("abcd\n", d.data_ptr, d.length));
    bs_dynbuf_fini(&d);

    char buf[6];
    bs_dynbuf_init_unmanaged(&d, buf, sizeof(buf));
    fd = open(bs_test_resolve_path("data/abcd.txt"), 0);
    if (0 >= fd) {
        BS_TEST_FAIL(test_ptr, "Failed open(\"%s\", 0)",
                     bs_test_resolve_path("data/string.plist"));
        return;
    }
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, 0 == bs_dynbuf_read(&d, fd));
    close(fd);

    BS_TEST_VERIFY_EQ_OR_RETURN(test_ptr, 5, d.length);
    BS_TEST_VERIFY_EQ(test_ptr, 0, memcmp("abcd\n", d.data_ptr, d.length));
    bs_dynbuf_fini(&d);
}

/* ------------------------------------------------------------------------- */
void test_dynbuf_read_capped(bs_test_t *test_ptr)
{
    bs_dynbuf_t d;
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, bs_dynbuf_init(&d, 1, 3));
    int fd = open(bs_test_resolve_path("data/abcd.txt"), 0);
    if (0 >= fd) {
        BS_TEST_FAIL(test_ptr, "Failed open(\"%s\", 0)",
                     bs_test_resolve_path("data/string.plist"));
        return;
    }
    BS_TEST_VERIFY_TRUE(test_ptr, -1 == bs_dynbuf_read(&d, fd));
    close(fd);

    BS_TEST_VERIFY_EQ_OR_RETURN(test_ptr, 3, d.length);
    BS_TEST_VERIFY_EQ(test_ptr, 0, memcmp("abc", d.data_ptr, d.length));
    bs_dynbuf_fini(&d);

    char buf[3];
    bs_dynbuf_init_unmanaged(&d, buf, sizeof(buf));
    fd = open(bs_test_resolve_path("data/abcd.txt"), 0);
    if (0 >= fd) {
        BS_TEST_FAIL(test_ptr, "Failed open(\"%s\", 0)",
                     bs_test_resolve_path("data/string.plist"));
        return;
    }
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, -1 == bs_dynbuf_read(&d, fd));
    close(fd);

    BS_TEST_VERIFY_EQ_OR_RETURN(test_ptr, 3, d.length);
    BS_TEST_VERIFY_EQ(test_ptr, 0, memcmp("abc", d.data_ptr, d.length));
    bs_dynbuf_fini(&d);
}

/* ------------------------------------------------------------------------- */
void test_dynbuf_write(bs_test_t *test_ptr)
{
    struct stat stat_buffer;
    char tmp_path[] = "/tmp/dynbuf-XXXXXX";

    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, mkdtemp(tmp_path));

    char *fn1 = bs_strdupf("%s/sub/file.txt", tmp_path);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, fn1);
    char *fn2 = bs_strdupf("%s/sub/file.txt.old", tmp_path);
    BS_TEST_VERIFY_NEQ_OR_RETURN(test_ptr, NULL, fn2);

    char content[8] = {};
    bs_dynbuf_t db;
    bs_dynbuf_init_unmanaged(&db, content, sizeof(content));

    // Write initial content. Must create one file, size 4.
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, bs_dynbuf_append(&db, "test", 4));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_write_file(&db, fn1, 0600));
    BS_TEST_VERIFY_EQ(test_ptr, 0, stat(fn1, &stat_buffer));
    BS_TEST_VERIFY_EQ(test_ptr, 4, (size_t)stat_buffer.st_size);
    BS_TEST_VERIFY_NEQ(test_ptr, 0, stat(fn2, &stat_buffer));

    // Write further content. Expect a file with size 5, and backup.
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, bs_dynbuf_append_char(&db, 'a'));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_write_file(&db, fn1, 0600));
    BS_TEST_VERIFY_EQ(test_ptr, 0, stat(fn1, &stat_buffer));
    BS_TEST_VERIFY_EQ(test_ptr, 5, (size_t)stat_buffer.st_size);
    BS_TEST_VERIFY_EQ(test_ptr, 0, stat(fn2, &stat_buffer));
    BS_TEST_VERIFY_EQ(test_ptr, 4, (size_t)stat_buffer.st_size);

    // Further content: Rotates the files.
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, bs_dynbuf_append_char(&db, 'b'));
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_write_file(&db, fn1, 0600));
    BS_TEST_VERIFY_EQ(test_ptr, 0, stat(fn1, &stat_buffer));
    BS_TEST_VERIFY_EQ(test_ptr, 6, (size_t)stat_buffer.st_size);
    BS_TEST_VERIFY_EQ(test_ptr, 0, stat(fn2, &stat_buffer));
    BS_TEST_VERIFY_EQ(test_ptr, 5, (size_t)stat_buffer.st_size);

    // The file is read-only: Our write must fail.
    BS_TEST_VERIFY_EQ(test_ptr, 0, chmod(fn1, S_IRUSR));
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, bs_dynbuf_append_char(&db, 'c'));
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dynbuf_write_file(&db, fn1, 0600));
    BS_TEST_VERIFY_EQ(test_ptr, 0, chmod(fn1, S_IRUSR | S_IWUSR));

    // Must be able to clean up the file.
    unlink(fn2);
    unlink(fn1);
    rmdir(dirname(fn1));
    free(fn2);
    free(fn1);
    BS_TEST_VERIFY_EQ(test_ptr, 0, rmdir(tmp_path));
}

/* ------------------------------------------------------------------------- */
/** Tests appending. */
void test_dynbuf_append(bs_test_t *test_ptr)
{
    bs_dynbuf_t d;
    BS_TEST_VERIFY_TRUE_OR_RETURN(test_ptr, bs_dynbuf_init(&d, 3, 3));

    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_append(&d, "ab", 2));
    BS_TEST_VERIFY_EQ(test_ptr, 2, d.length);
    BS_TEST_VERIFY_MEMEQ(test_ptr, "ab", d.data_ptr, 2);

    BS_TEST_VERIFY_FALSE(test_ptr, bs_dynbuf_append(&d, "cd", 2));
    BS_TEST_VERIFY_EQ(test_ptr, 2, d.length);
    BS_TEST_VERIFY_MEMEQ(test_ptr, "ab", d.data_ptr, 2);

    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_append(&d, "c", 1));
    BS_TEST_VERIFY_EQ(test_ptr, 3, d.length);
    BS_TEST_VERIFY_MEMEQ(test_ptr, "abc", d.data_ptr, 3);

    BS_TEST_VERIFY_FALSE(test_ptr, bs_dynbuf_append(&d, "d", 1));

    d.length = 2;
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_append_char(&d, 'x'));
    BS_TEST_VERIFY_EQ(test_ptr, 3, d.length);
    BS_TEST_VERIFY_MEMEQ(test_ptr, "abx", d.data_ptr, 3);
    BS_TEST_VERIFY_FALSE(test_ptr, bs_dynbuf_append_char(&d, 'y'));

    d.length = 2;
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_maybe_append_char(&d, false, 'z'));
    BS_TEST_VERIFY_EQ(test_ptr, 2, d.length);
    BS_TEST_VERIFY_TRUE(test_ptr, bs_dynbuf_maybe_append_char(&d, true, 'z'));
    BS_TEST_VERIFY_EQ(test_ptr, 3, d.length);
    BS_TEST_VERIFY_MEMEQ(test_ptr, "abz", d.data_ptr, 3);

    bs_dynbuf_fini(&d);
}

/* == End of dynbuf.c ====================================================== */
