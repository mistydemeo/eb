@SET_MAKE@

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebzip

ebzip_SOURCES = ebzip.c zip1.c
ebzip_LDADD = $(LIBEB) $(LIBEBUTILS) @LDADD_FOR_LIBZ@
ebzip_DEPENDENCIES = $(LIBEB) $(LIBEBUTILS) @DEPENDENCIES_FOR_LIBZ@

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libebutils @INCLUDES_FOR_LIBZ@

install-exec-hook:
	rm -f $(DESTDIR)$(bindir)/ebunzip $(DESTDIR)$(bindir)/ebzipinfo
	ln $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'` \
	   $(DESTDIR)$(bindir)/`echo ebunzip|sed '$(transform)'` \
	   || cp $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'` \
	      $(DESTDIR)$(bindir)/`echo ebunzip|sed '$(transform)'`
	ln $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'` \
	   $(DESTDIR)$(bindir)/`echo ebzipinfo|sed '$(transform)'` \
	   || cp $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'` \
	      $(DESTDIR)$(bindir)/`echo ebzipinfo|sed '$(transform)'`

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/`echo ebunzip|sed '$(transform)'`
	rm -f $(DESTDIR)$(bindir)/`echo ebzipinfo|sed '$(transform)'`

### *.o dependencies ###
ebzip.o: ebzip.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/error.h \
 ../eb/internal.h ../libebutils/fakelog.h ../libebutils/getopt.h \
 ../libebutils/getumask.h ../libebutils/makedir.h \
 ../libebutils/samefile.h ../libebutils/yesno.h
zip1.o: zip1.c ../config.h ../eb/eb.h ../eb/defs.h
