# Macro to add for using GNU gettext.
# Ulrich Drepper <drepper@cygnus.com>, 1995.
# Modified by Motoyuki Kasahara, 2000, 01.
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU Public License
# but which still want to provide support for the GNU gettext functionality.
# Please note that the actual code is *not* freely available.

# serial 5

AC_DEFUN(eb_GNU_GETTEXT, [dnl
  INTLINCS=
  INTLDEPS=
  INTLLIBS=

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
  AC_ARG_ENABLE(nls,
    AC_HELP_STRING([--enable-nls], [Native Language Support [[yes]]]),
    ENABLE_NLS=$enableval, ENABLE_NLS=auto)

  dnl * 
  dnl * --with-gettext-includes option
  dnl * 
  AC_ARG_WITH(gettext-includes,
    AC_HELP_STRING([--with-gettext-includes=DIR],
      [gettext include files are in DIR]),
  [gettext_includes="-I${withval}"], [gettext_includes=''])

  dnl * 
  dnl * --with-gettext-libraries option
  dnl * 
  AC_ARG_WITH(gettext-libraries,
    AC_HELP_STRING([--with-gettext-libraries=DIR],
      [gettext library files are in DIR]),
  [gettext_libraries="-L${withval} -lintl"], [gettext_libraries=''])

  dnl * 
  dnl * --with-iconv-includes option
  dnl * 
  AC_ARG_WITH(iconv-includes,
    AC_HELP_STRING([--with-iconv-includes=DIR],
      [iconv include files are in DIR]),
  [iconv_includes="-I${withval}"], [iconv_includes=''])

  dnl * 
  dnl * --with-iconv-libraries option
  dnl * 
  AC_ARG_WITH(iconv-libraries,
    AC_HELP_STRING([--with-iconv-libraries=DIR],
      [iconv library files are in DIR]),
  [iconv_libraries="-L${withval} -liconv"], [iconv_libraries=''])

  dnl *
  dnl * Check iconv(), iconv.h and -liconv.
  dnl *
  ICONVINCS=
  ICONVLIBS=
  save_CPPFLAGS=$CPPFLAGS
  save_LIBS=$LIBS
  CPPFLAGS="$save_CPPFLAGS $iconv_includes"
  LIBS="$save_LIBS $iconv_libraries"
  AC_CHECK_LIB(iconv, iconv_open)
  AC_CHECK_LIB(iconv, libiconv_open)
  AC_CHECK_FUNCS(iconv_open libiconv_open locale_charset)
  AC_CHECK_HEADERS(iconv.h libcharset.h)
  if test "$ac_cv_func_iconv_open$ac_cv_func_libiconv_open" != nono; then
    ICONVINCS="$iconv_includes"
    ICONVLIBS="$iconv_libraries"
  else
    iconv_includes=
    iconv_libraries=
  fi
  CPPFLAGS=$save_CPPFLAGS
  LIBS=$save_LIBS
  AC_SUBST(ICONVINCS)
  AC_SUBST(ICONVLIBS)

  dnl *
  dnl * Check gettext().
  dnl * (Note that LANGUAGE has highest priority in GNU gettext).
  dnl * 
  INTLINCS=
  INTLLIBS=
  try_nls=no

  AC_MSG_CHECKING([for NLS support])

  if test $ENABLE_NLS != no; then
    rm -rf .locale
    mkdir .locale
    mkdir .locale/en
    mkdir .locale/en/LC_MESSAGES
    cp $srcdir/gttest.mo .locale/en/LC_MESSAGES/gttest.mo

    save_CPPFLAGS=$CPPFLAGS
    save_LIBS=$LIBS
    save_LANGUAGE=$LANGUAGE
    save_LC_ALL=$LC_ALL

    LANGUAGE=en_US
    LC_ALL=en_US
    export LANGUAGE LC_ALL

    dnl *
    dnl * Test 1: Try to link both libintl and libiconv.
    dnl *
    CPPFLAGS="$save_CPPFLAGS $gettext_includes"
    LIBS="$save_LIBS $gettext_libraries $iconv_libraries"
    AC_TRY_RUN([
#include <stdio.h>
#ifdef ENABLE_NLS
#undef ENABLE_NLS
#endif
#define ENABLE_NLS 1
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
    try_nls=yes, try_nls=no, try_nls=yes)

    if test "$try_nls" = yes; then
      INTLINCS="$gettext_includes"
      INTLLIBS="$gettext_libraries $iconv_libraries"
    fi

    dnl *
    dnl * Test 2: Try to link libintl.
    dnl * 
    if test "$try_nls" = no; then
      CPPFLAGS="$save_CPPFLAGS $gettext_includes"
      LIBS="$save_LIBS $gettext_libraries"
      AC_TRY_RUN([
#include <stdio.h>
#ifdef ENABLE_NLS
#undef ENABLE_NLS
#endif
#define ENABLE_NLS 1
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
      try_nls=yes, try_nls=no, try_nls=yes)

      if test "$try_nls" = yes; then
        INTLINCS="$gettext_includes"
        INTLLIBS="$gettext_libraries"
      fi
    fi

    dnl *
    dnl * Test 3: Try to link libiconv.
    dnl * 
    if test "$try_nls" = no; then
      CPPFLAGS="$save_CPPFLAGS"
      LIBS="$save_LIBS $iconv_libraries"
      AC_TRY_RUN([
#include <stdio.h>
#ifdef ENABLE_NLS
#undef ENABLE_NLS
#endif
#define ENABLE_NLS 1
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
      try_nls=yes, try_nls=no, try_nls=yes)

      if test "$try_nls" = yes; then
        INTLINCS=
        INTLLIBS="$iconv_libraries"
      fi
    fi

    dnl *
    dnl * Test 4: Try to link libc only.
    dnl * 
    if test "$try_nls" = no; then
      CPPFLAGS="$save_CPPFLAGS"
      LIBS="$save_LIBS"
      AC_TRY_RUN([
#include <stdio.h>
#ifdef ENABLE_NLS
#undef ENABLE_NLS
#endif
#define ENABLE_NLS 1
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
      try_nls=yes, try_nls=no, try_nls=yes)

      if test "$try_nls" = yes; then
        INTLINCS=
        INTLLIBS=
      fi
    fi

    rm -rf .locale

    CPPFLAGS=$save_CPPFLAGS
    LIBS=$save_LIBS
    LANGUAGE=$save_LANGUAGE
    LC_ALL=$save_LC_ALL
  fi

  if test $ENABLE_NLS = auto; then
    ENABLE_NLS=$try_nls
  fi

  AC_MSG_RESULT($try_nls)

  if test $ENABLE_NLS = yes; then
    if test $try_nls = no; then
      AC_MSG_ERROR(gettext not available)
    fi
  fi

  AC_SUBST(ENABLE_NLS)
  AC_SUBST(INTLINCS)
  AC_SUBST(INTLLIBS)
  localedir='$(datadir)/locale'
  AC_SUBST(localedir)
  if test $ENABLE_NLS = yes; then
    AC_DEFINE(ENABLE_NLS, 1, [Define if NLS is requested])
  fi

  dnl *
  dnl * Check msgfmt and xgettext commands.
  dnl * 
  AC_PATH_PROGS(MSGFMT, gmsgfmt msgfmt, :)
  AC_PATH_PROGS(XGETTEXT, gxgettext xgettext, :)
  MSGMERGE=msgmerge
  AC_SUBST(MSGMERGE)
])
