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
  ${PROJECT_SOURCE_DIR}/include/libbase/arg.h
  ${PROJECT_SOURCE_DIR}/include/libbase/assert.h
  ${PROJECT_SOURCE_DIR}/include/libbase/atomic.h
  ${PROJECT_SOURCE_DIR}/include/libbase/avltree.h
  ${PROJECT_SOURCE_DIR}/include/libbase/c2x_compat.h
  ${PROJECT_SOURCE_DIR}/include/libbase/def.h
  ${PROJECT_SOURCE_DIR}/include/libbase/dequeue.h
  ${PROJECT_SOURCE_DIR}/include/libbase/dllist.h
  ${PROJECT_SOURCE_DIR}/include/libbase/dynbuf.h
  ${PROJECT_SOURCE_DIR}/include/libbase/file.h
  ${PROJECT_SOURCE_DIR}/include/libbase/gfxbuf.h
  ${PROJECT_SOURCE_DIR}/include/libbase/gfxbuf_xpm.h
  ${PROJECT_SOURCE_DIR}/include/libbase/libbase.h
  ${PROJECT_SOURCE_DIR}/include/libbase/log.h
  ${PROJECT_SOURCE_DIR}/include/libbase/log_wrappers.h
  ${PROJECT_SOURCE_DIR}/include/libbase/ptr_set.h
  ${PROJECT_SOURCE_DIR}/include/libbase/ptr_stack.h
  ${PROJECT_SOURCE_DIR}/include/libbase/ptr_vector.h
  ${PROJECT_SOURCE_DIR}/include/libbase/sock.h
  ${PROJECT_SOURCE_DIR}/include/libbase/strutil.h
  ${PROJECT_SOURCE_DIR}/include/libbase/subprocess.h
  ${PROJECT_SOURCE_DIR}/include/libbase/test.h
  ${PROJECT_SOURCE_DIR}/include/libbase/thread.h
  ${PROJECT_SOURCE_DIR}/include/libbase/time.h
  ${PROJECT_SOURCE_DIR}/include/libbase/vector.h)

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

# Add 'install' target, but only if we're the toplevel project.
IF(CMAKE_PROJECT_NAME STREQUAL libbase)
  INSTALL(
    TARGETS libbase
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libbase")
ENDIF()
