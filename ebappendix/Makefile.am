bin_SCRIPTS = ebappendix

EXTRA_DIST = ebappendix.in
CLEANFILES = ebappendix ebappendix.tmp

ebappendix: ebappendix.in $(top_srcdir)/libebutils/getopt_long.pl Makefile
	rm -f ebappendix
	sed -e 's;\@PERL\@;$(PERL);' \
	    $(srcdir)/ebappendix.in | sed '/^$$/q' > ebappendix.tmp
	cat $(top_srcdir)/libebutils/getopt_long.pl >> ebappendix.tmp
	sed -e 's;\@VERSION\@;$(VERSION);' \
	   -e 's;\@MAILING_ADDRESS\@;$(MAILING_ADDRESS);' \
	   $(srcdir)/ebappendix.in | sed '1,/^$$/d' >> ebappendix.tmp
	cp ebappendix.tmp ebappendix
	chmod +x ebappendix
	rm -f ebappendix.tmp
