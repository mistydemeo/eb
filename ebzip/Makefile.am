localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebzip
noinst_HEADERS = ebzip.h

ebzip_SOURCES = ebzip.c ebzip1.c copyfile.c filename.c unzipbook.c \
	unzipfile.c zipbook.c zipfile.c zipinfobook.c zipinfofile.c
ebzip_LDADD = $(LIBEB) $(LIBEBUTILS) $(ZLIBLIBS) $(INTLLIBS)
ebzip_DEPENDENCIES = $(LIBEB) $(LIBEBUTILS) $(ZLIBDEPS) $(INTLDEPS)

INCLUDES = -I../libebutils -I$(top_srcdir)/libebutils \
	-I../eb -I$(top_srcdir)/eb $(INTLINCS) $(ZLIBINCS)

# Shorten command name `ebzipinfo' for DOS & Win32.
if EXEEXT_EXE
ebzipinfo = ebzipinf
else
ebzipinfo = ebzipinfo
endif

install-exec-hook:
	rm -f $(DESTDIR)$(bindir)/ebunzip$(EXEEXT)
	ln $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'`$(EXEEXT) \
	   $(DESTDIR)$(bindir)/`echo ebunzip|sed '$(transform)'`$(EXEEXT) \
	   || cp $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'`$(EXEEXT) \
	      $(DESTDIR)$(bindir)/`echo ebunzip|sed '$(transform)'`$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/ebzipinfo$(EXEEXT)
	ln $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'`$(EXEEXT) \
	   $(DESTDIR)$(bindir)/`echo $(ebzipinfo)|sed '$(transform)'`$(EXEEXT) \
	   || cp $(DESTDIR)$(bindir)/`echo ebzip|sed '$(transform)'`$(EXEEXT) \
	      $(DESTDIR)$(bindir)/`echo $(ebzipinfo)|sed '$(transform)'`$(EXEEXT)

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/`echo ebunzip|sed '$(transform)'`$(EXEEXT)
	rm -f $(DESTDIR)$(bindir)/`echo $(ebzipinfo)|sed '$(transform)'`$(EXEEXT)

### *.o dependencies ###
copyfile.o: copyfile.c ../config.h ../libebutils/ebutils.h ../eb/eb.h \
 ../eb/defs.h ../eb/zio.h ../libebutils/getumask.h \
 ../libebutils/makedir.h ../libebutils/samefile.h \
 ../libebutils/yesno.h ../eb/error.h ../eb/internal.h ebzip.h
ebzip.o: ebzip.c ../config.h ../libebutils/ebutils.h ../eb/eb.h \
 ../eb/defs.h ../eb/zio.h ../eb/error.h ../eb/internal.h ../eb/font.h \
 ../libebutils/getopt.h ../libebutils/getumask.h \
 ../libebutils/makedir.h ../libebutils/samefile.h \
 ../libebutils/yesno.h ebzip.h
ebzip1.o: ebzip1.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/zio.h
filename.o: filename.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/zio.h
unzipbook.o: unzipbook.c ../config.h ../eb/eb.h ../eb/defs.h \
 ../eb/zio.h ../eb/error.h ../eb/internal.h ../eb/font.h \
 ../libebutils/getumask.h ../libebutils/makedir.h \
 ../libebutils/ebutils.h ebzip.h
unzipfile.o: unzipfile.c ../config.h ../eb/eb.h ../eb/defs.h \
 ../eb/zio.h ../eb/internal.h ../libebutils/getumask.h \
 ../libebutils/makedir.h ../libebutils/samefile.h \
 ../libebutils/yesno.h ebzip.h ../libebutils/ebutils.h
zipbook.o: zipbook.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/zio.h \
 ../eb/error.h ../eb/internal.h ../eb/font.h ../libebutils/getumask.h \
 ../libebutils/makedir.h ../libebutils/ebutils.h ebzip.h
zipfile.o: zipfile.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/zio.h \
 ../eb/internal.h ../libebutils/getumask.h ../libebutils/makedir.h \
 ../libebutils/samefile.h ../libebutils/yesno.h ebzip.h \
 ../libebutils/ebutils.h
zipinfobook.o: zipinfobook.c ../config.h ../eb/eb.h ../eb/defs.h \
 ../eb/zio.h ../eb/error.h ../eb/internal.h ../eb/font.h \
 ../libebutils/ebutils.h ebzip.h
zipinfofile.o: zipinfofile.c ../config.h ../eb/eb.h ../eb/defs.h \
 ../eb/zio.h ../eb/internal.h ../libebutils/ebutils.h \
 ../libebutils/samefile.h ../libebutils/yesno.h ebzip.h
