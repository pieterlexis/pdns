AC_DEFUN([PDNS_WITH_LIBYANG], [
  AC_MSG_CHECKING([whether we will be linking in libyang])
  HAVE_LIBYANG=0
  AC_ARG_WITH([libyang],
    AS_HELP_STRING([--with-libyang],[use libyang @<:@default=auto@:>@]),
    [with_libyang=$withval],
    [with_libyang=auto],
  )
  AC_MSG_RESULT([$with_libyang])

  AS_IF([test "x$with_libyang" != "xno"], [
    AS_IF([test "x$with_libyang" = "xyes" -o "x$with_libyang" = "xauto"], [
      PKG_CHECK_MODULES([LIBYANG], [libyang-cpp] , [
        [HAVE_LIBYANG=1]
        AC_DEFINE([HAVE_LIBYANG], [1], [Define to 1 if you have libyang])
      ], [ : ])
    ])
  ])
  AM_CONDITIONAL([HAVE_LIBYANG], [test "x$LIBYANG_LIBS" != "x"])
  AS_IF([test "x$with_libyang" = "xyes"], [
    AS_IF([test x"$LIBYANG_LIBS" = "x"], [
      AC_MSG_ERROR([libyang requested but libraries were not found])
    ])
  ])
])
