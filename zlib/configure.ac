dnl *
dnl * Process this file with autoconf to produce a configure script.
dnl *
AC_INIT(zlib, 1.1.4-lt)
AC_CONFIG_SRCDIR(zlib.h)
AC_PREREQ(2.52)
AM_INIT_AUTOMAKE

dnl *
dnl * Library version info.
dnl *
dnl LIBZ_VERSION_INFO=1:2:0
dnl AC_SUBST(LIBZ_VERSION_INFO)

dnl *
dnl * Programs.
dnl *
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_RANLIB
AC_PROG_LN_S
AC_PROG_INSTALL

dnl * 
dnl * Compiler Characteristics.
dnl * 
AC_C_CONST

dnl * 
dnl * Header Files.
dnl * 
AC_HEADER_STDC
AC_CHECK_HEADERS(string.h unistd.h string.h stddef.h stdlib.h errno.h)

dnl * 
dnl * Library Functions.
dnl * 
AC_CHECK_FUNCS(memcpy strerror snprintf vsprintf vsnprintf)

dnl * 
dnl * Typedefs.
dnl * 
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

dnl * 
dnl * Output Files.
dnl * 
AC_CONFIG_FILES(Makefile)
AC_OUTPUT
