# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dsf2flac], [1.0])
AC_CONFIG_SRCDIR([src/main.cpp])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign -Wall])
AM_PROG_AR
LT_INIT

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG()

AC_HEADER_STDC

# Checks for libraries.
AX_LIB_ID3([],
	[AC_MSG_ERROR([libid3 required, but it was not found in your system])])
AM_PATH_LIBFLAC([],
	[AC_MSG_ERROR([libflac required, but it was not found in your system])]])
AM_PATH_LIBFLACPP([],
	[AC_MSG_ERROR([libflac++ required, but it was not found in your system])]])
PKG_CONFIG_CHECK_MODULE([taglib],[1.6])
AX_BOOST_BASE([1.48],,
	[AC_MSG_ERROR([Boost required, but it was not found in your system])])
AX_BOOST_FILESYSTEM
AX_BOOST_SYSTEM
AX_BOOST_TIMER

# Checks for header files.
AC_CHECK_HEADERS([malloc.h memory.h stdint.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([memset pow strtol])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/libdstdec/Makefile])
AC_OUTPUT
