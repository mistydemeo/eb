@SET_MAKE@

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebfont

ebfont_SOURCES = ebfont.c
ebfont_LDADD = $(LIBEB) $(LIBEBUTILS) @LDADD_FOR_LIBZ@
ebfont_DEPENDENCIES = $(LIBEB) $(LIBEBUTILS) @DEPENDENCIES_FOR_LIBZ@

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libebutils

### *.o dependencies ###
ebfont.o: ebfont.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/error.h \
 ../eb/font.h ../libebutils/fakelog.h ../libebutils/getopt.h \
 ../libebutils/getumask.h ../libebutils/makedir.h
