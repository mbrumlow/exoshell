#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([exoshell], [0.1], [exoshell@brumlow.io])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CC

# Use the C language and compiler for the following checks
AC_LANG([C])

# Checks for libraries.
AC_CHECK_LIB([ecl], [cl_boot])
AC_CHECK_LIB([gc], [GC_pthread_create])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([util], [forkpty])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h stdlib.h string.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([atexit memmove memset strchr strdup])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
