# Copyright 2024 Google LLC
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

find_package(FLEX 2.6 REQUIRED)
flex_target(
  analyzer
  analyzer.l
  "${CMAKE_CURRENT_BINARY_DIR}/analyzer.c"
  DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/analyzer.h")

find_package(BISON 3.0 REQUIRED)
bison_target(
  grammar
  grammar.y
  "${CMAKE_CURRENT_BINARY_DIR}/grammar.c"
  DEFINES_FILE "${CMAKE_CURRENT_BINARY_DIR}/grammar.h")

add_flex_bison_dependency(analyzer grammar)

set(public_header_files
  "${PROJECT_SOURCE_DIR}/include/libbase/plist/decode.h"
  "${PROJECT_SOURCE_DIR}/include/libbase/plist/model.h"
  "${PROJECT_SOURCE_DIR}/include/libbase/plist/parse.h")

add_library(libbase_plist STATIC)
target_sources(libbase_plist PRIVATE
  decode.c
  model.c
  parse.c
  "${CMAKE_CURRENT_BINARY_DIR}/analyzer.c"
  "${CMAKE_CURRENT_BINARY_DIR}/grammar.c")
target_include_directories(
  libbase_plist
  PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  PUBLIC
  "${CMAKE_CURRENT_BINARY_DIR}/")

# TODO(kaeser@gubbe.ch):Remove, once updating post bison 3.8.2 (Aug 2022).
# See: https://github.com/phkaeser/wlmaker/issues/53
target_compile_options(libbase_plist PRIVATE -Wno-unused-but-set-variable)
target_link_libraries(libbase_plist PRIVATE libbase libbase_compiler_flags)
set_target_properties(
  libbase_plist
  PROPERTIES VERSION 1.0
  PUBLIC_HEADER "${public_header_files}")
if(iwyu_path_and_options)
  set_target_properties(
    libbase_plist PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
endif()

# Add 'install' target, but only if `libbase` is the toplevel project.
if(CMAKE_PROJECT_NAME STREQUAL libbase)
  install(
    TARGETS libbase_plist
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libbase/plist")
endif()
