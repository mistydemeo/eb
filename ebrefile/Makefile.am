localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebrefile

ebrefile_SOURCES = ebrefile.c
ebrefile_LDADD = $(LIBEBUTILS) $(LIBEB) $(ZLIBLIBS) $(INTLLIBS)
ebrefile_DEPENDENCIES = $(LIBEBUTILS) $(LIBEB) $(ZLIBDEPS) $(INTLDEPS)

INCLUDES = -I../libebutils -I$(top_srcdir)/libebutils -I$(top_srcdir) \
	$(INTLINCS)
