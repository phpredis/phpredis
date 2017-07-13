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

dpkg -b deb phpredis-`git rev-parse --abbrev-ref HEAD`_`uname -m`.deb

rm -rf deb
