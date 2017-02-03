AC_DEFUN([DNSDIST_CHECK_S2N], [
  AC_MSG_CHECKING([if s2n is available])
  AC_CHECK_HEADERS([s2n.h], s2n_headers=yes, s2n_headers=no)
  AM_CONDITIONAL([HAVE_S2N], [test x"$s2n_headers" = "xyes" ])
  AS_IF([test x"$s2n_headers" = "xyes" ],
    [ AC_DEFINE([HAVE_S2N], [1], [Define to 1 if you have s2n]) ],
  )
])
