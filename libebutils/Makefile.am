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

### *.o dependencies ###
ebutils.o: ebutils.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/zio.h \
 ../eb/error.h ebutils.h
getopt.o: getopt.c getopt.h
getopt1.o: getopt1.c getopt.h
getumask.o: getumask.c ../config.h
makedir.o: makedir.c ../config.h
memset.o: memset.c ../config.h
samefile.o: samefile.c ../config.h
strcasecmp.o: strcasecmp.c ../config.h
strerror.o: strerror.c ../config.h
yesno.o: yesno.c ../config.h
