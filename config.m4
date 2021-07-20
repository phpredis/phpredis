dnl $Id$
dnl config.m4 for extension redis

PHP_ARG_ENABLE(redis, whether to enable redis support,
dnl Make sure that the comment is aligned:
[  --enable-redis               Enable redis support])

PHP_ARG_ENABLE(redis-session, whether to enable sessions,
[  --disable-redis-session      Disable session support], yes, no)

PHP_ARG_ENABLE(redis-json, whether to enable json serializer support,
[  --disable-redis-json         Disable json serializer support], yes, no)

PHP_ARG_ENABLE(redis-igbinary, whether to enable igbinary serializer support,
[  --enable-redis-igbinary Enable igbinary serializer support], no, no)

PHP_ARG_ENABLE(redis-msgpack, whether to enable msgpack serializer support,
[  --enable-redis-msgpack       Enable msgpack serializer support], no, no)

PHP_ARG_ENABLE(redis-lzf, whether to enable lzf compression,
[  --enable-redis-lzf      Enable lzf compression support], no, no)

PHP_ARG_WITH(liblzf, use system liblzf,
[  --with-liblzf[=DIR]       Use system liblzf], no, no)

PHP_ARG_ENABLE(redis-zstd, whether to enable Zstd compression,
[  --enable-redis-zstd     Enable Zstd compression support], no, no)

PHP_ARG_WITH(libzstd, use system libsztd,
[  --with-libzstd[=DIR]      Use system libzstd], yes, no)

PHP_ARG_ENABLE(redis-lz4, whether to enable lz4 compression,
[  --enable-redis-lz4      Enable lz4 compression support], no, no)

PHP_ARG_WITH(liblz4, use system liblz4,
[  --with-liblz4[=DIR]       Use system liblz4], no, no)

