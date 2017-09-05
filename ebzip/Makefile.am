@SET_MAKE@

localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebzip

ebzip_SOURCES = ebzip.c ebzip1.c filename.c
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
ebzip.o: ebzip.c ../config.h ../libebutils/ebutils.h ../eb/eb.h \
 ../eb/error.h ../eb/internal.h ../eb/font.h ../libebutils/fakelog.h \
 ../libebutils/getopt.h ../libebutils/getumask.h \
 ../libebutils/makedir.h ../libebutils/samefile.h \
 ../libebutils/yesno.h
ebzip1.o: ebzip1.c ../config.h ../eb/eb.h
filename.o: filename.c ../config.h ../eb/eb.h
