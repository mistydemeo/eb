localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebstopcode

ebstopcode_SOURCES = ebstopcode.c
ebstopcode_LDADD = $(LIBEBUTILS) $(LIBEB) $(ZLIBLIBS) $(INTLLIBS)
ebstopcode_DEPENDENCIES = $(LIBEBUTILS) $(LIBEB) $(ZLIBDEPS) $(INTLDEPS)

INCLUDES = -I../libebutils -I$(top_srcdir)/libebutils -I$(top_srcdir) \
	$(INTLINCS)
