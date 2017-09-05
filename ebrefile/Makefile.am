bin_SCRIPTS = ebrefile

EXTRA_DIST = ebrefile.in
CLEANFILES = ebrefile ebrefile.tmp

ebrefile: ebrefile.in $(top_srcdir)/libebutils/getopt_long.pl Makefile
	rm -f ebrefile
	sed -e 's;\@PERL\@;$(PERL);' \
	    $(srcdir)/ebrefile.in | sed '/^$$/q' > ebrefile.tmp
	cat $(top_srcdir)/libebutils/getopt_long.pl >> ebrefile.tmp
	sed -e 's;\@VERSION\@;$(VERSION);' \
	   -e 's;\@MAILING_ADDRESS\@;$(MAILING_ADDRESS);' \
	   $(srcdir)/ebrefile.in | sed '1,/^$$/d' >> ebrefile.tmp
	cp ebrefile.tmp ebrefile
	chmod +x ebrefile
	rm -f ebrefile.tmp
