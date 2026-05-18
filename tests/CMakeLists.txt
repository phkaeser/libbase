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
  libbase_test
  libbase_test.c)
target_link_libraries(
  libbase_test
  libbase)
# Refer to the source path for relative testdata.
target_compile_definitions(
  libbase_test PUBLIC BS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(
  libbase_test PUBLIC BS_TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")

add_executable(
  libbase_benchmark
  libbase_benchmark.c)
target_link_libraries(
  libbase_benchmark
  libbase)

add_executable(
  subprocess_test_hang
  subprocess_test_hang.c)
add_executable(
  subprocess_test_failure
  subprocess_test_failure.c)
add_executable(
  subprocess_test_sigpipe
  subprocess_test_sigpipe.c)
add_executable(
  subprocess_test_success
  subprocess_test_success.c)

add_executable(
  plist_test
  plist_test.c)
target_link_libraries(
  plist_test
  libbase
  libbase_plist)
target_include_directories(plist_test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/")
target_compile_definitions(
  plist_test PUBLIC TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

add_test(NAME libbase_test COMMAND libbase_test)
add_test(NAME plist_test COMMAND plist_test)

if(iwyu_path_and_options)
  set_target_properties(
    libbase_test PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
  set_target_properties(
    libbase_benchmark PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
  set_target_properties(
    plist_test PROPERTIES
    C_INCLUDE_WHAT_YOU_USE "${iwyu_path_and_options}")
endif()
