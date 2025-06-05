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
#
# Suggested configuration:
#   cmake -B build -Dconfig_WERROR=ON -Dconfig_DEBUG=ON
CMAKE_MINIMUM_REQUIRED(VERSION 3.25)
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
INCLUDE(CTest)
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
OPTION(config_WERROR "Make all compiler warnings into errors." OFF)

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
  ADD_COMPILE_OPTIONS(-Wall -Wextra)

  IF(config_WERROR)
    ADD_COMPILE_OPTIONS(-Werror)
  ENDIF(config_WERROR)

  # CMake provides absolute paths to GCC, hence the __FILE__ macro includes the
  # full path. This option resets it to a path relative to project source.
  ADD_COMPILE_OPTIONS(-fmacro-prefix-map=${PROJECT_SOURCE_DIR}/src=.)

  # Target system.
  IF(LINUX)
    ADD_COMPILE_OPTIONS(-D__LIBBASE_LINUX)
  ENDIF(LINUX)

  # Run iwyu when available.
  FIND_PROGRAM(iwyu_executable NAMES include-what-you-use iwyu OPTIONAL)
  IF(iwyu_executable)
    SET(iwyu_path_and_options
      ${iwyu_executable}
      -Xiwyu --error=1 -Xiwyu --transitive_includes_only -Xiwyu --mapping_file=${PROJECT_SOURCE_DIR}/iwyu-mappings.imp)
  ENDIF(iwyu_executable)
ENDIF()
SET(CMAKE_C_STANDARD 11)

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(CAIRO REQUIRED IMPORTED_TARGET cairo>=1.16.0)

# Create pkg-config file from the template.
CONFIGURE_FILE(libbase.pc.in ${CMAKE_BINARY_DIR}/libbase.pc @ONLY)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(src/plist)

# Add 'install' target, but only if we're the toplevel project.
IF(CMAKE_PROJECT_NAME STREQUAL libbase)
  INSTALL(
    TARGETS libbase
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libbase")
  INSTALL(
    TARGETS libbase_plist
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libbase")

  INSTALL(
    FILES ${CMAKE_BINARY_DIR}/libbase.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )
ENDIF()

# Add test target, but only if we're the toplevel project.
IF(CMAKE_PROJECT_NAME STREQUAL libbase)
  ADD_SUBDIRECTORY(tests)
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
