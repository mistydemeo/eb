@SET_MAKE@

LIBEB = $(top_builddir)/eb/libeb.la
LIBEBUTILS = $(top_builddir)/libebutils/libebutils.a

bin_PROGRAMS = ebzip

ebzip_SOURCES = ebzip.c ebzip1.c filelist.c
ebzip_LDADD = $(LIBEB) $(LIBEBUTILS) @LDADD_FOR_LIBZ@
ebzip_DEPENDENCIES = $(LIBEB) $(LIBEBUTILS) @DEPENDENCIES_FOR_LIBZ@

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/libebutils @INCLUDES_FOR_LIBZ@

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
ebzip.o: ebzip.c ../config.h ../eb/eb.h ../eb/defs.h ../eb/error.h \
 ../eb/internal.h ../libebutils/fakelog.h ../libebutils/getopt.h \
 ../libebutils/getumask.h ../libebutils/makedir.h \
 ../libebutils/samefile.h ../libebutils/yesno.h
ebzip1.o: ebzip1.c ../config.h ../eb/eb.h ../eb/defs.h
