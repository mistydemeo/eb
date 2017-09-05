# Macro to add for using GNU gettext.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
# Modified by Motoyuki Kasahara, 2000.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 5

AC_DEFUN(AM_GNU_GETTEXT_HACKED, [dnl
  INTLINCS=
  INTLDEPS=
  INTLLIBS=

  PO_SUBDIRS="$1"
  if test "X$PO_SUBDIRS" = X ; then
    PO_SUBDIRS=po
  fi

  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_PROG_LIBTOOL])
  AC_REQUIRE([AC_HEADER_STDC])
  AC_REQUIRE([AC_C_CONST])
  AC_REQUIRE([AC_C_INLINE])
  AC_REQUIRE([AC_TYPE_OFF_T])
  AC_REQUIRE([AC_TYPE_SIZE_T])
  AC_REQUIRE([AC_FUNC_ALLOCA])
  AC_REQUIRE([AC_FUNC_MMAP])

  AC_CHECK_HEADERS([argz.h limits.h locale.h nl_types.h malloc.h string.h \
unistd.h sys/param.h])
  AC_CHECK_FUNCS([getcwd munmap putenv setenv setlocale strchr strcasecmp \
strdup __argz_count __argz_stringify __argz_next])

  AM_LC_MESSAGES

  dnl * 
  dnl * --enable-nls option
  dnl * 
  AC_MSG_CHECKING([whether NLS is requested])
  dnl Default is enabled NLS
  AC_ARG_ENABLE(nls,
    [  --enable-nls           Native Language Support \[yes\]],
    ENABLE_NLS=$enableval, ENABLE_NLS=yes)
  AC_MSG_RESULT($ENABLE_NLS)
  AC_SUBST(ENABLE_NLS)

  dnl * 
  dnl * --with-gettext-includes option
  dnl * 
  AC_ARG_WITH(gettext-includes,
  [  --with-gettext-includes=DIR
                          gettext include files are in DIR],
  [gettext_includedir="${withval}"], [gettext_includedir=''])

  if test "X$gettext_includedir" != X ; then
    INTLINCS="-I$gettext_includedir"
  else
    INTLINCS=
  fi

  dnl * 
  dnl * --with-gettext-libraries option
  dnl * 
  AC_ARG_WITH(gettext-libraries,
  [  --with-gettext-libraries=DIR
                          gettext library files are in DIR],
  [gettext_libdir="${withval}"], [gettext_libdir=''])

  if test "X$gettext_libdir" != X ; then
    INTLLIBS="-L$gettext_libdir -lintl"
    INTLDEPS=
  fi

  if test "$ENABLE_NLS" = "yes"; then
    AC_DEFINE(ENABLE_NLS, 1, [Define if NLS is requeste])

    dnl * 
    dnl * --with-included-gettext option
    dnl *
    AC_MSG_CHECKING([whether included gettext is requested])
    AC_ARG_WITH(included-gettext,
      [  --with-included-gettext use the GNU gettext library included here],
      INCLUDED_GETTEXT=$withval, INCLUDED_GETTEXT=no)

    dnl *
    dnl * Check libintl library.
    dnl * (LANGUAGE has highest priority in GNU gettext).
    dnl * 
    if test "$INCLUDED_GETTEXT" = no; then
      save_CPPFLAGS=$CPPFLAGS
      save_LIBS=$LIBS
      save_LANGUAGE=$LANGUAGE

      CPPFLAGS="$CPPFLAGS $INTLINCS"
      LIBS="$LIBS $INTLLIBS"
      LANGUAGE=
      LC_ALL=en
      export LANGUAGE
      export LANGUAGE LC_ALL

      rm -rf .locale
      mkdir .locale
      mkdir .locale/en
      mkdir .locale/en/LC_MESSAGES
      cp $srcdir/gttest.mo .locale/en/LC_MESSAGES/gttest.mo
      AC_TRY_RUN([
#include <stdio.h>
#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif
#include <libintl.h>

int
main()
{
  const char *p;

#ifdef HAVE_SETLOCALE
  setlocale(LC_ALL, "");
#endif
  bindtextdomain("gttest", ".locale");
  textdomain("gttest");
  p = gettext("foo");
  if (*p == 'b' && *(p + 1) == 'a' && *(p + 2) == 'r' && *(p + 3) == '\0')
    return 0;
  return 1;
}
], 
      INCLUDED_GETTEXT=no,
      INCLUDED_GETTEXT=yes,
      INCLUDED_GETTEXT=no)
      rm -rf .locale

      CPPFLAGS=$save_CPPFLAGS
      LIBS=$save_LIBS
      LANGUAGE=$save_LANGUAGE
      LC_ALL=$save_LC_ALL
    fi

    if test "$INCLUDED_GETTEXT" = yes; then
      INTLINCS='-I$(top_srcdir)/intl'
      INTLDEPS='$(top_builddir)/intl/libintl.la'
      INTLLIBS=$INTLDEPS
    fi

    AC_MSG_RESULT($INCLUDED_GETTEXT)

    dnl *
    dnl * Check msgfmt and xgettext commands.
    dnl * 
    AC_PATH_PROGS(MSGFMT, gmsgfmt msgfmt, :)
    AC_PATH_PROGS(XGETTEXT, gxgettext xgettext, :)

    MSGMERGE=msgmerge
    AC_SUBST(MSGMERGE)
  fi

  AM_CONDITIONAL(ENABLE_NLS, test $ENABLE_NLS = yes)
  AM_CONDITIONAL(INCLUDED_GETTEXT, test $INCLUDED_GETTEXT = yes)

  AC_SUBST(INTLINCS)
  AC_SUBST(INTLDEPS)
  AC_SUBST(INTLLIBS)

  localedir='$(datadir)/locale'
  AC_SUBST(localedir)
])
