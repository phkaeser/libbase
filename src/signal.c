/* ========================================================================= */
/**
 * @file signal.c
 * Copyright (c) 2026 Philipp Kaeser
 *
 * @copyright
 * Copyright 2026 Google LLC
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

#include <libbase/signal.h>

#include <libbase/libbase.h>
#include <stdbool.h>

/* == Declarations ========================================================= */

static void _bs_signal_notify_dlnode(
    bs_dllist_node_t *dlnode_ptr,
    void *ud_ptr);
static void _bs_test_listener_notify(
    struct bs_listener *listener_ptr,
    void *data_ptr);

/* == Exported methods ===================================================== */

/* ------------------------------------------------------------------------- */
void bs_signal_emit(
    const struct bs_signal *signal_ptr,
    void *data_ptr)
{
    bs_dllist_for_each(
        &signal_ptr->listeners,
        _bs_signal_notify_dlnode,
        data_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_listener_connect(
    struct bs_listener *listener_ptr,
    bs_signal_notify_t notify,
    struct bs_signal *signal_ptr)
{
    listener_ptr->notify = notify;
    bs_dllist_push_back(
        &signal_ptr->listeners,
        &listener_ptr->dlnode);
}

/* ------------------------------------------------------------------------- */
void bs_listener_disconnect(
    struct bs_listener *listener_ptr,
    struct bs_signal *signal_ptr)
{
    bs_dllist_remove(
        &signal_ptr->listeners,
        &listener_ptr->dlnode);
}

/* ------------------------------------------------------------------------- */
void bs_test_listener_clear(
    struct bs_test_listener *test_listener_ptr)
{
    test_listener_ptr->calls = 0;
    test_listener_ptr->last_data_ptr = NULL;
}

/* ------------------------------------------------------------------------- */
void bs_test_listener_connect(
    struct bs_signal *signal_ptr,
    struct bs_test_listener *test_listener_ptr)
{
    bs_listener_connect(
        &test_listener_ptr->listener,
        _bs_test_listener_notify,
        signal_ptr);
}

/* ------------------------------------------------------------------------- */
void bs_test_listener_disconnect(
    struct bs_signal *signal_ptr,
    struct bs_test_listener *test_listener_ptr)
{
    bs_listener_disconnect(
        &test_listener_ptr->listener,
        signal_ptr);
}

/* == Local (static) methods =============================================== */

/* ------------------------------------------------------------------------- */
/** Iterator for @ref bs_dllist_for_each: Calls @ref bs_listener::notify. */
void _bs_signal_notify_dlnode(
    bs_dllist_node_t *dlnode_ptr,
    void *ud_ptr)
{
    struct bs_listener *listener_ptr = BS_CONTAINER_OF(
        dlnode_ptr, struct bs_listener, dlnode);
    listener_ptr->notify(listener_ptr, ud_ptr);
}

/* ------------------------------------------------------------------------- */
void _bs_test_listener_notify(
    struct bs_listener *listener_ptr,
    void *data_ptr)
{
    struct bs_test_listener *tl = BS_CONTAINER_OF(
        listener_ptr, struct bs_test_listener, listener);
    tl->calls++;
    tl->last_data_ptr = data_ptr;
}

/* == Unit tests =========================================================== */

void _bs_signal_test_simple(bs_test_t *test_ptr);

/** Unit test cases. */
static const bs_test_case_t   _bs_signal_test_cases[] = {
    { true, "simple", _bs_signal_test_simple },
    BS_TEST_CASE_SENTINEL()
};

/** Unit tests set. */
const bs_test_set_t           bs_signal_test_set = BS_TEST_SET(
    true, "signal", _bs_signal_test_cases);

/* ------------------------------------------------------------------------- */
/** Simple test case for the signal. */
void _bs_signal_test_simple(bs_test_t *test_ptr)
{
    struct bs_test_listener tl1 = {}, tl2 = {};
    struct bs_signal s = {};

    bs_test_listener_connect(&s, &tl1);

    BS_TEST_VERIFY_EQ(
        test_ptr,
        &tl1,
        BS_CONTAINER_OF(&tl1.calls, struct bs_test_listener, calls));
    void *p = NULL;
    BS_TEST_VERIFY_EQ(
        test_ptr,
        NULL,
        BS_CONTAINER_OF(p, struct bs_test_listener, calls));

    BS_TEST_VERIFY_EQ(test_ptr, 0, tl1.calls);
    BS_TEST_VERIFY_EQ(test_ptr, 0, tl2.calls);

    bs_signal_emit(&s, _bs_signal_test_simple);
    BS_TEST_VERIFY_EQ(test_ptr, 1, tl1.calls);
    BS_TEST_VERIFY_EQ(test_ptr, _bs_signal_test_simple, tl1.last_data_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, 0, tl2.calls);

    bs_test_listener_connect(&s, &tl2);
    bs_signal_emit(&s, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, 2, tl1.calls);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, tl1.last_data_ptr);
    BS_TEST_VERIFY_EQ(test_ptr, 1, tl2.calls);
    BS_TEST_VERIFY_EQ(test_ptr, NULL, tl2.last_data_ptr);

    bs_test_listener_disconnect(&s, &tl1);
    bs_signal_emit(&s, NULL);
    BS_TEST_VERIFY_EQ(test_ptr, 2, tl2.calls);

    bs_test_listener_clear(&tl1);
    BS_TEST_VERIFY_EQ(test_ptr, 0, tl1.calls);
    bs_test_listener_clear(&tl2);
    BS_TEST_VERIFY_EQ(test_ptr, 0, tl2.calls);
}

/* == End of signal.c ====================================================== */
