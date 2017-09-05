dnl *
dnl * Make ready to link EB Library 3.x.
dnl *
dnl * Copyright (c) 2000, 01
dnl *    Motoyuki Kasahara
dnl *
dnl * This program is free software; you can redistribute it and/or modify
dnl * it under the terms of the GNU General Public License as published by
dnl * the Free Software Foundation; either version 2, or (at your option)
dnl * any later version.
dnl *
dnl * This program is distributed in the hope that it will be useful,
dnl * but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl * GNU General Public License for more details.
dnl *

AC_DEFUN(eb_LIB_EB3,
[dnl
dnl *
dnl * Requirements.
dnl *
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_LIBTOOL])
AC_REQUIRE([AC_C_CONST])
AC_REQUIRE([AC_TYPE_OFF_T])
AC_REQUIRE([AC_TYPE_SIZE_T])
AC_REQUIRE([AC_TYPE_SSIZE_T])
AC_REQUIRE([AC_HEADER_TIME])

dnl *
dnl * --with-eb-conf option.
dnl *
AC_ARG_WITH(eb-conf,
AC_HELP_STRING([--with-eb-conf=FILE],
    [eb.conf file is FILE [[SYSCONFDIR/eb.conf]]]),
[ebconf="${withval}"], [ebconf=$sysconfdir/eb.conf])
if test X$prefix = XNONE ; then
   PREFIX=$ac_default_prefix
else
   PREFIX=$prefix
fi
ebconf=`echo X$ebconf | sed -e 's/^X//' -e 's;\${prefix};'"$PREFIX;g" \
   -e 's;\$(prefix);'"$PREFIX;g"`

dnl *
dnl * Read eb.conf
dnl *
AC_MSG_CHECKING(for eb.conf)
AC_MSG_RESULT($ebconf)
if test -f ${ebconf} ; then
   . ${ebconf}
else
   AC_MSG_ERROR($ebconf not found)
fi

if test X$EBCONF_ENABLE_PTHREAD = Xyes ; then
   AC_DEFINE(EBCONF_ENABLE_PTHREAD, 1,
      [Define if EB Library supports pthread.])
fi
if test X$EBCONF_ENABLE_NLS = Xyes ; then
   AC_DEFINE(EBCONF_ENABLE_NLS, 1,
      [Define if EB Library supports native language.])
fi

AC_SUBST(EBCONF_EBINCS)
AC_SUBST(EBCONF_EBLIBS)
AC_SUBST(EBCONF_ZLIBINCS)
AC_SUBST(EBCONF_ZLIBLIBS)
AC_SUBST(EBCONF_PTHREAD_CPPFLAGS)
AC_SUBST(EBCONF_PTHREAD_CFLAGS)
AC_SUBST(EBCONF_PTHREAD_LDFLAGS)
AC_SUBST(EBCONF_INTLINCS)
AC_SUBST(EBCONF_INTLLIBS)

dnl *
dnl * Check for EB Library.
dnl *
AC_MSG_CHECKING(for EB Library)
save_CPPFLAGS=$CPPFLAGS
save_CFLAGS=$CFLAGS
save_LDFLAGS=$LDFLAGS
save_LIBS=$LIBS
CPPFLAGS="$CPPFLAGS $EBCONF_PTHREAD_CPPFLAGS $EBCONF_EBINCS $EBCONF_ZLIBINCS $EBCONF_INTLINCS"
CFLAGS="$CFLAGS $EBCONF_PTHREAD_CFLAGS"
LDFLAGS="$LDFAGS $EBCONF_PTHREAD_LDFLAGS"
LIBS="$LIBS $EBCONF_EBLIBS $EBCONF_ZLIBLIBS $EBCONF_INTLLIBS"
AC_TRY_LINK([#include <eb/eb.h>],
[eb_initialize_library(); return 0;],
try_eb=yes, try_eb=no)
CPPFLAGS=$save_CPPFLAGS
CFLAGS=$save_CFLAGS
LDFLAGS=$save_LDFLAGS
LIBS=$save_LIBS
AC_MSG_RESULT($try_eb)
if test ${try_eb} != yes ; then
   AC_MSG_ERROR(EB Library not available)
fi
])
