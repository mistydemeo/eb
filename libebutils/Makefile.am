noinst_LIBRARIES = libebutils.a
libebutils_a_SOURCES = ebutils.c getopt.c getopt1.c getumask.c makedir.c \
	samefile.c yesno.c
libebutils_a_LIBADD = @LIBOBJS@

EXTRA_DIST = getopt_long.pl ebutils.h.in
CLEANFILES = stamp-ebutils-h ebutils.h
BUILT_SOURCES = ebutils.h
noinst_HEADERS = getopt.h getumask.h makedir.h samefile.h yesno.h

INCLUDES = -I$(top_srcdir) @INTLINCS@

ebutils.h: stamp-ebutils-h
stamp-ebutils-h: ebutils.h.in Makefile
	rm -f ebutils.h.tmp
	sed -e 's;\@localedir\@;$(localedir);' \
	   $(srcdir)/ebutils.h.in > ebutils.h.tmp
	$(top_srcdir)/move-if-change ebutils.h.tmp ebutils.h
	@echo timestamp > stamp-ebutils-h
