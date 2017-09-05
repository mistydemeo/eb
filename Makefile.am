if WITH_ZLIB
ZLIB_SUBDIR = 
else
ZLIB_SUBDIR = zlib
endif

if ENABLE_JA_DOC
JA_DOC_SUBDIR = doc-ja
else
JA_DOC_SUBDIR = 
endif

SUBDIRS = $(ZLIB_SUBDIR) eb libebutils ebappendix ebfont ebinfo ebrefile \
	ebzip doc $(JA_DOC_SUBDIR)

EXTRA_DIST = INSTALL-ja README-ja \
	samples/Makefile.am samples/Makefile.in samples/README \
	samples/aclocal.m4 samples/configure samples/configure.in \
	samples/disctype.c samples/missing samples/stamp-h.in \
	samples/subbook.c samples/word.c \
	samples-ja/Makefile.am samples-ja/Makefile.in samples-ja/README-ja \
	samples-ja/aclocal.m4 samples-ja/configure samples-ja/configure.in \
	samples-ja/disctype.c samples-ja/missing samples-ja/stamp-h.in \
	samples-ja/subbook.c samples-ja/word.c

appendixdir = $(pkgdatadir)/appendix
appendix_DATA =
