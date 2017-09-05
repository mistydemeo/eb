gnulocaledir = $(datadir)/locale

if INCLUDED_LIBINTL
INTL_LTLIB = libintl.la 
else
INTL_LTLIB = 
endif

lib_LTLIBRARIES = $(INTL_LTLIB)
libintl_la_SOURCES = bindtextdom.c dcgettext.c dgettext.c gettext.c \
   finddomain.c loadmsgcat.c localealias.c textdomain.c l10nflist.c \
   explodename.c intl-compat.c
libintl_la_LDFLAGS = -version-info 1:0:0

include_HEADERS = libintl.h
noinst_HEADERS = gettext.h gettextP.h hash-string.h libgettext.h loadinfo.h
BUILT_SOURCES = libintl.h

INCLUDES = -DLOCALEDIR=\"$(localedir)\" -DGNULOCALEDIR=\"$(gnulocaledir)\" \
   -DLOCALE_ALIAS_PATH=\"$(aliaspath)\"

EXTRA_DIST = ChangeLog VERSION po2tbl.sed.in

DISTCLEANFILES = libintl.h po2tbl.sed

libintl.h: $(srcdir)/libgettext.h
	rm -f $@
	$(LN_S) $(srcdir)/libgettext.h libintl.h

### *.o dependencies ###
bindtextdom.o: bindtextdom.c libgettext.h gettext.h gettextP.h \
 loadinfo.h
dcgettext.o: dcgettext.c gettext.h gettextP.h loadinfo.h libgettext.h \
 hash-string.h
dgettext.o: dgettext.c libgettext.h
explodename.o: explodename.c loadinfo.h
finddomain.o: finddomain.c gettext.h gettextP.h loadinfo.h \
 libgettext.h
gettext.o: gettext.c libgettext.h
intl-compat.o: intl-compat.c libgettext.h
l10nflist.o: l10nflist.c loadinfo.h
loadmsgcat.o: loadmsgcat.c gettext.h gettextP.h loadinfo.h
localealias.o: localealias.c gettext.h gettextP.h loadinfo.h
textdomain.o: textdomain.c libgettext.h

### *.lo dependencies ###
bindtextdom.lo: bindtextdom.c libgettext.h gettext.h gettextP.h \
 loadinfo.h
dcgettext.lo: dcgettext.c gettext.h gettextP.h loadinfo.h libgettext.h \
 hash-string.h
dgettext.lo: dgettext.c libgettext.h
explodename.lo: explodename.c loadinfo.h
finddomain.lo: finddomain.c gettext.h gettextP.h loadinfo.h \
 libgettext.h
gettext.lo: gettext.c libgettext.h
intl-compat.lo: intl-compat.c libgettext.h
l10nflist.lo: l10nflist.c loadinfo.h
loadmsgcat.lo: loadmsgcat.c gettext.h gettextP.h loadinfo.h
localealias.lo: localealias.c gettext.h gettextP.h loadinfo.h
textdomain.lo: textdomain.c libgettext.h

