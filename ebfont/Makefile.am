@SET_MAKE@

localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebfont

ebfont_SOURCES = ebfont.c
ebfont_LDADD = $(LIBEB) $(LIBEBUTILS) $(ZLIBLIBS) $(INTLLIBS)
ebfont_DEPENDENCIES = $(LIBEB) $(LIBEBUTILS) $(ZLIBDEPS) $(INTLDEPS)

INCLUDES = -I$(top_srcdir) -I../libebutils -I$(top_srcdir)/libebutils \
   $(INTLINCS)

### *.o dependencies ###
ebfont.o: ebfont.c ../config.h ../eb/eb.h ../eb/error.h ../eb/font.h \
 ../libebutils/fakelog.h ../libebutils/getopt.h \
 ../libebutils/getumask.h ../libebutils/makedir.h \
 ../libebutils/ebutils.h
