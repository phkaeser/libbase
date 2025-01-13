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

CMAKE_MINIMUM_REQUIRED(VERSION 3.13)
PROJECT(libbase VERSION 1.0
  DESCRIPTION "A small standard library for C"
  LANGUAGES C)

# Requires out-of-source builds.
FILE(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOCATION_PATH)
IF(EXISTS "${LOCATION_PATH}")
  MESSAGE(
    FATAL_ERROR
    "You cannot build into a source directory (containing a CMakeLists.txt file).\n"
    "Please make a build subdirectory, for example:\n"
    "cmake -B build\n"
    "(cd build && make)")
ENDIF()

# Defaults to /usr/local/lib for installation.
INCLUDE(GNUInstallDirs)
INCLUDE(FindCurses)

IF(NOT CURSES_FOUND)
  MESSAGE(
    FATAL_ERROR
    "Failed to find curses nor ncurses file and library.")
ENDIF(NOT CURSES_FOUND)

# Configuration. Remove CMakeCache.txt to rerun...
OPTION(config_DEBUG "Include debugging information" ON)
OPTION(config_OPTIM "Optimizations" OFF)
OPTION(config_COVERAGE "Build with test coverage" OFF)

# Toplevel compile options, for Clang and GCC.
IF(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  IF(config_DEBUG)
    ADD_COMPILE_OPTIONS(-ggdb -DDEBUG)
  ENDIF(config_DEBUG)

  IF(config_OPTIM)
    ADD_COMPILE_OPTIONS(-O2)
  ELSE(config_OPTIM)
    ADD_COMPILE_OPTIONS(-O0)
  ENDIF(config_OPTIM)
  ADD_COMPILE_OPTIONS(-Wall -Wextra -Werror)

  # CMake provides absolute paths to GCC, hence the __FILE__ macro includes the
  # full path. This option resets it to a path relative to project source.
  ADD_COMPILE_OPTIONS(-fmacro-prefix-map=${PROJECT_SOURCE_DIR}=.)
ENDIF()
SET(CMAKE_C_STANDARD 11)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(CAIRO REQUIRED IMPORTED_TARGET cairo>=1.16.0)

SET(PUBLIC_HEADER_FILES
  arg.h
  assert.h
  atomic.h
  avltree.h
  c2x_compat.h
  def.h
  dequeue.h
  dllist.h
  file.h
  gfxbuf.h
  gfxbuf_xpm.h
  libbase.h
  log.h
  log_wrappers.h
  ptr_set.h
  ptr_stack.h
  ptr_vector.h
  sock.h
  strutil.h
  subprocess.h
  test.h
  thread.h
  time.h
  vector.h)

SET(SOURCES
  arg.c
  atomic.c
  avltree.c
  c2x_compat.c
  dequeue.c
  dllist.c
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

ADD_LIBRARY(base STATIC)
TARGET_SOURCES(base PRIVATE ${SOURCES})
TARGET_INCLUDE_DIRECTORIES(base PRIVATE ${CURSES_INCLUDE_DIRS})
TARGET_INCLUDE_DIRECTORIES(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
TARGET_LINK_LIBRARIES(base PRIVATE ${CURSES_LIBRARIES})
SET_TARGET_PROPERTIES(
  base PROPERTIES
  VERSION 1.0
  PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")

IF(CAIRO_FOUND)
  TARGET_COMPILE_DEFINITIONS(base PUBLIC HAVE_CAIRO)
  TARGET_INCLUDE_DIRECTORIES(base PUBLIC ${CAIRO_INCLUDE_DIRS})
  TARGET_LINK_LIBRARIES(base PUBLIC PkgConfig::CAIRO)
ENDIF(CAIRO_FOUND)

# Create pkg-config file from the template.
CONFIGURE_FILE(libbase.pc.in ${CMAKE_BINARY_DIR}/libbase.pc @ONLY)

# Add 'install' target, but only if we're the toplevel project.
IF(CMAKE_PROJECT_NAME STREQUAL libbase)
  INSTALL(
    TARGETS base
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libbase")

  INSTALL(
    FILES ${CMAKE_BINARY_DIR}/libbase.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
ENDIF()

# Add test target, but only if we're the toplevel project.
IF(CMAKE_PROJECT_NAME STREQUAL libbase)
  ADD_EXECUTABLE(libbase_test libbase_test.c)
  TARGET_LINK_LIBRARIES(libbase_test base)
  # Refer to the source path for relative testdata.
  TARGET_COMPILE_DEFINITIONS(
    libbase_test PUBLIC BS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}")


  ADD_EXECUTABLE(libbase_benchmark libbase_benchmark.c)
  TARGET_LINK_LIBRARIES(libbase_benchmark base)

  ADD_EXECUTABLE(subprocess_test_hang subprocess_test_hang.c)
  ADD_EXECUTABLE(subprocess_test_failure subprocess_test_failure.c)
  ADD_EXECUTABLE(subprocess_test_sigpipe subprocess_test_sigpipe.c)
  ADD_EXECUTABLE(subprocess_test_success subprocess_test_success.c)

  INCLUDE(CTest)
  IF(BUILD_TESTING)
    ADD_TEST(NAME libbase_test COMMAND libbase_test)
  ENDIF()
ENDIF()

# Adds 'doc' target, if doxygen is installed.
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
  # Set input and output files.
  SET(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
  SET(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # Configure the file.
  CONFIGURE_FILE(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
  ADD_CUSTOM_TARGET(
    libbase_doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
    DEPENDS ${DOXYGEN_OUT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen."
    VERBATIM)
  MESSAGE(NOTICE "Doxygen available, adding libbase documentation to 'doc' target.")

  # Add as dependency to an existing 'doc' target, or create it.
  IF(TARGET doc)
    ADD_DEPENDENCIES(doc libbase_doc)
  ELSE(TARGET doc)
    ADD_CUSTOM_TARGET(doc DEPENDS libbase_doc)
  ENDIF(TARGET doc)

ELSE(DOXYGEN_FOUND)
  MESSAGE(NOTICE "Doxygen not found. Not adding 'doc' target to generate API documentation.")
ENDIF(DOXYGEN_FOUND)

# Adds 'coverage' target, if configured. Needs 'gcovr'.
IF(config_COVERAGE)
  FIND_PROGRAM(GCOVR_FOUND gcovr OPTIONAL)
  IF(GCOVR_FOUND)
    ADD_COMPILE_OPTIONS(--coverage)
    ADD_LINK_OPTIONS(--coverage)

    MESSAGE(NOTICE "Coverage configured, adding 'coverage' target.")
    ADD_CUSTOM_TARGET(coverage
      COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} . --exclude _deps --print-summary --html-details --html-title "Unittest coverage" -o coverage.html)
  ELSE(GCOVR_FOUND)
    MESSAGE(WARNING "Coverage enabled, but 'govr' not found.")
  ENDIF(GCOVR_FOUND)
ENDIF()
