localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebinfo

ebinfo_SOURCES = ebinfo.c
ebinfo_LDADD = $(LIBEBUTILS) $(LIBEB) $(ZLIBLIBS) $(INTLLIBS)
ebinfo_DEPENDENCIES = $(LIBEBUTILS) $(LIBEB) $(ZLIBDEPS) $(INTLDEPS)

INCLUDES = -I../libebutils -I$(top_srcdir)/libebutils -I$(top_srcdir) \
	$(INTLINCS)
