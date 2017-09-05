@SET_MAKE@

noinst_LIBRARIES = libebutils.a
libebutils_a_SOURCES = fakelog.c getopt.c getopt1.c getumask.c makedir.c \
	samefile.c yesno.c
libebutils_a_LIBADD = @LIBOBJS@

EXTRA_DIST = memset.c strcasecmp.c strerror.c getopt_long.pl

noinst_HEADERS = fakelog.h getopt.h getumask.h makedir.h samefile.h yesno.h

### *.o dependencies ###
fakelog.o: fakelog.c ../config.h fakelog.h
getopt.o: getopt.c getopt.h
getopt1.o: getopt1.c getopt.h
getumask.o: getumask.c ../config.h
makedir.o: makedir.c ../config.h fakelog.h
memset.o: memset.c ../config.h
samefile.o: samefile.c ../config.h
strcasecmp.o: strcasecmp.c ../config.h
strerror.o: strerror.c ../config.h
yesno.o: yesno.c ../config.h
