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

if ENABLE_SAMPLES
SAMPLES_SUBDIR = samples
else
SAMPLES_SUBDIR =
endif

SUBDIRS = $(ZLIB_SUBDIR) eb libebutils ebappendix ebfont ebinfo ebrefile \
	ebzip doc $(JA_DOC_SUBDIR) $(SAMPLES_SUBDIR)

EXTRA_DIST = INSTALL-ja README-ja

appendixdir = $(pkgdatadir)/appendix
appendix_DATA =
