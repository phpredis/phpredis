dnl $Id: config.m4,v 1.7 2008/07/03 17:05:57 phadej Exp $
dnl config.m4 for extension igbinary

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(igbinary, for igbinary support,
dnl Make sure that the comment is aligned:
dnl [  --with-igbinary             Include igbinary support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(igbinary, whether to enable igbinary support,
	[  --enable-igbinary          Enable igbinary support])

if test "$PHP_IGBINARY" != "no"; then
  AC_CHECK_HEADERS([stdbool.h],, AC_MSG_ERROR([stdbool.h not exists]))
  AC_CHECK_HEADERS([stddef.h],, AC_MSG_ERROR([stddef.h not exists]))
  AC_CHECK_HEADERS([stdint.h],, AC_MSG_ERROR([stdint.h not exists]))

  AC_CHECK_SIZEOF([long])

  dnl GCC
	AC_MSG_CHECKING(compiler type)
	if test ! -z "`$CC --version | grep -i GCC`"; then
	  AC_MSG_RESULT(gcc)
		PHP_IGBINARY_CFLAGS="-Wall -Wpointer-arith -Wmissing-prototypes -Wstrict-prototypes -Wcast-align -Wshadow -Wwrite-strings -Wswitch -Winline -finline-limit=10000 --param large-function-growth=10000 --param inline-unit-growth=10000"
	elif test ! -z "`$CC --version | grep -i ICC`"; then
	  AC_MSG_RESULT(icc)
		PHP_IGBINARY_CFLAGS="-no-prec-div -O3 -x0 -unroll2"
	else
	  AC_MSG_RESULT(other)
	fi

  PHP_INSTALL_HEADERS([ext/igbinary], [igbinary.h])
  PHP_NEW_EXTENSION(igbinary, igbinary.c hash_si.c hash_function.c, $ext_shared,, $PHP_IGBINARY_CFLAGS)
fi
