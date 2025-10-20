/* ========================================================================= */
/**
 * @file libbase.h
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
#ifndef __LIBBASE_H__
#define __LIBBASE_H__

// IWYU pragma: begin_exports
#include "arg.h"
#include "assert.h"
#include "atomic.h"
#include "avltree.h"
#include "c2x_compat.h"
#include "def.h"
#include "dequeue.h"
#include "dllist.h"
#include "dynbuf.h"
#include "file.h"
#include "gfxbuf.h"
#include "gfxbuf_xpm.h"
#include "log.h"
#include "log_wrappers.h"
#include "ptr_set.h"
#include "ptr_stack.h"
#include "ptr_vector.h"
#include "sock.h"
#include "strutil.h"
#include "subprocess.h"
#include "test.h"
#include "thread.h"
#include "time.h"
#include "vector.h"
// IWYU pragma: end_exports

// Hack to prevent IWYU from suggesting the sub-includes We don't want that.
// IWYU pragma: no_include "libbase/arg.h"
// IWYU pragma: no_include "libbase/assert.h"
// IWYU pragma: no_include "libbase/atomic.h"
// IWYU pragma: no_include "libbase/avltree.h"
// IWYU pragma: no_include "libbase/c2x_compat.h"
// IWYU pragma: no_include "libbase/def.h"
// IWYU pragma: no_include "libbase/dequeue.h"
// IWYU pragma: no_include "libbase/dllist.h"
// IWYU pragma: no_include "libbase/dynbuf.h"
// IWYU pragma: no_include "libbase/file.h"
// IWYU pragma: no_include "libbase/gfxbuf.h"
// IWYU pragma: no_include "libbase/gfxbuf_xpm.h"
// IWYU pragma: no_include "libbase/log.h"
// IWYU pragma: no_include "libbase/log_wrappers.h"
// IWYU pragma: no_include "libbase/ptr_set.h"
// IWYU pragma: no_include "libbase/ptr_stack.h"
// IWYU pragma: no_include "libbase/ptr_vector.h"
// IWYU pragma: no_include "libbase/sock.h"
// IWYU pragma: no_include "libbase/strutil.h"
// IWYU pragma: no_include "libbase/subprocess.h"
// IWYU pragma: no_include "libbase/test.h"
// IWYU pragma: no_include "libbase/thread.h"
// IWYU pragma: no_include "libbase/time.h"
// IWYU pragma: no_include "libbase/vector.h"

#endif /* __LIBBASE_H__ */
/* == End of libbase.h ================================================== */
