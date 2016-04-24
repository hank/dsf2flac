# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([dsf2flac], [1.0])
AC_CONFIG_SRCDIR([src/main.cpp])
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE([foreign])
AC_LANG(C++)

# Support for building libdstdec
AC_PROG_RANLIB

AC_CONFIG_MACRO_DIR(m4)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET

# Checks for libraries.
AM_PATH_LIBFLAC([], [
  AC_MSG_ERROR([Unable to find libFLAC])
])
AM_PATH_LIBFLACPP([], [
  AC_MSG_ERROR([Unable to find libFLAC++])
])
AX_LIB_ID3([], [
  AC_MSG_ERROR([Unable to find libid3])
])
AX_BOOST_BASE
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
