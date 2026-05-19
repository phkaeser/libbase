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

add_executable(
  plist_parse
  plist_parse.c)
target_link_libraries(
  plist_parse
  PRIVATE
  libbase
  libbase_plist
  libbase_compiler_flags)
target_include_directories(plist_parse PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/")

if(iwyu_path_and_options)
  set_target_properties(
    plist_parse PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
endif()

# Add 'install' target, but only if we're the toplevel project.
if(CMAKE_PROJECT_NAME STREQUAL libbase)
  install(
    TARGETS plist_parse
    DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
