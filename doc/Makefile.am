info_TEXINFOS = ebfont.texi ebinfo.texi ebrefile.texi ebzip.texi
ebfont_TEXINFOS = ebfont-ver.texi
ebinfo_TEXINFOS = ebinfo-ver.texi
ebrefile_TEXINFOS = ebrefile-ver.texi
ebzip_TEXINFOS = ebzip-ver.texi

samplesdir = $(srcdir)/../samples

CLEANFILES = vti.tmp
EXTRA_DIST = stamp-ebfont stamp-ebinfo stamp-ebrefile stamp-ebzip

ebfont-ver.texi: stamp-ebfont
	cp $(srcdir)/stamp-ebfont $(srcdir)/ebfont-ver.texi

stamp-ebfont: ebfont.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebfont.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebfont \
	  || (echo "Updating $(srcdir)/stamp-ebfont"; \
	      cp vti.tmp $(srcdir)/stamp-ebfont)
	-@rm -f vti.tmp

ebinfo-ver.texi: stamp-ebinfo
	cp $(srcdir)/stamp-ebinfo $(srcdir)/ebinfo-ver.texi

stamp-ebinfo: ebinfo.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebinfo.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebinfo \
	  || (echo "Updating $(srcdir)/stamp-ebinfo"; \
	      cp vti.tmp $(srcdir)/stamp-ebinfo)
	-@rm -f vti.tmp

ebrefile-ver.texi: stamp-ebrefile
	cp $(srcdir)/stamp-ebrefile $(srcdir)/ebrefile-ver.texi

stamp-ebrefile: ebrefile.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebrefile.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebrefile \
	  || (echo "Updating $(srcdir)/stamp-ebrefile"; \
	      cp vti.tmp $(srcdir)/stamp-ebrefile)
	-@rm -f vti.tmp

ebzip-ver.texi: stamp-ebzip
	cp $(srcdir)/stamp-ebzip $(srcdir)/ebzip-ver.texi

stamp-ebzip: ebzip.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebzip.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebzip \
	  || (echo "Updating $(srcdir)/stamp-ebzip"; \
	      cp vti.tmp $(srcdir)/stamp-ebzip)
	-@rm -f vti.tmp

