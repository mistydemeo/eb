dnl *
dnl * Process this file with autoconf to produce a configure script.
dnl *
AC_INIT
AC_CONFIG_SRCDIR(eb/eb.c)
AC_PREREQ(2.52)
AM_INIT_AUTOMAKE(eb, 3.2.2)

dnl *
dnl * Library version info.
dnl *
LIBEB_VERSION_INFO=6:2:0
AC_SUBST(LIBEB_VERSION_INFO)

EB_VERSION_MAJOR=3
EB_VERSION_MINOR=2
AC_SUBST(EB_VERSION_MAJOR)
AC_SUBST(EB_VERSION_MINOR)

dnl *
dnl * Set mailing address.
dnl *
MAILING_ADDRESS='m-kasahr@sra.co.jp'
AC_SUBST(MAILING_ADDRESS)
AC_DEFINE_UNQUOTED(MAILING_ADDRESS, "$MAILING_ADDRESS", [Mailing address])

dnl *
dnl * Programs
dnl *
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PROG_INSTALL

AC_PATH_PROG(PERL, perl, no)
if test "$PERL" = no ; then
   AC_MSG_WARN(perl not found)
   PERL=/usr/bin/perl
fi

dnl * 
dnl * GNU gettext
dnl * 
eb_GNU_GETTEXT

dnl *
dnl * --enable-samples option.
dnl *
AC_ARG_ENABLE(samples,
[  --enable-samples        compile sample programs [default=no]],
[case "${enableval}" in
   yes) samples=true  ;;
   no)  samples=false ;;
   *)   AC_MSG_ERROR(invalid argument to --enable-samples) ;;
esac], samples=false)
AM_CONDITIONAL(ENABLE_SAMPLES, test ${samples} = true)

dnl *
dnl * --enable-pthread option.
dnl *
AC_ARG_ENABLE(pthread,
AC_HELP_STRING([--enable-pthread], [build pthread safe libraries [[no]]]),
[case "${enableval}" in
   yes) ENABLE_PTHREAD=yes  ;;
   no)  ENABLE_PTHREAD=no ;;
   *)   AC_MSG_ERROR(invalid argument to --enable-pthread) ;;
esac], ENABLE_PTHREAD=no)

dnl *
dnl * --with-pthread-cppflags option.
dnl *
AC_ARG_WITH(pthread-cppflags,
AC_HELP_STRING([--with-pthread-cppflags=FLAGS],
    [additional CPPFLAGS for pthread support]),
[PTHREAD_CPPFLAGS="${withval}"], [PTHREAD_CPPFLAGS=''])

dnl *
dnl * --with-pthread-cflags option.
dnl *
AC_ARG_WITH(pthread-cflags,
AC_HELP_STRING([--with-pthread-cflags=FLAGS], 
    [additional CFLAGS for pthread support]),
[PTHREAD_CFLAGS="${withval}"], [PTHREAD_CFLAGS=''])

dnl *
dnl * --with-pthread-ldflags option.
dnl *
AC_ARG_WITH(pthread-ldflags,
AC_HELP_STRING([--with-pthread-ldflags=FLAGS],
    [additional LDFLAGS for pthread support]),
[PTHREAD_LDFLAGS="${withval}"], [PTHREAD_LDFLAGS=''])

dnl *
dnl * Check for pthread.
dnl *
AC_MSG_CHECKING(for pthread)
if test $ENABLE_PTHREAD = no ; then
   try_pthread=no
else
   save_CPPFLAGS=$CPPFLAGS
   save_CFLAGS=$CFLAGS
   save_LDFLAGS=$LDFLAGS
   CPPFLAGS="$CPPFLAGS $PTHREAD_CPPFLAGS"
   CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
   LDFLAGS="$LDFLAGS $PTHREAD_LDFLAGS"
   AC_TRY_LINK([#include <pthread.h>],
   [static pthread_mutex_t m=PTHREAD_MUTEX_INITIALIZER; 
    pthread_mutex_lock(&m);
    return 0;], 
   try_pthread=yes, try_pthread=no)
   CPPFLAGS=$save_CPPFLAGS
   CFLAGS=$save_CFLAGS
   LDFLAGS=$save_LDFLAGS
fi
AC_MSG_RESULT($try_pthread)
if test $ENABLE_PTHREAD = yes ; then
   if test $try_pthread = no ; then
      AC_MSG_ERROR(pthread not available)
   fi
fi

if test $ENABLE_PTHREAD = yes ; then
   AC_DEFINE(ENABLE_PTHREAD, 1, [Define if pthread support is enabled.])
   CPPFLAGS="$CPPFLAGS $PTHREAD_CPPFLAGS"
   CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
   LDFLAGS="$LDFLAGS $PTHREAD_LDFLAGS"
   AC_DEFINE(EBCONF_ENABLE_PTHREAD, 1, 
      [Define if EB Library is built with pthread support.])
fi

dnl *
dnl * --with-included-zlib option.
dnl *
AC_ARG_WITH(included-zlib,
AC_HELP_STRING([--with-included-zlib], [use zlib included here]),
[case "${withval}" in
   yes|no) included_zlib=${withval} ;;
   *)   AC_MSG_ERROR(invalid argument to --with-included-zlib) ;;
esac], included_zlib=auto)

