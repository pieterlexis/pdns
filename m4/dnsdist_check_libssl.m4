AC_DEFUN([DNSDIST_CHECK_LIBSSL], [
  AC_MSG_CHECKING([if OpenSSL libssl is available])
  PKG_CHECK_MODULES([LIBSSL], [libssl], [
    [HAVE_LIBSSL=1],
    AC_DEFINE([HAVE_LIBSSL], [1], [Define to 1 if you have OpenSSL libssl])
  ], [
      [HAVE_LIBSSL=0],
  ])
])
