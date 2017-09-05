AUTOMAKE_OPTIONS = foreign

lib_LTLIBRARIES = libz.la
libz_la_SOURCES = adler32.c compress.c crc32.c gzio.c uncompr.c deflate.c \
	trees.c zutil.c inflate.c infblock.c inftrees.c infcodes.c \
	infutil.c inffast.c
#libz_la_LDFLAGS = -version-info @LIBZ_VERSION_INFO@
include_HEADERS = zlib.h zconf.h

noinst_HEADERS = deflate.h infblock.h infcodes.h inffast.h inffixed.h \
	inftrees.h infutil.h trees.h zutil.h

EXTRA_DIST = ChangeLog FAQ INDEX README.altered algorithm.txt zlib.3
