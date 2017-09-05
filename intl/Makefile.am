gnulocaledir = $(datadir)/locale

if INCLUDED_GETTEXT
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
noinst_HEADERS = gettext.h gettextP.h hash-string.h loadinfo.h

INCLUDES = -DLOCALEDIR=\"$(localedir)\" -DGNULOCALEDIR=\"$(gnulocaledir)\" \
   -DLOCALE_ALIAS_PATH=\"$(aliaspath)\"

EXTRA_DIST = ChangeLog VERSION po2tbl.sed.in

DISTCLEANFILES = po2tbl.sed
