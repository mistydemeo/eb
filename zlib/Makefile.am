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

### *.o dependencies ###
adler32.o: zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: zlib.h zconf.h
deflate.o: deflate.h zutil.h zlib.h zconf.h
example.o: zlib.h zconf.h
gzio.o: zutil.h zlib.h zconf.h
infblock.o: infblock.h inftrees.h infcodes.h infutil.h zutil.h zlib.h zconf.h
infcodes.o: zutil.h zlib.h zconf.h
infcodes.o: inftrees.h infblock.h infcodes.h infutil.h inffast.h
inffast.o: zutil.h zlib.h zconf.h inftrees.h
inffast.o: infblock.h infcodes.h infutil.h inffast.h
inflate.o: zutil.h zlib.h zconf.h infblock.h
inftrees.o: zutil.h zlib.h zconf.h inftrees.h
infutil.o: zutil.h zlib.h zconf.h infblock.h inftrees.h infcodes.h infutil.h
minigzip.o:  zlib.h zconf.h
trees.o: deflate.h zutil.h zlib.h zconf.h trees.h
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h

### *.lo dependencies ###
adler32.lo: zlib.h zconf.h
compress.lo: zlib.h zconf.h
crc32.lo: zlib.h zconf.h
deflate.lo: deflate.h zutil.h zlib.h zconf.h
example.lo: zlib.h zconf.h  
gzio.lo: zutil.h zlib.h zconf.h
infblock.lo: infblock.h inftrees.h infcodes.h infutil.h zutil.h zlib.h zconf.h
infcodes.lo: zutil.h zlib.h zconf.h
infcodes.lo: inftrees.h infblock.h infcodes.h infutil.h inffast.h
inffast.lo: zutil.h zlib.h zconf.h inftrees.h
inffast.lo: infblock.h infcodes.h infutil.h inffast.h
inflate.lo: zutil.h zlib.h zconf.h infblock.h
inftrees.lo: zutil.h zlib.h zconf.h inftrees.h
infutil.lo: zutil.h zlib.h zconf.h infblock.h inftrees.h infcodes.h infutil.h
minigzip.lo:  zlib.h zconf.h
trees.lo: deflate.h zutil.h zlib.h zconf.h trees.h
uncompr.lo: zlib.h zconf.h
zutil.lo: zutil.h zlib.h zconf.h
