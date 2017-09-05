@SET_MAKE@

localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebinfo

ebinfo_SOURCES = ebinfo.c
ebinfo_LDADD = $(LIBEB) $(LIBEBUTILS) $(ZLIBLIBS) $(INTLLIBS)
ebinfo_DEPENDENCIES = $(LIBEB) $(LIBEBUTILS) $(ZLIBDEPS) $(INTLDEPS)

INCLUDES = -I$(top_srcdir) -I../libebutils -I$(top_srcdir)/libebutils \
   $(INTLINCS)

### *.o dependencies ###
ebinfo.o: ebinfo.c ../config.h ../eb/eb.h ../eb/error.h ../eb/font.h \
 ../libebutils/fakelog.h ../libebutils/getopt.h \
 ../libebutils/ebutils.h