dnl *
dnl * --with-zlib-includes option.
dnl *
AC_ARG_WITH(zlib-includes,
AC_HELP_STRING([--with-zlib-includes=DIR], [zlib include files are in DIR]),
[z_includedir="${withval}"], [z_includedir=''])

if test "X$z_includedir" != X ; then
   ZLIBINCS="-I$z_includedir"
else
   ZLIBINCS=''
fi

dnl *
dnl * --with-zlib-libraries option.
dnl *
AC_ARG_WITH(zlib-libraries,
AC_HELP_STRING([--with-zlib-libraries=DIR], [zlib library files are in DIR]),
[z_libdir="${withval}"], [z_libdir=''])

if test "X$z_libdir" != X ; then
   ZLIBLIBS="-L$z_libdir -lz"
   ZLIBDEPS=''
else
   ZLIBLIBS='-lz'
   ZLIBDEPS=''
fi

dnl *
dnl * Check for zlib.
dnl *
AC_MSG_CHECKING(for included zlib)
if test $included_zlib = yes ; then
   try_zlib=yes
else
   save_CPPFLAGS=$CPPFLAGS
   save_LIBS=$LIBS
   CPPFLAGS="$CPPFLAGS $ZLIBINCS"
   LIBS="$LIBS $ZLIBLIBS"
   AC_TRY_LINK([#include <zlib.h>],
   [z_stream stream; inflate(&stream, Z_FINISH); return 0;], 
   try_zlib=no, try_zlib=yes)
   CPPFLAGS=$save_CPPFLAGS
   LIBS=$save_LIBS
   if test $included_zlib = auto ; then
      included_zlib=$try_zlib
   fi
fi
AC_MSG_RESULT($try_zlib)
if test $included_zlib = no ; then
   if test $try_zlib = yes ; then
      AC_MSG_ERROR(zlib not found)
   fi
fi
if test $included_zlib = yes ; then
   ZLIBINCS='-I$(top_srcdir)/zlib'
   ZLIBDEPS='$(top_builddir)/zlib/libz.la'
   ZLIBLIBS='$(top_builddir)/zlib/libz.la'
fi
AC_SUBST(ZLIBINCS)
AC_SUBST(ZLIBLIBS)
AC_SUBST(ZLIBDEPS)
AM_CONDITIONAL(INCLUDED_ZLIB, test $included_zlib = yes)

dnl * 
dnl * Subdirectories.
dnl *
AC_CONFIG_SUBDIRS(zlib)

dnl * 
dnl * Compiler Characteristics.
dnl * 
AC_C_CONST

dnl * 
dnl * Header Files.
dnl * 
AC_HEADER_STDC
AC_HEADER_STAT
AC_HEADER_TIME
AC_HEADER_DIRENT
AC_CHECK_HEADERS(direct.h fcntl.h limits.h memory.h pthread.h stdlib.h \
	string.h syslog.h unistd.h utime.h sys/utime.h)

dnl * 
dnl * Structures.
dnl * 
AC_STRUCT_UTIMBUF

dnl * 
dnl * Library Functions.
dnl * 
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(memcpy memmove getcwd strchr syslog utime vsyslog _getdcwd)
AC_REPLACE_FUNCS(memset strcasecmp strerror)

dnl * 
dnl * Typedefs.
dnl * 
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_MODE_T
AC_CHECK_TYPE(ssize_t, int)

AC_TYPE_SIGNAL
if test "$ac_cv_type_signal" = void; then
   AC_DEFINE(RETSIGTYPE_VOID, 1, [Define if the signal function returns void.])
fi

dnl *
dnl * System services
dnl *
AC_CYGWIN
AC_MINGW32
AC_EXEEXT
AC_OBJEXT
AM_CONDITIONAL(EXEEXT_EXE, test X$ac_exeext = Xyes)
if test X$ac_exeext = X.exe ; then
   AC_DEFINE(EXEEXT_EXE, 1, [Define if command names have the suffix \`.exe'])
fi

dnl *
dnl * Substitusion for eb.conf.
dnl *
if test $included_zlib = yes ; then
   EBCONF_ZLIBINCS='-I$(includedir)'
   EBCONF_ZLIBLIBS='-L$(libdir) -lz'
else
   EBCONF_ZLIBINCS=$ZLIBINCS
   EBCONF_ZLIBLIBS=$ZLIBLIBS
fi
AC_SUBST(EBCONF_ZLIBINCS)
AC_SUBST(EBCONF_ZLIBLIBS)

AC_SUBST(ENABLE_PTHREAD)
AC_SUBST(PTHREAD_CPPFLAGS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LDFLAGS)

EBCONF_INTLINCS=$INTLINCS
EBCONF_INTLLIBS=$INTLLIBS
AC_SUBST(EBCONF_INTLINCS)
AC_SUBST(EBCONF_INTLLIBS)

EBCONF_EBINCS='-I$(includedir)'
EBCONF_EBLIBS='-L$(libdir) -leb'
AC_SUBST(EBCONF_EBINCS)
AC_SUBST(EBCONF_EBLIBS)

dnl * 
dnl * Output Files.
dnl * 
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile eb/Makefile libebutils/Makefile ebappendix/Makefile
    ebfont/Makefile ebinfo/Makefile ebrefile/Makefile ebzip/Makefile
    doc/Makefile doc-ja/Makefile po-eb/Makefile po-ebutils/Makefile
    m4/Makefile samples/Makefile])
AC_OUTPUT
