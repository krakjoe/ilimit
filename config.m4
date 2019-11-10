dnl config.m4 for extension ilimit

PHP_ARG_ENABLE([ilimit],
  [whether to enable ilimit support],
  [AS_HELP_STRING([--enable-ilimit],
    [Enable ilimit support])],
  [no])

if test "$PHP_ILIMIT" != "no"; then
  PHP_NEW_EXTENSION(ilimit, ilimit.c, $ext_shared,, "-DZEND_ENABLE_STATIC_TSRMLS_CACHE=1")
fi
