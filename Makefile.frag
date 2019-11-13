ilimit-test-coverage:
	CCACHE_DISABLE=1 EXTRA_CFLAGS="-fprofile-arcs -ftest-coverage" TEST_PHP_ARGS="-q" $(MAKE) clean test

ilimit-test-coverage-lcov: ilimit-test-coverage
	lcov -c --directory $(top_srcdir)/src/.libs --output-file $(top_srcdir)/coverage.info

ilimit-test-coverage-html: ilimit-test-coverage-lcov
	genhtml $(top_srcdir)/coverage.info --output-directory=$(top_srcdir)/html

ilimit-test-coverage-travis:
	CCACHE_DISABLE=1 EXTRA_CFLAGS="-fprofile-arcs -ftest-coverage" $(MAKE)
