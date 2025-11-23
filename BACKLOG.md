# libbase idea/feature backlog

## Overall

* Split non-core functionality out of libbase/libbase.h and require explicit
  includes. While doing so, benchmark compilation time.

## bs_file

* Merge write_buffer and read_buffer with bs_dynbuf.

## bs_test

* When skipping tests, don't clutter the output with the full skipped test.
* Store the failure position for failed tests, include in test summary.
* Eliminate bs_test().
* Make it easy to run a single case, or a single set.
* Use bs_test_case_init(), bs_test_case_fini() for the overloaded prepare/report.
