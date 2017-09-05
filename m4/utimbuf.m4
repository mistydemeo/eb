#
# Check for struct utimbuf (taken from GNU fileutils 3.16).
#
AC_DEFUN(AC_STRUCT_UTIMBUF,
[AC_HEADER_TIME
AC_CHECK_HEADERS(utime.h)
AC_CACHE_CHECK(for struct utimbuf, ax_cv_have_struct_utimbuf,
[AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif], [static struct utimbuf x; x.actime = x.modtime;
], [ax_cv_have_struct_utimbuf=yes], [ax_cv_have_struct_utimbuf=no])])
if test $ax_cv_have_struct_utimbuf = yes; then
   AC_DEFINE(HAVE_STRUCT_UTIMBUF, 1,
[Define if \`struct utimbuf' is declared -- usually in <utime.h>.])
fi
])
