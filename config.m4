dnl config.m4 for extension ilimit

PHP_ARG_ENABLE([ilimit],
  [whether to enable ilimit support],
  [AS_HELP_STRING([--enable-ilimit],
    [Enable ilimit support])],
  [no])

PHP_ARG_ENABLE([ilimit-coverage],
  [whether to enable ilimit coverage support],
  [AS_HELP_STRING([--enable-ilimit-coverage],
    [Enable ilimit coverage support])],
  [no])

if test "$PHP_ILIMIT" != "no"; then
  AC_MSG_CHECKING([for NTS])
  if test "$PHP_THREAD_SAFETY" != "no"; then
    AC_MSG_ERROR([ilimit requires NTS, please use PHP with ZTS disabled])
  else
    AC_MSG_RESULT([ok])
  fi

  PHP_ADD_LIBRARY(pthread,, ILIMIT_SHARED_LIBADD)

  PHP_NEW_EXTENSION(ilimit, php_ilimit.c src/ilimit.c, $ext_shared)

  PHP_ADD_BUILD_DIR($ext_builddir/src, 1)
  PHP_ADD_INCLUDE($ext_srcdir)

  AC_MSG_CHECKING([ilimit coverage])
  if test "$PHP_ILIMIT_COVERAGE" != "no"; then
    AC_MSG_RESULT([enabled])

    PHP_ADD_MAKEFILE_FRAGMENT
  else
    AC_MSG_RESULT([disabled])
  fi


  PHP_SUBST(ILIMIT_SHARED_LIBADD)
fi
