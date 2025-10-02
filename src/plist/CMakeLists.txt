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

FIND_PACKAGE(FLEX 2.6 REQUIRED)
FLEX_TARGET(
  analyzer
  analyzer.l
  ${CMAKE_CURRENT_BINARY_DIR}/analyzer.c
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/analyzer.h)

FIND_PACKAGE(BISON 3.0 REQUIRED)
BISON_TARGET(
  grammar
  grammar.y
  ${CMAKE_CURRENT_BINARY_DIR}/grammar.c
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/grammar.h)

ADD_FLEX_BISON_DEPENDENCY(analyzer grammar)

SET(PUBLIC_HEADER_FILES
  ${PROJECT_SOURCE_DIR}/include/libbase/plist/decode.h
  ${PROJECT_SOURCE_DIR}/include/libbase/plist/model.h
  ${PROJECT_SOURCE_DIR}/include/libbase/plist/parse.h)

ADD_LIBRARY(libbase_plist STATIC)
TARGET_SOURCES(libbase_plist PRIVATE
  decode.c
  model.c
  parse.c
  ${CMAKE_CURRENT_BINARY_DIR}/analyzer.c
  ${CMAKE_CURRENT_BINARY_DIR}/grammar.c)
TARGET_INCLUDE_DIRECTORIES(
  libbase_plist
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC
  ${CMAKE_CURRENT_BINARY_DIR}/)

# TODO(kaeser@gubbe.ch):Remove, once updating post bison 3.8.2 (Aug 2022).
# See: https://github.com/phkaeser/wlmaker/issues/53
TARGET_COMPILE_OPTIONS(libbase_plist PRIVATE -Wno-unused-but-set-variable)
TARGET_LINK_LIBRARIES(libbase_plist libbase)
SET_TARGET_PROPERTIES(
  libbase_plist
  PROPERTIES VERSION 1.0
  PUBLIC_HEADER "${PUBLIC_HEADER_FILES}")
IF(iwyu_path_and_options)
  SET_TARGET_PROPERTIES(
    libbase_plist PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
ENDIF(iwyu_path_and_options)

# Add 'install' target, but only if `libbase` is the toplevel project.
IF(CMAKE_PROJECT_NAME STREQUAL libbase)
  INSTALL(
    TARGETS libbase_plist
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libbase/plist")
ENDIF()
