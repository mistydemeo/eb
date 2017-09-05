info_TEXINFOS = eb-ja.texi ebappendix-ja.texi ebfont-ja.texi ebinfo-ja.texi \
	ebrefile-ja.texi ebstopcode-ja.texi ebzip-ja.texi
eb_ja_TEXINFOS = initexit-c.texi disctype-c.texi subbook-c.texi \
	word-c.texi text-c.texi font-c.texi appendix-c.texi booklist-c.texi

samplesdir = $(srcdir)/../samples

C2TEXI = $(srcdir)/c2texi
EXTRA_DIST = c2texi texinfo-ja.tex
MAINTAINERCLEANFILES = initexit-c.texi disctype-c.texi subbook-c.texi \
	word-c.texi text-c.texi font-c.texi appendix-c.texi booklist-c.texi

initexit-c.texi: $(C2TEXI) $(samplesdir)/initexit.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/initexit.c > $@

disctype-c.texi: $(C2TEXI) $(samplesdir)/disctype.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/disctype.c > $@

subbook-c.texi: $(C2TEXI) $(samplesdir)/subbook.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/subbook.c > $@

word-c.texi: $(C2TEXI) $(samplesdir)/word.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/word.c > $@

text-c.texi: $(C2TEXI) $(samplesdir)/text.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/text.c > $@

font-c.texi: $(C2TEXI) $(samplesdir)/font.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/font.c > $@

appendix-c.texi: $(C2TEXI) $(samplesdir)/appendix.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/appendix.c > $@

booklist-c.texi: $(C2TEXI) $(samplesdir)/booklist.c
	-@rm -f $@
	$(SHELL) $(C2TEXI) $(samplesdir)/booklist.c > $@
