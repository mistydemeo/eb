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

INCLUDES = -I$(srcdir)/..

EXTRA_DIST = README-ja
### *.o dependencies ###
disctype.o: disctype.c ../eb/eb.h ../eb/defs.h ../eb/error.h
subbook.o: subbook.c ../eb/eb.h ../eb/defs.h ../eb/error.h
word.o: word.c ../eb/eb.h ../eb/defs.h ../eb/error.h ../eb/text.h
