dnl $Id$
dnl config.m4 for extension redis

PHP_ARG_ENABLE(redis, whether to enable redis support,
dnl Make sure that the comment is aligned:
[  --enable-redis          Enable redis support])

PHP_ARG_ENABLE(redis-session, whether to enable sessions,
[  --disable-redis-session Disable session support], yes, no)

PHP_ARG_ENABLE(redis-igbinary, whether to enable igbinary serializer support,
[  --enable-redis-igbinary Enable igbinary serializer support], no, no)

PHP_ARG_ENABLE(redis-lzf, whether to enable lzf compression,
[  --enable-redis-lzf      Enable lzf compression support], no, no)

PHP_ARG_WITH(liblzf, use system liblzf,
[  --with-liblzf[=DIR]       Use system liblzf], no, no)

if test "$PHP_REDIS" != "no"; then

  if test "$PHP_REDIS_SESSION" != "no"; then
    AC_DEFINE(PHP_SESSION,1,[redis sessions])
  fi

dnl Check for igbinary
  if test "$PHP_REDIS_IGBINARY" != "no"; then
    AC_MSG_CHECKING([for igbinary includes])
    igbinary_inc_path=""

    if test -f "$abs_srcdir/include/php/ext/igbinary/igbinary.h"; then
      igbinary_inc_path="$abs_srcdir/include/php"
    elif test -f "$abs_srcdir/ext/igbinary/igbinary.h"; then
      igbinary_inc_path="$abs_srcdir"
    elif test -f "$phpincludedir/ext/igbinary/igbinary.h"; then
      igbinary_inc_path="$phpincludedir"
    else
      for i in php php4 php5 php6; do
        if test -f "$prefix/include/$i/ext/igbinary/igbinary.h"; then
          igbinary_inc_path="$prefix/include/$i"
        fi
      done
    fi

    if test "$igbinary_inc_path" = ""; then
      AC_MSG_ERROR([Cannot find igbinary.h])
    else
      AC_MSG_RESULT([$igbinary_inc_path])
    fi
  fi

  AC_MSG_CHECKING([for redis igbinary support])
  if test "$PHP_REDIS_IGBINARY" != "no"; then
    AC_MSG_RESULT([enabled])
    AC_DEFINE(HAVE_REDIS_IGBINARY,1,[Whether redis igbinary serializer is enabled])
    IGBINARY_INCLUDES="-I$igbinary_inc_path"
    IGBINARY_EXT_DIR="$igbinary_inc_path/ext"
    ifdef([PHP_ADD_EXTENSION_DEP],
    [
      PHP_ADD_EXTENSION_DEP(redis, igbinary)
    ])
    PHP_ADD_INCLUDE($IGBINARY_EXT_DIR)
  else
    IGBINARY_INCLUDES=""
    AC_MSG_RESULT([disabled])
  fi

  if test "$PHP_REDIS_LZF" != "no"; then
    AC_DEFINE(HAVE_REDIS_LZF, 1, [ ])
    if test "$PHP_LIBLZF" != "no"; then
      AC_MSG_CHECKING(for liblzf files in default path)
      for i in $PHP_LIBLZF /usr/local /usr; do
        if test -r $i/include/lzf.h; then
          AC_MSG_RESULT(found in $i)
          LIBLZF_DIR=$i
          break
        fi
      done
      if test -z "$LIBLZF_DIR"; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([Please reinstall the liblzf distribution])
      fi
      PHP_CHECK_LIBRARY(lzf, lzf_compress,
      [
        PHP_ADD_LIBRARY_WITH_PATH(lzf, $LIBLZF_DIR/$PHP_LIBDIR, REDIS_SHARED_LIBADD)
      ], [
        AC_MSG_ERROR([could not find usable liblzf])
      ], [
        -L$LIBLZF_DIR/$PHP_LIBDIR
      ])
      PHP_SUBST(REDIS_SHARED_LIBADD)
    else
      PHP_ADD_INCLUDE(liblzf)
      PHP_ADD_INCLUDE($srcdir/liblzf)
      PHP_ADD_BUILD_DIR(liblzf)
      lzf_sources="liblzf/lzf_c.c liblzf/lzf_d.c"
    fi
  fi

  AC_CHECK_PROG([GIT], [git], [yes], [no])
  if test "$GIT" == "yes" && test -d "$srcdir/.git"; then
    AC_DEFINE_UNQUOTED(GIT_REVISION, ["$(git log -1 --format=%H)"], [ ])
  fi

  dnl # --with-redis -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/redis.h"  # you most likely want to change this
  dnl if test -r $PHP_REDIS/$SEARCH_FOR; then # path given as parameter
  dnl   REDIS_DIR=$PHP_REDIS
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for redis files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       REDIS_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$REDIS_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the redis distribution])
  dnl fi

  dnl # --with-redis -> add include path
  dnl PHP_ADD_INCLUDE($REDIS_DIR/include)

  dnl # --with-redis -> check for lib and symbol presence
  dnl LIBNAME=redis # you may want to change this
  dnl LIBSYMBOL=redis # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $REDIS_DIR/lib, REDIS_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_REDISLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong redis lib version or lib not found])
  dnl ],[
  dnl   -L$REDIS_DIR/lib -lm -ldl
  dnl ])
  dnl
  dnl PHP_SUBST(REDIS_SHARED_LIBADD)

  PHP_NEW_EXTENSION(redis, redis.c redis_commands.c library.c redis_session.c redis_array.c redis_array_impl.c redis_cluster.c cluster_library.c $lzf_sources, $ext_shared)
fi
