/* ========================================================================= */
/**
 * @file signal.h
 *
 * Support for publisher/subscriber flows, similar to Wayland's server-side
 * wl_signal. Implemented separately, so it can be used w/o Wayland's server.
 *
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
#ifndef __SIGNAL_H__
#define __SIGNAL_H__

#include <libbase/libbase.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

struct bs_listener;

/** Notifier function for a listener. */
typedef void (*bs_signal_notify_t)(
    struct bs_listener *listener_ptr,
    void *data_ptr);

/** A signal (the publisher). */
struct bs_signal {
    /** Subscribed listeners, through @ref bs_listener::dlnode. */
    bs_dllist_t               listeners;
};

/** A listener (the subscriber). */
struct bs_listener {
    /** Element of @ref bs_signal::listeners. */
    bs_dllist_node_t          dlnode;
    /** The notifier for this subscriber. */
    bs_signal_notify_t        notify;
};

/**
 * Emit the signal, with the provided argument.
 *
 * Calls each listener, with `data_ptr` as argument. It is safe to disconnect
 * `listener_ptr` from `signal_ptr`, but not to modify other listeners.
 *
 * @param signal_ptr
 * @param data_ptr
 */
void bs_signal_emit(
    const struct bs_signal *signal_ptr,
    void *data_ptr);

/**
 * Connects the listener with the provided notification function to the signal.
 *
 * @param signal_ptr
 * @param listener_ptr
 * @param notify
 */
void bs_signal_connect(
    struct bs_signal *signal_ptr,
    struct bs_listener *listener_ptr,
    bs_signal_notify_t notify);

/**
 * Disconnects the listener from the signal.
 *
 * @param signal_ptr
 * @param listener_ptr
 */
void bs_signal_disconnect(
    struct bs_signal *signal_ptr,
    struct bs_listener *listener_ptr);

/** A listener that can be used in unit tests. */
struct bs_test_listener {
    /** The listener. */
    struct bs_listener        listener;
    /** Calls since initialization or last @ref bs_test_listener_clear. */
    size_t                    calls;
    /** Argumet of last call, if any since @ref bs_test_listener_clear. */
    void                      *last_data_ptr;
};

/**
 * Clears @ref bs_test_listener::calls and
 * @ref bs_test_listener::last_data_ptr.
 *
 * @param test_listener_ptr
 */
void bs_test_listener_clear(
    struct bs_test_listener *test_listener_ptr);

/** Connects the test listener to `signal_ptr`. */
void bs_test_listener_connect(
    struct bs_signal *signal_ptr,
    struct bs_test_listener *test_listener_ptr);

/** Disconnects the test listener to `signal_ptr`. */
void bs_test_listener_disconnect(
    struct bs_signal *signal_ptr,
    struct bs_test_listener *test_listener_ptr);

/** Unit tests set. */
extern const bs_test_set_t    bs_signal_test_set;

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif // __SIGNAL_H__
/* == End of signal.h ====================================================== */
