AC_PREREQ([2.69])
AC_INIT([suitcase], [0.1-dev], [https://github.org/halderen/suitcase/])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_MACRO_DIR([m4])
AC_USE_SYSTEM_EXTENSIONS
LT_INIT
ACX_PREFIXHACK
AC_CONFIG_SRCDIR([main.c])
AC_CONFIG_HEADERS([config.h])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AM_PROG_CC_C_O
ACX_PEDANTIC
ACX_STRICT
dnl CHECK_COMPILER_FLAG(std=c99, [C99FLAG="-std=c99"])
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h])
AC_CHECK_FUNCS([strrchr getopt_long])
AC_C_CONST
AC_TYPE_SIZE_T
AC_TYPE_UID_T
AC_TYPE_UINT16_T
AX_PTHREAD
AX_C___ATTRIBUTE__

AX_LIB_SQLITE3
AX_LIB_MYSQL
AX_WITH_LUA
AX_PROG_LUA
AX_LUA_HEADERS
AX_LUA_LIBS
AX_LUA_READLINE
AM_CONDITIONAL(HAVE_TCL, test x"$TCL" != x)

ACX_LIBMICROHTTPD
ACX_LIBJANSSON
ACX_LIBYAML
ACX_CUNIT
AM_CONDITIONAL([WITH_CUNIT], [test "${with_cunit}" != "no" -a -n "$CUNIT_LIBS"])
LIBCURL_CHECK_CONFIG

# doxygen
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN(${PACKAGE_NAME}, [Doxyfile], [doxygen])

AC_CHECK_LIB(pthread, pthread_barrier_wait, [AC_DEFINE([HAVE_PTHREAD_BARRIER], [1], [Define if pthread barrier supported])])

AC_CHECK_FUNC(backtrace, [AC_DEFINE([HAVE_BACKTRACE], [1], [Define if backtrace supported])])
AC_CHECK_LIB(backtrace, backtrace_full, [AC_DEFINE([HAVE_BACKTRACE_FULL], [1], [Define if backtrace_full supported]) LIBS="$LIBS -lbacktrace"])

AM_CONDITIONAL([HAVE_MYSQL],[grep HAVE_MYSQL confdefs.h > /dev/null])
AM_CONDITIONAL([HAVE_SQLITE3],[grep HAVE_SQLITE3 confdefs.h > /dev/null])
DYNAMIC_SQLITE3
AM_CONDITIONAL([INCLUDE_SQLITE3], [grep -E "HAVE_SQLITE3 |DO_DYNAMIC_SQLITE3" confdefs.h > /dev/null])
AM_CONDITIONAL([INCLUDE_MYSQL], [grep -E "HAVE_MYSQL |DO_DYNAMIC_MYSQL" confdefs.h > /dev/null])
AC_CHECK_HEADERS([sqlite3.h tcl.h])
AC_CHECK_HEADER([mysql/mysql.h], [AC_DEFINE([HAVE_MYSQL_H], 1, [Define to 1 if you have <mysql.h>.])], [])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
