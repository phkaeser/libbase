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
PROJECT(libbase)

# Configuration. Remove CMakeCache.txt to rerun...
OPTION(libbase_config_DEBUG "Include debugging information" ON)
OPTION(libbase_config_OPTIM "Optimizations" OFF)

IF (libbase_config_DEBUG)
  ADD_DEFINITIONS(-DDEBUG)
ENDIF (libbase_config_DEBUG)

# Toplevel gcc specific options
IF (CMAKE_COMPILER_IS_GNUCXX)
  IF (libbase_config_DEBUG)
    ADD_DEFINITIONS(-ggdb)
  ENDIF (libbase_config_DEBUG)

  IF (libbase_config_OPTIM)
    ADD_DEFINITIONS(-O2)
  ELSE (libbase_config_OPTIM)
    ADD_DEFINITIONS(-O0)
  ENDIF (libbase_config_OPTIM)

  ADD_DEFINITIONS(-std=c11 -Wall -Wextra -Werror)
ENDIF (CMAKE_COMPILER_IS_GNUCXX)

FIND_PACKAGE(Curses REQUIRED)

ADD_LIBRARY(
  base
  arg.c
  atomic.c
  avltree.c
  c2x_compat.c
  dequeue.c
  dllist.c
  file.c
  log.c
  sock.c
  subprocess.c
  strconvert.c
  test.c
  thread.c
  time.c)
TARGET_LINK_LIBRARIES(base ${CURSES_CURSES_LIBRARY})

ADD_EXECUTABLE(libbase_test libbase_test.c)
TARGET_LINK_LIBRARIES(libbase_test base)

ADD_EXECUTABLE(subprocess_test_hang subprocess_test_hang.c)
ADD_EXECUTABLE(subprocess_test_failure subprocess_test_failure.c)
ADD_EXECUTABLE(subprocess_test_sigpipe subprocess_test_sigpipe.c)
ADD_EXECUTABLE(subprocess_test_success subprocess_test_success.c)
