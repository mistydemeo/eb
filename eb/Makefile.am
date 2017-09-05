@SET_MAKE@

SUBDIRS = @POSUB@

lib_LTLIBRARIES = libeb.la
pkginclude_HEADERS = appendix.h defs.h eb.h error.h font.h language.h text.h

INCLUDES = -DEB_BUILD_LIBRARY $(INTLINCS) $(ZLIBINCS)

libeb_la_SOURCES = appendix.c appsub.c bitmap.c book.c copyright.c \
   eb.c endword.c error.c exactword.c filename.c font.c graphic.c hook.c \
   jacode.c keyword.c language.c match.c menu.c memmove.c message.c multi.c \
   narwalt.c narwfont.c search.c setword.c stopcode.c strcasecmp.c subbook.c \
   text.c uint.c unzebzip.c unzepwing.c widealt.c widefont.c \
   word.c zio.c
libeb_la_LDFLAGS = -version-info @LIBEB_VERSION_INFO@

EXTRA_DIST = internal.h.in
CLEANFILES = stamp-internal-h internal.h
BUILT_SOURCES = internal.h

internal.h: stamp-internal-h
stamp-internal-h: internal.h.in Makefile
	rm -f internal.h.tmp
	sed -e 's;\@localedir\@;$(localedir);' \
	   $(srcdir)/internal.h.in > internal.h.tmp
	$(top_srcdir)/move-if-change internal.h.tmp internal.h
	@echo timestamp > stamp-internal-h

### *.o dependencies ###
appendix.o: appendix.c ../config.h eb.h error.h internal.h appendix.h
appsub.o: appsub.c ../config.h eb.h error.h internal.h appendix.h
bitmap.o: bitmap.c ../config.h eb.h error.h internal.h font.h
book.o: book.c ../config.h eb.h error.h internal.h font.h language.h
copyright.o: copyright.c ../config.h eb.h error.h internal.h
eb.o: eb.c ../config.h eb.h error.h internal.h
endword.o: endword.c ../config.h eb.h error.h internal.h
error.o: error.c ../config.h eb.h error.h internal.h
exactword.o: exactword.c ../config.h eb.h error.h internal.h
filename.o: filename.c ../config.h eb.h error.h internal.h
font.o: font.c ../config.h eb.h error.h font.h internal.h
graphic.o: graphic.c ../config.h eb.h error.h internal.h
hook.o: hook.c ../config.h eb.h error.h internal.h text.h appendix.h
jacode.o: jacode.c ../config.h
keyword.o: keyword.c ../config.h eb.h error.h internal.h
language.o: language.c ../config.h eb.h error.h internal.h language.h
lock.o: lock.c ../config.h
match.o: match.c ../config.h
memmove.o: memmove.c ../config.h
menu.o: menu.c ../config.h eb.h error.h internal.h
message.o: message.c ../config.h eb.h error.h internal.h language.h
multi.o: multi.c ../config.h eb.h error.h internal.h
narwalt.o: narwalt.c ../config.h eb.h error.h appendix.h internal.h
narwfont.o: narwfont.c ../config.h eb.h error.h font.h internal.h
search.o: search.c ../config.h eb.h error.h internal.h text.h
setword.o: setword.c ../config.h eb.h error.h internal.h
stopcode.o: stopcode.c ../config.h eb.h error.h internal.h appendix.h \
 text.h
strcasecmp.o: strcasecmp.c ../config.h
subbook.o: subbook.c ../config.h eb.h error.h font.h internal.h
text.o: text.c ../config.h eb.h error.h internal.h text.h
uint.o: uint.c ../config.h eb.h error.h internal.h
unzebzip.o: unzebzip.c ../config.h eb.h error.h internal.h
unzepwing.o: unzepwing.c ../config.h eb.h error.h internal.h
widealt.o: widealt.c ../config.h eb.h error.h appendix.h internal.h
widefont.o: widefont.c ../config.h eb.h error.h font.h internal.h
word.o: word.c ../config.h eb.h error.h internal.h
zio.o: zio.c ../config.h eb.h error.h internal.h

### *.lo dependencies ###
appendix.lo: appendix.c ../config.h eb.h error.h internal.h appendix.h
appsub.lo: appsub.c ../config.h eb.h error.h internal.h appendix.h
bitmap.lo: bitmap.c ../config.h eb.h error.h internal.h font.h
book.lo: book.c ../config.h eb.h error.h internal.h font.h language.h
copyright.lo: copyright.c ../config.h eb.h error.h internal.h
eb.lo: eb.c ../config.h eb.h error.h internal.h
endword.lo: endword.c ../config.h eb.h error.h internal.h
error.lo: error.c ../config.h eb.h error.h internal.h
exactword.lo: exactword.c ../config.h eb.h error.h internal.h
filename.lo: filename.c ../config.h eb.h error.h internal.h
font.lo: font.c ../config.h eb.h error.h font.h internal.h
graphic.lo: graphic.c ../config.h eb.h error.h internal.h
hook.lo: hook.c ../config.h eb.h error.h internal.h text.h appendix.h
jacode.lo: jacode.c ../config.h
keyword.lo: keyword.c ../config.h eb.h error.h internal.h
language.lo: language.c ../config.h eb.h error.h internal.h language.h
lock.lo: lock.c ../config.h
match.lo: match.c ../config.h
memmove.lo: memmove.c ../config.h
menu.lo: menu.c ../config.h eb.h error.h internal.h
message.lo: message.c ../config.h eb.h error.h internal.h language.h
multi.lo: multi.c ../config.h eb.h error.h internal.h
narwalt.lo: narwalt.c ../config.h eb.h error.h appendix.h internal.h
narwfont.lo: narwfont.c ../config.h eb.h error.h font.h internal.h
search.lo: search.c ../config.h eb.h error.h internal.h text.h
setword.lo: setword.c ../config.h eb.h error.h internal.h
stopcode.lo: stopcode.c ../config.h eb.h error.h internal.h appendix.h \
 text.h
strcasecmp.lo: strcasecmp.c ../config.h
subbook.lo: subbook.c ../config.h eb.h error.h font.h internal.h
text.lo: text.c ../config.h eb.h error.h internal.h text.h
uint.lo: uint.c ../config.h eb.h error.h internal.h
unzebzip.lo: unzebzip.c ../config.h eb.h error.h internal.h
unzepwing.lo: unzepwing.c ../config.h eb.h error.h internal.h
widealt.lo: widealt.c ../config.h eb.h error.h appendix.h internal.h
widefont.lo: widefont.c ../config.h eb.h error.h font.h internal.h
word.lo: word.c ../config.h eb.h error.h internal.h
zio.lo: zio.c ../config.h eb.h error.h internal.h

