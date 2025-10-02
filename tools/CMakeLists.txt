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

ADD_EXECUTABLE(plist_parse plist_parse.c)
TARGET_LINK_LIBRARIES(plist_parse libbase libbase_plist)
TARGET_INCLUDE_DIRECTORIES(plist_parse PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/)

IF(iwyu_path_and_options)
  SET_TARGET_PROPERTIES(
    plist_parse PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
ENDIF(iwyu_path_and_options)

# Add 'install' target, but only if we're the toplevel project.
IF(CMAKE_PROJECT_NAME STREQUAL libbase)
  INSTALL(
    TARGETS plist_parse
    DESTINATION ${CMAKE_INSTALL_BINDIR})
ENDIF()
