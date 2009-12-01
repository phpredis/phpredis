#!/bin/sh
phpize
./configure CFLAGS="-O3" 
make clean all
DIR=`php-config --extension-dir | cut -c 2-`

rm -rf debian

mkdir -p debian
mkdir -p debian/DEBIAN
mkdir -p debian/$DIR

cp debian.control debian/DEBIAN/control

mkdir -p debian/etc/php5/apache2/conf.d/
mkdir -p debian/etc/php5/cli/conf.d/

echo "extension=redis.so" >> debian/etc/php5/apache2/conf.d/redis.ini

cp debian/etc/php5/apache2/conf.d/redis.ini debian/etc/php5/cli/conf.d/redis.ini

cp modules/redis.so debian/$DIR
dpkg -b debian phpredis-`uname -m`.deb
rm -rf debian/
