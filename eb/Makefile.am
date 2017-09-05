@SET_MAKE@

lib_LTLIBRARIES = libeb.la
pkginclude_HEADERS = appendix.h defs.h eb.h error.h font.h language.h \
	text.h
noinst_HEADERS = internal.h

libeb_la_SOURCES = appendix.c appsub.c bitmap.c copyright.c eb.c error.c \
	filename.c font.c graphic.c hook.c jpcode.c keyword.c language.c \
	match.c menu.c message.c multi.c narwalt.c narwfont.c search.c \
	setword.c stopcode.c subbook.c text.c uint.c unzip1.c widealt.c \
	widefont.c zio.c

libeb_la_LDFLAGS = -version-info @LIBEB_VERSION_INFO@

INCLUDES = @INCLUDES_FOR_LIBZ@

install-data-hook:
	rm -f $(DESTDIR)$(pkgincludedir)/io.h
	rm -f $(DESTDIR)$(pkgincludedir)/types.h

### *.o dependencies ###
appendix.o: appendix.c ../config.h eb.h defs.h error.h internal.h \
 appendix.h
appsub.o: appsub.c ../config.h eb.h defs.h error.h internal.h \
 appendix.h
bitmap.o: bitmap.c ../config.h eb.h defs.h error.h font.h
copyright.o: copyright.c ../config.h eb.h defs.h error.h
eb.o: eb.c ../config.h eb.h defs.h error.h internal.h language.h
error.o: error.c ../config.h eb.h defs.h error.h
filename.o: filename.c ../config.h eb.h defs.h error.h
font.o: font.c ../config.h eb.h defs.h error.h font.h internal.h
graphic.o: graphic.c ../config.h eb.h defs.h error.h
hook.o: hook.c ../config.h eb.h defs.h text.h
jpcode.o: jpcode.c ../config.h
keyword.o: keyword.c ../config.h eb.h defs.h error.h internal.h
language.o: language.c ../config.h eb.h defs.h error.h internal.h \
 language.h
match.o: match.c ../config.h
menu.o: menu.c ../config.h eb.h defs.h error.h
message.o: message.c ../config.h eb.h defs.h error.h internal.h \
 language.h
multi.o: multi.c ../config.h eb.h defs.h error.h internal.h
narwalt.o: narwalt.c ../config.h eb.h defs.h error.h appendix.h \
 internal.h
narwfont.o: narwfont.c ../config.h eb.h defs.h error.h font.h \
 internal.h
search.o: search.c ../config.h eb.h defs.h error.h internal.h
setword.o: setword.c ../config.h eb.h defs.h error.h
stopcode.o: stopcode.c ../config.h eb.h defs.h appendix.h error.h \
 text.h
subbook.o: subbook.c ../config.h eb.h defs.h error.h font.h internal.h
text.o: text.c ../config.h eb.h defs.h error.h internal.h text.h
uint.o: uint.c ../config.h eb.h defs.h error.h
unzip1.o: unzip1.c ../config.h defs.h
widealt.o: widealt.c ../config.h eb.h defs.h error.h appendix.h \
 internal.h
widefont.o: widefont.c ../config.h eb.h defs.h error.h font.h \
 internal.h
zio.o: zio.c ../config.h eb.h defs.h error.h internal.h

### *.lo dependencies ###
appendix.lo: appendix.c ../config.h eb.h defs.h error.h internal.h \
 appendix.h
appsub.lo: appsub.c ../config.h eb.h defs.h error.h internal.h \
 appendix.h
bitmap.lo: bitmap.c ../config.h eb.h defs.h error.h font.h
copyright.lo: copyright.c ../config.h eb.h defs.h error.h
eb.lo: eb.c ../config.h eb.h defs.h error.h internal.h language.h
error.lo: error.c ../config.h eb.h defs.h error.h
filename.lo: filename.c ../config.h eb.h defs.h error.h
font.lo: font.c ../config.h eb.h defs.h error.h font.h internal.h
graphic.lo: graphic.c ../config.h eb.h defs.h error.h
hook.lo: hook.c ../config.h eb.h defs.h text.h
jpcode.lo: jpcode.c ../config.h
keyword.lo: keyword.c ../config.h eb.h defs.h error.h internal.h
language.lo: language.c ../config.h eb.h defs.h error.h internal.h \
 language.h
match.lo: match.c ../config.h
menu.lo: menu.c ../config.h eb.h defs.h error.h
message.lo: message.c ../config.h eb.h defs.h error.h internal.h \
 language.h
multi.lo: multi.c ../config.h eb.h defs.h error.h internal.h
narwalt.lo: narwalt.c ../config.h eb.h defs.h error.h appendix.h \
 internal.h
narwfont.lo: narwfont.c ../config.h eb.h defs.h error.h font.h \
 internal.h
search.lo: search.c ../config.h eb.h defs.h error.h internal.h
setword.lo: setword.c ../config.h eb.h defs.h error.h
stopcode.lo: stopcode.c ../config.h eb.h defs.h appendix.h error.h \
 text.h
subbook.lo: subbook.c ../config.h eb.h defs.h error.h font.h internal.h
text.lo: text.c ../config.h eb.h defs.h error.h internal.h text.h
uint.lo: uint.c ../config.h eb.h defs.h error.h
unzip1.lo: unzip1.c ../config.h defs.h
widealt.lo: widealt.c ../config.h eb.h defs.h error.h appendix.h \
 internal.h
widefont.lo: widefont.c ../config.h eb.h defs.h error.h font.h \
 internal.h
zio.lo: zio.c ../config.h eb.h defs.h error.h internal.h

