localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebinfo

ebinfo_SOURCES = ebinfo.c
ebinfo_LDADD = $(LIBEBUTILS) $(LIBEB) $(ZLIBLIBS) $(INTLLIBS) $(ICONVLIBS)
ebinfo_DEPENDENCIES = $(LIBEBUTILS) $(LIBEB) $(ZLIBDEPS) $(INTLDEPS) \
	$(ICONVDEPS)

INCLUDES = -I../libebutils -I$(top_srcdir)/libebutils -I$(top_srcdir) \
	$(INTLINCS)
