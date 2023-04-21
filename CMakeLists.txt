# Copyright 2023 Google LLC
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

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
PROJECT(libbase VERSION 1.0 DESCRIPTION "A small standard library for C")

# Configuration. Remove CMakeCache.txt to rerun...
OPTION(config_DEBUG "Include debugging information" ON)
OPTION(config_OPTIM "Optimizations" OFF)

IF(config_DEBUG)
  ADD_COMPILE_OPTIONS(-DDEBUG)
ENDIF(config_DEBUG)

# Configuration. Remove CMakeCache.txt to re-generate.
OPTION(config_DEBUG "Enable Debug mode" ON)
OPTION(config_OPTIM "Optimize" OFF)

# Toplevel compile options, for GCC.
IF(CMAKE_COMPILER_IS_GNUCXX)
  IF(config_DEBUG)
    ADD_COMPILE_OPTIONS(-O2)
  ENDIF(config_DEBUG)

  IF(config_OPTIM)
    ADD_COMPILE_OPTIONS(-O2)
  ELSE(config_OPTIM)
    ADD_COMPILE_OPTIONS(-O0)
  ENDIF(config_OPTIM)
  ADD_COMPILE_OPTIONS(-std=c11 -Wall -Wextra -Werror)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

FIND_PACKAGE(Curses REQUIRED)

SET(PUBLIC_HEADER_FILES
  arg.h
  assert.h
  atomic.h
  avltree.h
  c2x_compat.h
  clock.h
  def.h
  dequeue.h
  dllist.h
  file.h
  libbase.h
  log.h
  log_wrappers.h
  sock.h
  strconvert.h
  subprocess.h
  test.h
  thread.h
  vector.h)

ADD_LIBRARY(base)
SET_TARGET_PROPERTIES(
  base PROPERTIES
  VERSION 1.0
  PUBLIC_HEADER ${PUBLIC_HEADER_FILES})
TARGET_SOURCES(
  base
  PRIVATE
  arg.c
  atomic.c
  avltree.c
  c2x_compat.c
  clock.c
  dequeue.c
  dllist.c
  file.c
  log.c
  sock.c
  subprocess.c
  strconvert.c
  test.c
  thread.c)

TARGET_LINK_LIBRARIES(base ${CURSES_CURSES_LIBRARY})


ADD_EXECUTABLE(libbase_test libbase_test.c)
TARGET_LINK_LIBRARIES(libbase_test base)

ADD_EXECUTABLE(subprocess_test_hang subprocess_test_hang.c)
ADD_EXECUTABLE(subprocess_test_failure subprocess_test_failure.c)
ADD_EXECUTABLE(subprocess_test_sigpipe subprocess_test_sigpipe.c)
ADD_EXECUTABLE(subprocess_test_success subprocess_test_success.c)
