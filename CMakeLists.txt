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
#   cmake -DCMAKE_INSTALL_PREFIX:PATH=${HOME}/.local -Dconfig_WERROR=ON -Dconfig_DEBUG=ON -DIWYU_MODE=FAIL -B build
cmake_minimum_required(VERSION 3.25)
project(libbase VERSION 1.0
  DESCRIPTION "A small standard library for C"
  LANGUAGES C)

# Requires out-of-source builds.
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" location_path)
if(EXISTS "${location_path}")
  message(
    FATAL_ERROR
    "You cannot build into a source directory (containing a CMakeLists.txt file).\n"
    "Please make a build subdirectory, for example:\n"
    "cmake -B build\n"
    "(cd build && make)")
endif()

# Defaults to /usr/local/lib for installation.
include(CTest)
include(GNUInstallDirs)
include(FindCurses)

if(NOT CURSES_FOUND)
  message(
    FATAL_ERROR
    "Failed to find curses nor ncurses file and library.")
endif()

# Configuration. Remove CMakeCache.txt to rerun...
option(config_DEBUG "Include debugging information" ON)
option(config_OPTIM "Optimizations" OFF)
option(config_COVERAGE "Build with test coverage" OFF)
option(config_WERROR "Make all compiler warnings into errors." OFF)

set(IWYU_MODE OFF CACHE STRING "Whether to run `iwyu`. OFF, WARN or FAIL.")
set_property(CACHE IWYU_MODE PROPERTY STRINGS "OFF" "WARN" "FAIL")

# Toplevel compile options, for Clang and GCC.
add_library(libbase_compiler_flags INTERFACE)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  if(config_DEBUG)
    target_compile_options(libbase_compiler_flags INTERFACE -ggdb -DDEBUG)
  endif()

  if(config_OPTIM)
    target_compile_options(libbase_compiler_flags INTERFACE -O2)
  else()
    target_compile_options(libbase_compiler_flags INTERFACE -O0)
  endif()
  target_compile_options(libbase_compiler_flags INTERFACE -Wall -Wextra)

  if(config_WERROR)
    target_compile_options(libbase_compiler_flags INTERFACE -Werror)
  endif()

  # CMake provides absolute paths to GCC, hence the __FILE__ macro includes the
  # full path. This option resets it to a path relative to project source.
  target_compile_options(libbase_compiler_flags INTERFACE -fmacro-prefix-map=${PROJECT_SOURCE_DIR}/src=.)

  # Target system.
  if(LINUX)
    target_compile_options(libbase_compiler_flags INTERFACE -D__LIBBASE_LINUX)
  endif()

  # Run iwyu when available.
  if((IWYU_MODE STREQUAL "WARN") OR (IWYU_MODE STREQUAL "FAIL"))
    find_program(iwyu_executable NAMES include-what-you-use iwyu REQUIRED)

    if(IWYU_MODE STREQUAL "FAIL")
      set(iwyu_error "1")
    else()
      set(iwyu_error "0")
    endif()

    if(iwyu_executable)
      set(iwyu_path_and_options
        ${iwyu_executable}
        -Xiwyu --error=${iwyu_error} -Xiwyu --transitive_includes_only -Xiwyu --mapping_file=${PROJECT_SOURCE_DIR}/iwyu-mappings.imp)
    endif()

    message(STATUS "Configured iwyu (${iwyu_executable}) to run in mode \"${IWYU_MODE}\"")
  elseif(IWYU_MODE STREQUAL "OFF")
    # Permitted value, we don't set iwyu_path_and_options.
  else()
    message(FATAL_ERROR
      "Invalid value for IWYU_MODE: \"${IWYU_MODE}\". Must be OFF, WARN or FAIL")
  endif()
endif()
set(CMAKE_C_STANDARD 11)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo>=1.16.0)

# Create pkg-config file from the template.
configure_file(libbase.pc.in ${CMAKE_BINARY_DIR}/libbase.pc @ONLY)

add_subdirectory(src)
add_subdirectory(src/plist)
add_subdirectory(tools)

# Add 'install' target, but only if we're the toplevel project.
if(CMAKE_PROJECT_NAME STREQUAL libbase)
  install(
    FILES ${CMAKE_BINARY_DIR}/libbase.pc
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
  )
endif()

# Add test target, but only if we're the toplevel project.
if(CMAKE_PROJECT_NAME STREQUAL libbase)
  add_subdirectory(tests)
endif()

# Adds 'doc' target, if doxygen is installed.
find_package(Doxygen)
if(DOXYGEN_FOUND)
  # Set input and output files.
  set(doxygen_in ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
  set(doxygen_out ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

  # Configure the file.
  configure_file(${doxygen_in} ${doxygen_out} @ONLY)
  add_custom_target(
    libbase_doc
    COMMAND ${DOXYGEN_EXECUTABLE} ${doxygen_out}
    DEPENDS ${doxygen_out}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Generating API documentation with Doxygen."
    VERBATIM)
  message(NOTICE "Doxygen available, adding libbase documentation to 'doc' target.")

  # Add as dependency to an existing 'doc' target, or create it.
  if(TARGET doc)
    add_dependencies(doc libbase_doc)
  else()
    add_custom_target(doc DEPENDS libbase_doc)
  endif()

else()
  message(NOTICE "Doxygen not found. Not adding 'doc' target to generate API documentation.")
endif()

# Adds 'coverage' target, if configured. Needs 'gcovr'.
if(config_COVERAGE)
  find_program(GCOVR_FOUND gcovr OPTIONAL)
  if(GCOVR_FOUND)
    target_compile_options(libbase_compiler_flags INTERFACE --coverage)
    target_link_options(libbase_compiler_flags INTERFACE --coverage)

    message(NOTICE "Coverage configured, adding 'coverage' target.")
    add_custom_target(coverage
      COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} . --exclude _deps --print-summary --html-details --html-title "Unittest coverage" -o coverage.html)
  else()
    message(WARNING "Coverage enabled, but 'govr' not found.")
  endif()
endif()
