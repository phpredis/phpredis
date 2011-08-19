dnl $Id$
dnl config.m4 for extension redis

PHP_ARG_ENABLE(redis, whether to enable redis support,
dnl Make sure that the comment is aligned:
[  --enable-redis           Enable redis support])

PHP_ARG_ENABLE(redis-session, whether to enable sessions,
[  --disable-redis-session      Disable session support], yes, no)



if test "$PHP_REDIS" != "no"; then

  if test "$PHP_REDIS_SESSION" != "no"; then
    AC_DEFINE(PHP_SESSION,1,[redis sessions])
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

  PHP_NEW_EXTENSION(redis, redis.c library.c redis_session.c redis_array.c redis_array_impl.c igbinary/igbinary.c igbinary/hash_si.c igbinary/hash_function.c, $ext_shared)
fi
