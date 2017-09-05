SUBDIRS = po

lib_LTLIBRARIES = libeb.la
pkginclude_HEADERS = appendix.h binary.h defs.h eb.h error.h font.h \
	language.h libintl_r.h text.h zio.h
noinst_HEADERS = ebconfig.h

INCLUDES = -DEB_BUILD_LIBRARY $(INTLINCS) $(ZLIBINCS)

libeb_la_SOURCES = appendix.c appsub.c bcd.c binary.c bitmap.c book.c \
   copyright.c eb.c endword.c error.c exactword.c filename.c font.c \
   hook.c jacode.c keyword.c language.c libintl_r.c lock.c match.c menu.c \
   memmove.c message.c multi.c narwalt.c narwfont.c search.c setword.c \
   stopcode.c strcasecmp.c subbook.c text.c widealt.c widefont.c word.c zio.c
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

### *.o dependencies ###
appendix.o: appendix.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h appendix.h internal.h
appsub.o: appsub.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h appendix.h
bcd.o: bcd.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
binary.o: binary.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h binary.h
bitmap.o: bitmap.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h font.h
book.o: book.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h font.h language.h
copyright.o: copyright.c ebconfig.h ../config.h libintl_r.h eb.h \
 defs.h zio.h error.h internal.h
eb.o: eb.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
endword.o: endword.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
error.o: error.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
exactword.o: exactword.c ebconfig.h ../config.h libintl_r.h eb.h \
 defs.h zio.h error.h internal.h
filename.o: filename.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
font.o: font.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h font.h internal.h
hook.o: hook.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h text.h appendix.h
jacode.o: jacode.c ebconfig.h ../config.h libintl_r.h
keyword.o: keyword.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
language.o: language.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h language.h
libintl_r.o: libintl_r.c ../config.h
lock.o: lock.c ebconfig.h ../config.h libintl_r.h defs.h zio.h
match.o: match.c ebconfig.h ../config.h libintl_r.h
memmove.o: memmove.c ../config.h
menu.o: menu.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
message.o: message.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h language.h
multi.o: multi.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
narwalt.o: narwalt.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h appendix.h internal.h
narwfont.o: narwfont.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h font.h internal.h
search.o: search.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h text.h
setword.o: setword.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
stopcode.o: stopcode.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h appendix.h text.h
strcasecmp.o: strcasecmp.c ../config.h
subbook.o: subbook.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h font.h internal.h
text.o: text.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h text.h
texttable.o: texttable.c ../config.h
uint.o: uint.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
word.o: word.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
zio.o: zio.c ../config.h zio.h

### *.lo dependencies ###
appendix.lo: appendix.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h appendix.h internal.h
appsub.lo: appsub.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h appendix.h
bcd.lo: bcd.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
binary.lo: binary.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h binary.h
bitmap.lo: bitmap.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h font.h
book.lo: book.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h font.h language.h
copyright.lo: copyright.c ebconfig.h ../config.h libintl_r.h eb.h \
 defs.h zio.h error.h internal.h
eb.lo: eb.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
endword.lo: endword.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
error.lo: error.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
exactword.lo: exactword.c ebconfig.h ../config.h libintl_r.h eb.h \
 defs.h zio.h error.h internal.h
filename.lo: filename.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
font.lo: font.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h font.h internal.h
hook.lo: hook.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h text.h appendix.h
jacode.lo: jacode.c ebconfig.h ../config.h libintl_r.h
keyword.lo: keyword.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
language.lo: language.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h language.h
libintl_r.lo: libintl_r.c ../config.h
lock.lo: lock.c ebconfig.h ../config.h libintl_r.h defs.h zio.h
match.lo: match.c ebconfig.h ../config.h libintl_r.h
memmove.lo: memmove.c ../config.h
menu.lo: menu.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
message.lo: message.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h language.h
multi.lo: multi.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
narwalt.lo: narwalt.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h appendix.h internal.h
narwfont.lo: narwfont.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h font.h internal.h
search.lo: search.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h text.h
setword.lo: setword.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h
stopcode.lo: stopcode.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h internal.h appendix.h text.h
strcasecmp.lo: strcasecmp.c ../config.h
subbook.lo: subbook.c ebconfig.h ../config.h libintl_r.h eb.h defs.h \
 zio.h error.h font.h internal.h
text.lo: text.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h text.h
texttable.lo: texttable.c ../config.h
uint.lo: uint.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
word.lo: word.c ebconfig.h ../config.h libintl_r.h eb.h defs.h zio.h \
 error.h internal.h
zio.lo: zio.c ../config.h zio.h

