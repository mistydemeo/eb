lib_LTLIBRARIES = libeb.la
pkginclude_HEADERS = appendix.h binary.h defs.h eb.h error.h font.h \
	text.h zio.h
noinst_HEADERS = ebconfig.h

INCLUDES = -DEB_BUILD_LIBRARY $(INTLINCS) $(ZLIBINCS)

libeb_la_SOURCES = appendix.c appsub.c bcd.c binary.c bitmap.c book.c \
   copyright.c eb.c endword.c error.c exactword.c filename.c font.c \
   hook.c jacode.c keyword.c lock.c match.c menu.c memmove.c multi.c \
   narwalt.c narwfont.c search.c setword.c stopcode.c strcasecmp.c \
   subbook.c text.c widealt.c widefont.c word.c zio.c
libeb_la_LDFLAGS = -version-info @LIBEB_VERSION_INFO@

EXTRA_DIST = internal.h.in
CLEANFILES = stamp-internal-h stamp-widealt-h stamp-widefont-h \
   internal.h widealt.c widefont.c
BUILT_SOURCES = internal.h widealt.c widefont.c

internal.h: stamp-internal-h
stamp-internal-h: internal.h.in Makefile
	rm -f internal.h.tmp
	sed -e 's;\@localedir\@;$(localedir);' \
	   $(srcdir)/internal.h.in > internal.h.tmp
	$(top_srcdir)/move-if-change internal.h.tmp internal.h
	@echo timestamp > stamp-internal-h

widealt.c: stamp-widealt-h
stamp-widealt-h: narwalt.c Makefile
	rm -f widealt.c.tmp
	sed -e s/narrow/wide/g -e s/Narrow/Wide/g -e s/NARROW/WIDE/g \
	   $(srcdir)/narwalt.c > widealt.c.tmp
	$(top_srcdir)/move-if-change widealt.c.tmp widealt.c
	@echo timestamp > stamp-widealt-h

widefont.c: stamp-widefont-h
stamp-widefont-h: narwfont.c Makefile
	rm -f widefont.c.tmp
	sed -e s/narrow/wide/g -e s/Narrow/Wide/g -e s/NARROW/WIDE/g \
	   $(srcdir)/narwfont.c > widefont.c.tmp
	$(top_srcdir)/move-if-change widefont.c.tmp widefont.c
	@echo timestamp > stamp-widefont-h
