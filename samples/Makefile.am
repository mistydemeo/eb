AUTOMAKE_OPTIONS = foreign

@SET_MAKE@

LIBEB = ../eb/libeb.la

noinst_PROGRAMS = disctype subbook word

disctype_SOURCES = disctype.c
disctype_LDADD = $(LIBEB) @LDADD_FOR_LIBZ@
disctype_DEPENDENCIES = $(LIBEB) @DEPENDENCIES_FOR_LIBZ@

subbook_SOURCES = subbook.c
subbook_LDADD = $(LIBEB) @LDADD_FOR_LIBZ@
subbook_DEPENDENCIES = $(LIBEB) @DEPENDENCIES_FOR_LIBZ@

word_SOURCES = word.c
word_LDADD = $(LIBEB) @LDADD_FOR_LIBZ@
word_DEPENDENCIES = $(LIBEB) @DEPENDENCIES_FOR_LIBZ@

BUILT_SOURCES = disctype.c subbook.c word.c
EXTRA_DIST = README README-ja disctype.c.in subbook.c.in word.c.in

INCLUDES = -I$(srcdir)/..

disctype.c: $(srcdir)/disctype.c.in
	-@rm -f disctype.c
	sed -e '1,/^$$/d' -e '/ ja:/d' -e 's/ en://' \
	  $(srcdir)/disctype.c.in > disctype.c

subbook.c: $(srcdir)/subbook.c.in
	-@rm -f subbook.c
	sed -e '1,/^$$/d' -e '/ ja:/d' -e 's/ en://' \
	  $(srcdir)/subbook.c.in > subbook.c

word.c: $(srcdir)/word.c.in
	-@rm -f word.c
	sed -e '1,/^$$/d' -e '/ ja:/d' -e 's/ en://' \
	  $(srcdir)/word.c.in > word.c

### *.o dependencies ###
disctype.o: disctype.c ../eb/eb.h ../eb/defs.h ../eb/error.h
subbook.o: subbook.c ../eb/eb.h ../eb/defs.h ../eb/error.h
word.o: word.c ../eb/eb.h ../eb/defs.h ../eb/error.h ../eb/text.h