if test "$PHP_REDIS" != "no"; then

  if test "$PHP_REDIS_SESSION" != "no"; then
    AC_DEFINE(PHP_SESSION,1,[redis sessions])
  fi

  AC_MSG_CHECKING([for hash includes])
  hash_inc_path=""
  if test -f "$abs_srcdir/include/php/ext/hash/php_hash.h"; then
    hash_inc_path="$abs_srcdir/include/php"
  elif test -f "$abs_srcdir/ext/hash/php_hash.h"; then
    hash_inc_path="$abs_srcdir"
  elif test -f "$phpincludedir/ext/hash/php_hash.h"; then
    hash_inc_path="$phpincludedir"
  else
    for i in php php7; do
      if test -f "$prefix/include/$i/ext/hash/php_hash.h"; then
        hash_inc_path="$prefix/include/$i"
      fi
    done
  fi

  if test "$hash_inc_path" = ""; then
    AC_MSG_ERROR([Cannot find php_hash.h])
  else
    AC_MSG_RESULT([$hash_inc_path])
  fi

  if test "$PHP_REDIS_JSON" != "no"; then
    AC_MSG_CHECKING([for json includes])
    json_inc_path=""
    if test -f "$abs_srcdir/include/php/ext/json/php_json.h"; then
      json_inc_path="$abs_srcdir/include/php"
    elif test -f "$abs_srcdir/ext/json/php_json.h"; then
      json_inc_path="$abs_srcdir"
    elif test -f "$phpincludedir/ext/json/php_json.h"; then
      json_inc_path="$phpincludedir"
    else
      for i in php php7; do
        if test -f "$prefix/include/$i/ext/json/php_json.h"; then
          json_inc_path="$prefix/include/$i"
        fi
      done
    fi

    if test "$json_inc_path" = ""; then
      AC_MSG_ERROR([Cannot find php_json.h])
    else
      AC_MSG_RESULT([$json_inc_path])
    fi
  fi

  AC_MSG_CHECKING([for redis json support])
  if test "$PHP_REDIS_JSON" != "no"; then
    AC_MSG_RESULT([enabled])
    AC_DEFINE(HAVE_REDIS_JSON,1,[Whether redis json serializer is enabled])
    JSON_INCLUDES="-I$json_inc_path"
    JSON_EXT_DIR="$json_inc_path/ext"
    ifdef([PHP_ADD_EXTENSION_DEP],
    [
      PHP_ADD_EXTENSION_DEP(redis, json)
    ])
    PHP_ADD_INCLUDE($JSON_EXT_DIR)
  else
    JSON_INCLUDES=""
    AC_MSG_RESULT([disabled])
  fi

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
      for i in php php7; do
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

  if test "$PHP_REDIS_MSGPACK" != "no"; then
    AC_MSG_CHECKING([for msgpack includes])
    msgpack_inc_path=""

    if test -f "$abs_srcdir/include/php/ext/msgpack/php_msgpack.h"; then
      msgpack_inc_path="$abs_srcdir/include/php"
    elif test -f "$abs_srcdir/ext/msgpack/php_msgpack.h"; then
      msgpack_inc_path="$abs_srcdir"
    elif test -f "$phpincludedir/ext/msgpack/php_msgpack.h"; then
      msgpack_inc_path="$phpincludedir"
    else
      for i in php php7; do
        if test -f "$prefix/include/$i/ext/msgpack/php_msgpack.h"; then
          msgpack_inc_path="$prefix/include/$i"
        fi
      done
    fi

    if test "$msgpack_inc_path" = ""; then
      AC_MSG_ERROR([Cannot find php_msgpack.h])
    else
      AC_MSG_RESULT([$msgpack_inc_path])
    fi
  fi

  if test "$PHP_REDIS_MSGPACK" != "no"; then
    AC_MSG_CHECKING([for php msgpack version >= 2.0.3])
    MSGPACK_VERSION=`$EGREP "define PHP_MSGPACK_VERSION" $msgpack_inc_path/ext/msgpack/php_msgpack.h | $SED -e 's/[[^0-9\.]]//g'`
    if test `echo $MSGPACK_VERSION | $SED -e 's/[[^0-9]]/ /g' | $AWK '{print $1*1000 + $2*100 + $3*10 + $4}'` -lt 2030; then
      AC_MSG_ERROR([version >= 2.0.3 required])
    else
      AC_MSG_RESULT([yes])
    fi

    AC_DEFINE(HAVE_REDIS_MSGPACK,1,[Whether redis msgpack serializer is enabled])
    MSGPACK_INCLUDES="-I$msgpack_inc_path"
    MSGPACK_EXT_DIR="$msgpack_inc_path/ext"
    ifdef([PHP_ADD_EXTENSION_DEP],
    [
      PHP_ADD_EXTENSION_DEP(redis, msgpack)
    ])
    PHP_ADD_INCLUDE($MSGPACK_EXT_DIR)
  else
    MSGPACK_INCLUDES=""
  fi

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)

  if test "$PHP_REDIS_LZF" != "no"; then
    AC_DEFINE(HAVE_REDIS_LZF, 1, [ ])

    if test "$PHP_LIBLZF" = "yes" && test -x "$PKG_CONFIG" && $PKG_CONFIG --exists liblzf; then
      AC_MSG_CHECKING(for liblzf using pkg-config)

      LIBLZF_INC=`$PKG_CONFIG liblzf --cflags`
      LIBLZF_LIB=`$PKG_CONFIG liblzf --libs`
      LIBLZF_VER=`$PKG_CONFIG liblzf --modversion`
      AC_MSG_RESULT(found version $LIBLZF_VER)
      PHP_EVAL_LIBLINE($LIBLZF_LIB, REDIS_SHARED_LIBADD)
      PHP_EVAL_INCLINE($LIBLZF_INC)

    elif test "$PHP_LIBLZF" != "no"; then
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
    else
      PHP_ADD_INCLUDE(liblzf)
      PHP_ADD_INCLUDE($srcdir/liblzf)
      PHP_ADD_BUILD_DIR(liblzf)
      lzf_sources="liblzf/lzf_c.c liblzf/lzf_d.c"
    fi
  fi

  if test "$PHP_REDIS_LZ4" != "no"; then
    AC_DEFINE(HAVE_REDIS_LZ4, 1, [ ])

    if test "$PHP_LIBLZ4" = "yes" && test -x "$PKG_CONFIG" && $PKG_CONFIG --exists liblz4; then
      AC_MSG_CHECKING(for liblz4 using pkg-config)

      LIBLZ4_VER=`$PKG_CONFIG liblz4 --modversion`
      LIBLZ4_INC=`$PKG_CONFIG liblz4 --cflags`
      LIBLZ4_LIB=`$PKG_CONFIG liblz4 --libs`
      AC_MSG_RESULT(found version $LIBLZ4_VER)
      PHP_EVAL_LIBLINE($LIBLZ4_LIB, REDIS_SHARED_LIBADD)
      PHP_EVAL_INCLINE($LIBLZ4_INC)

    elif test "$PHP_LIBLZ4" != "no"; then
      AC_MSG_CHECKING(for liblz4 files in default path)
      for i in $PHP_LIBLZ4 /usr/local /usr; do
        if test -r $i/include/lz4.h; then
          AC_MSG_RESULT(found in $i)
          LIBLZ4_DIR=$i
          break
        fi
      done
      if test -z "$LIBLZ4_DIR"; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([Please reinstall the liblz4 distribution])
      fi
      PHP_CHECK_LIBRARY(lz4, LZ4_compress,
      [
        PHP_ADD_LIBRARY_WITH_PATH(lz4, $LIBLZ4_DIR/$PHP_LIBDIR, REDIS_SHARED_LIBADD)
      ], [
        AC_MSG_ERROR([could not find usable liblz4])
      ], [
        -L$LIBLZ4_DIR/$PHP_LIBDIR
      ])
      PHP_SUBST(REDIS_SHARED_LIBADD)
    else
      AC_MSG_ERROR([only system liblz4 is supported])
    fi
  fi

  if test "$PHP_REDIS_ZSTD" != "no"; then
    AC_DEFINE(HAVE_REDIS_ZSTD, 1, [ ])

    if test "$PHP_LIBZSTD" = "yes" && test -x "$PKG_CONFIG" && $PKG_CONFIG --exists libzstd; then
      AC_MSG_CHECKING(for libzstd using pkg-config)

      LIBZSTD_VER=`$PKG_CONFIG libzstd --modversion`
      if $PKG_CONFIG libzstd --atleast-version 1.3.0; then
        LIBZSTD_INC=`$PKG_CONFIG libzstd --cflags`
        LIBZSTD_LIB=`$PKG_CONFIG libzstd --libs`
        AC_MSG_RESULT(found version $LIBZSTD_VER)
        PHP_EVAL_LIBLINE($LIBZSTD_LIB, REDIS_SHARED_LIBADD)
        PHP_EVAL_INCLINE($LIBZSTD_INC)
      else
        AC_MSG_ERROR([found version $LIBZSTD_VER, version 1.3.0 required])
      fi

    elif test "$PHP_LIBZSTD" != "no"; then
      AC_MSG_CHECKING(for libzstd files in default path)
      for i in $PHP_LIBZSTD /usr/local /usr; do
        if test -r $i/include/zstd.h; then
          AC_MSG_RESULT(found in $i)
          LIBZSTD_DIR=$i
          break
        fi
      done
      if test -z "$LIBZSTD_DIR"; then
        AC_MSG_RESULT([not found])
        AC_MSG_ERROR([Please reinstall the libzstd distribution])
      fi
      PHP_CHECK_LIBRARY(zstd, ZSTD_getFrameContentSize,
      [
        PHP_ADD_LIBRARY_WITH_PATH(zstd, $LIBZSTD_DIR/$PHP_LIBDIR, REDIS_SHARED_LIBADD)
      ], [
        AC_MSG_ERROR([could not find usable libzstd, version 1.3.0 required])
      ], [
        -L$LIBZSTD_DIR/$PHP_LIBDIR
      ])
    else
      AC_MSG_ERROR([only system libzstd is supported])
    fi
  fi

  AC_CHECK_PROG([GIT], [git], [yes], [no])
  if test "$GIT" = "yes" && test -d "$srcdir/.git"; then
    AC_DEFINE_UNQUOTED(GIT_REVISION, ["$(git log -1 --format=%H)"], [ ])
  fi

  PHP_SUBST(REDIS_SHARED_LIBADD)
  PHP_NEW_EXTENSION(redis, redis.c redis_commands.c library.c redis_session.c redis_array.c redis_array_impl.c redis_cluster.c cluster_library.c redis_sentinel.c sentinel_library.c backoff.c $lzf_sources, $ext_shared)
fi
