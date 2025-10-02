# Copyright 2025 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

SET(PUBLIC_HEADER_FILES
  include/libbase/arg.h
  include/libbase/assert.h
  include/libbase/atomic.h
  include/libbase/avltree.h
  include/libbase/c2x_compat.h
  include/libbase/def.h
  include/libbase/dequeue.h
  include/libbase/dllist.h
  include/libbase/dynbuf.h
  include/libbase/file.h
  include/libbase/gfxbuf.h
  include/libbase/gfxbuf_xpm.h
  include/libbase/libbase.h
  include/libbase/log.h
  include/libbase/log_wrappers.h
  include/libbase/ptr_set.h
  include/libbase/ptr_stack.h
  include/libbase/ptr_vector.h
  include/libbase/sock.h
  include/libbase/strutil.h
  include/libbase/subprocess.h
  include/libbase/test.h
  include/libbase/thread.h
  include/libbase/time.h
  include/libbase/vector.h)

SET(SOURCES
  arg.c
  atomic.c
  avltree.c
  c2x_compat.c
  dequeue.c
  dllist.c
  dynbuf.c
  file.c
  gfxbuf.c
  gfxbuf_xpm.c
  log.c
  ptr_set.c
  ptr_stack.c
  ptr_vector.c
  sock.c
  strutil.c
  subprocess.c
  test.c
  thread.c
  time.c)

ADD_LIBRARY(libbase STATIC)
TARGET_SOURCES(libbase PRIVATE ${SOURCES})
TARGET_INCLUDE_DIRECTORIES(libbase PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
TARGET_INCLUDE_DIRECTORIES(libbase PRIVATE ${CURSES_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(libbase PRIVATE ${CURSES_LIBRARIES})
SET_TARGET_PROPERTIES(
  libbase PROPERTIES
  VERSION 1.0
  PUBLIC_HEADER "${PUBLIC_HEADER_FILES}"
)

IF(iwyu_path_and_options)
  SET_TARGET_PROPERTIES(
    libbase PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
ENDIF(iwyu_path_and_options)

IF(CAIRO_FOUND)
  TARGET_COMPILE_DEFINITIONS(libbase PUBLIC HAVE_CAIRO)
  TARGET_INCLUDE_DIRECTORIES(libbase PUBLIC ${CAIRO_INCLUDE_DIRS})
  TARGET_LINK_LIBRARIES(libbase PUBLIC PkgConfig::CAIRO)
ENDIF(CAIRO_FOUND)
