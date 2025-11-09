# libbase idea/feature backlog

## Overall

* Split non-core functionality out of libbase/libbase.h and require explicit
  includes. While doing so, benchmark compilation time.

## bs_file

* Merge write_buffer and read_buffer with bs_dynbuf.

## bs_test

* Expand test method to create a test directory from within test, including
  automatic rmdir when the test succeeds.
* When skipping tests, don't clutter the output with the full skipped test.
* Store the failure position for failed tests, include in test summary.
* Look into providing temporary file (names), so that the PNG verification
  tests don't keep re-using the same file.
