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

ADD_EXECUTABLE(libbase_test libbase_test.c)
TARGET_LINK_LIBRARIES(libbase_test base)
# Refer to the source path for relative testdata.
TARGET_COMPILE_DEFINITIONS(
  libbase_test PUBLIC BS_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

TARGET_COMPILE_DEFINITIONS(
  libbase_test PUBLIC BS_TEST_BINARY_DIR="${CMAKE_CURRENT_BINARY_DIR}")

ADD_EXECUTABLE(libbase_benchmark libbase_benchmark.c)
TARGET_LINK_LIBRARIES(libbase_benchmark base)

ADD_EXECUTABLE(subprocess_test_hang subprocess_test_hang.c)
ADD_EXECUTABLE(subprocess_test_failure subprocess_test_failure.c)
ADD_EXECUTABLE(subprocess_test_sigpipe subprocess_test_sigpipe.c)
ADD_EXECUTABLE(subprocess_test_success subprocess_test_success.c)
