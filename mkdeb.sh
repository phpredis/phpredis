#!/bin/sh
phpize
./configure CFLAGS="-O3"
make clean all
DIR=`php-config --extension-dir | cut -c 2-`

mkdir -p deb
mkdir -p deb/DEBIAN
mkdir -p deb/$DIR

cp debian/* deb/DEBIAN/

cp modules/redis.so deb/$DIR

PHP_VERSION=`php-config --version | cut -d'-' -f1 | cut -d"." -f1,2`
export PHP_VERSION

PHP_REDIS_VERSION=`grep "#define PHP_REDIS_VERSION" php_redis.h | grep -Po '(?<=")([1-9a-z\.]+)(?=")'`
export PHP_REDIS_VERSION

cat > deb/DEBIAN/control <<EOF
Package: php$PHP_VERSION-redis
Version: $PHP_REDIS_VERSION
Section: web
Priority: optional
Architecture: all
Essential: no
Depends: php$PHP_VERSION-dev
Build-Depends:
Pre-Depends: php$PHP_VERSION-dev
Recommends: php$PHP_VERSION
Suggests:
Installed-Size:
Maintainer: Eugene Kirdzei [masterjus@gmail.com]
Conflicts: phpredis
Replaces: phpredis
Provides: php$PHP_VERSION-redis
Description: Redis C extension for PHP$PHP_VERSION
EOF

dpkg -b deb phpredis-$PHP_REDIS_VERSION-php$PHP_VERSION-`uname -m`.deb

rm -rf deb
