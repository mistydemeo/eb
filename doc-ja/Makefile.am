@SET_MAKE@

info_TEXINFOS = eb-ja.texi ebfont-ja.texi ebinfo-ja.texi ebrefile-ja.texi ebzip-ja.texi
eb_ja_TEXINFOS = eb-ver.texi disctype.c.texi subbook.c.texi
ebfont_ja_TEXINFOS = ebfont-ver.texi
ebinfo_ja_TEXINFOS = ebinfo-ver.texi
ebrefile_ja_TEXINFOS = ebrefile-ver.texi
ebzip_ja_TEXINFOS = ebzip-ver.texi

samplesdir = $(srcdir)/../samples

C2TEXI = $(srcdir)/c2texi

CLEANFILES = vti.tmp disctype.c.texi subbook.c.texi
EXTRA_DIST = texinfo-ja.tex stamp-eb stamp-ebfont stamp-ebinfo \
	stamp-ebrefile stamp-ebzip c2texi

eb-ver.texi: stamp-eb
	cp $(srcdir)/stamp-eb $(srcdir)/eb-ver.texi

stamp-eb: eb-ja.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh eb-ja.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-eb \
	  || (echo "Updating $(srcdir)/stamp-eb"; \
	      cp vti.tmp $(srcdir)/stamp-eb)
	-@rm -f vti.tmp

ebfont-ver.texi: stamp-ebfont
	cp $(srcdir)/stamp-ebfont $(srcdir)/ebfont-ver.texi

stamp-ebfont: ebfont-ja.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebfont-ja.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebfont \
	  || (echo "Updating $(srcdir)/stamp-ebfont"; \
	      cp vti.tmp $(srcdir)/stamp-ebfont)
	-@rm -f vti.tmp

ebinfo-ver.texi: stamp-ebinfo
	cp $(srcdir)/stamp-ebinfo $(srcdir)/ebinfo-ver.texi

stamp-ebinfo: ebinfo-ja.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebinfo-ja.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebinfo \
	  || (echo "Updating $(srcdir)/stamp-ebinfo"; \
	      cp vti.tmp $(srcdir)/stamp-ebinfo)
	-@rm -f vti.tmp

ebrefile-ver.texi: stamp-ebrefile
	cp $(srcdir)/stamp-ebrefile $(srcdir)/ebrefile-ver.texi

stamp-ebrefile: ebrefile-ja.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebrefile-ja.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebrefile \
	  || (echo "Updating $(srcdir)/stamp-ebrefile"; \
	      cp vti.tmp $(srcdir)/stamp-ebrefile)
	-@rm -f vti.tmp

ebzip-ver.texi: stamp-ebzip
	cp $(srcdir)/stamp-ebzip $(srcdir)/ebzip-ver.texi

stamp-ebzip: ebzip-ja.texi $(top_srcdir)/configure.in
	@echo "@set UPDATED `cd $(srcdir) \
	  && $(SHELL) ./mdate-sh ebzip-ja.texi`" > vti.tmp
	@echo "@set EDITION $(VERSION)" >> vti.tmp
	@echo "@set VERSION $(VERSION)" >> vti.tmp
	@cmp -s vti.tmp $(srcdir)/stamp-ebzip \
	  || (echo "Updating $(srcdir)/stamp-ebzip"; \
	      cp vti.tmp $(srcdir)/stamp-ebzip)
	-@rm -f vti.tmp

disctype.c.texi: $(C2TEXI) $(samplesdir)/disctype.c.in
	-@rm -f disctype.c.texi
	sed -e '1,/^$$/d' -e '/ en:/d' -e 's/ ja://' \
	  $(samplesdir)/disctype.c.in | $(SHELL) $(C2TEXI) > disctype.c.texi

subbook.c.texi: $(C2TEXI) $(samplesdir)/subbook.c.in
	-@rm -f subbook.c.texi
	sed -e '1,/^$$/d' -e '/ en:/d' -e 's/ ja://' \
	   $(samplesdir)/subbook.c.in | $(SHELL) $(C2TEXI) > subbook.c.texi

