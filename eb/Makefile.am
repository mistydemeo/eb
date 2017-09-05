lib_LTLIBRARIES = libeb.la
pkginclude_HEADERS = appendix.h binary.h defs.h eb.h error.h font.h \
	text.h zio.h
noinst_HEADERS = build-pre.h

INCLUDES = -DEB_BUILD_LIBRARY $(INTLINCS) $(ZLIBINCS)

libeb_la_SOURCES = appendix.c appsub.c bcd.c binary.c bitmap.c book.c \
	copyright.c eb.c endword.c error.c exactword.c filename.c font.c \
	hook.c jacode.c keyword.c lock.c log.c match.c menu.c memmove.c \
	multi.c narwalt.c narwfont.c search.c setword.c stopcode.c \
	strcasecmp.c subbook.c text.c widealt.c widefont.c word.c zio.c
libeb_la_LDFLAGS = -version-info @LIBEB_VERSION_INFO@

EXTRA_DIST = build-post.h.in
CLEANFILES = stamp-build-post-h stamp-widealt-h stamp-widefont-h \
	build-post.h widealt.c widefont.c
BUILT_SOURCES = build-post.h widealt.c widefont.c

build-post.h: stamp-build-post-h
stamp-build-post-h: build-post.h.in Makefile
	rm -f build-post.h.tmp
	sed -e 's;\@localedir\@;$(localedir);' \
	   $(srcdir)/build-post.h.in > build-post.h.tmp
	$(top_srcdir)/move-if-change build-post.h.tmp build-post.h
	@echo timestamp > stamp-build-post-h

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
