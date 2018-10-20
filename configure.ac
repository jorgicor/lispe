#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# ===========================================================================
# lispe, Scheme interpreter.
# ===========================================================================

AC_INIT([lispe],
	[0.99],
	[jorge.giner@hotmail.com],
	lispe,
	[https://jorgicor.niobe.org/lispe])

AC_CONFIG_MACRO_DIR([m4])

AH_TEMPLATE([PP_RANGECHECKS],
	    [Generate code to check the range on array indices])
AC_ARG_ENABLE(rangechecks,
	      AS_HELP_STRING([--enable-rangechecks],
		     [check the range on array indices @<:@default=yes@:>@]),
		     [], [enable_rangechecks=yes])

AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(-Wall -Werror -Wportability subdir-objects
		 color-tests parallel-tests)
AC_CONFIG_HEADERS([config.h])

AC_DEFINE([COPYRIGHT_YEARS], ["2018"], [Years of copyright])
AC_SUBST(COPYRIGHT_YEARS, [2018])

# Checks for programs.
AC_PROG_CC
# AM_MISSING_PROG(HELP2MAN, help2man, $missing_dir)

# Supported flags.
AX_CHECK_COMPILE_FLAG([-Wall], [WARN_CFLAGS="-Wall"], [], [])
AX_CHECK_COMPILE_FLAG([-Wdeclaration-after-statement],
	      [WARN_CFLAGS="$WARN_CFLAGS -Wdeclaration-after-statement"],
	      [], [])
AC_SUBST(WARN_CFLAGS)

# Checks for header files.
AC_CHECK_HEADERS([tgmath.h])

# Checks for libraries.
AC_SEARCH_LIBS([pow],[m])

# Checks for typedefs, structures, and compiler characteristics.
if test "${enable_rangechecks}" = yes; then
	AC_DEFINE([PP_RANGECHECKS])
fi

# Checks for library functions.

AC_CONFIG_FILES([Makefile
		 src/Makefile])
AC_OUTPUT
