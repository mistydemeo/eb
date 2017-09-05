localedir = $(datadir)/locale

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebzip
noinst_HEADERS = ebzip.h

ebzip_SOURCES = ebzip.c ebzip1.c copyfile.c unzipbook.c unzipfile.c \
	zipbook.c zipfile.c zipinfobook.c zipinfofile.c sebxa.c speedup.c \
	unlinkfile.c
ebzip_LDADD = $(LIBEBUTILS) $(LIBEB) $(ZLIBLIBS) $(INTLLIBS) $(ICONVLIBS)
ebzip_DEPENDENCIES = $(LIBEB) $(LIBEBUTILS) $(ZLIBDEPS) $(INTLDEPS) \
	$(ICONVDEPS)

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
