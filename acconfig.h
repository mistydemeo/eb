#ifndef CONFIG_H
#define CONFIG_H

#define EB_BUILD_LIBRARY

#define USE_FAKELOG

@TOP@

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef ssize_t

/* Define if the signal function returns void.  */
#undef RETSIGTYPE_VOID

/* Define if `struct utimbuf' is declared -- usually in <utime.h>.  */
#undef HAVE_STRUCT_UTIMBUF

/* The name of the package */
#undef PACKAGE

/* The the version number of the release that is being developed.  */
#undef VERSION

/* Mailing address */
#undef MAILING_ADDRESS

/* Define if compiled symbols have a leading underscore. */
#undef WITH_SYMBOL_UNDERSCORE

@BOTTOM@

#endif /* not CONFIG_H */
