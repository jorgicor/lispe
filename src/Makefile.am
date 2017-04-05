AM_CFLAGS = $(WARN_CFLAGS)

bin_PROGRAMS = lispe

lispe_CFLAGS = $(AM_CFLAGS)

lispe_CPPFLAGS = -DPP_DATADIR='"$(pkgdatadir)"'

lispe_SOURCES = lispe.c cfg.h cbase.h gc.h common.h \
		err.h err.c \
		cellmark.c cellmark.h \
		cells.c cells.h \
		lex.c lex.h \
		numbers.c numbers.h \
		symbols.c symbols.h \
		sexpr.c sexpr.h \
		gcbase.c parse.c pred.c env.c
