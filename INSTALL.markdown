# Installation from pecl

To pull latest stable released version, from [pecl](https://pecl.php.net/package/redis):

~~~
pecl install redis
~~~

# Installation from sources

To build this extension for the sources tree:

~~~
phpize
./configure [--enable-redis-igbinary] [--enable-redis-msgpack] [--enable-redis-lzf [--with-liblzf[=DIR]]] [--enable-redis-zstd]
make && make install
~~~

If you would like phpredis to serialize your data using the igbinary library, run configure with `--enable-redis-igbinary`.
If you would like to use the msgpack serializer, run configure with `--enable-redis-msgpack` (note:  Requires php-msgpack >= 2.0.3)
The extension also may compress data before sending it to Redis server, if you run configure with `--enable-redis-lzf`. If you want to use lzf library pre-installed into your system use `--with-liblzf` configuration option to specify the path where to search files.
`make install` copies `redis.so` to an appropriate location, but you still need to enable the module in the PHP config file. To do so, either edit your php.ini or add a redis.ini file in `/etc/php5/conf.d` with the following contents: `extension=redis.so`.

You can generate a debian package for PHP5, accessible from Apache 2 by running `./mkdeb-apache2.sh` or with `dpkg-buildpackage` or `svn-buildpackage`.

This extension exports a single class, [Redis](./README.markdown#class-redis) (and [RedisException](./README.markdown#class-redisexception) used in case of errors). Check out https://github.com/ukko/phpredis-phpdoc for a PHP stub that you can use in your IDE for code completion.


# Binary packages

Most distributions provides pre-build binary packages of this extension.

## Windows:

Follow the DLL link on the [https://pecl.php.net/package/redis](https://pecl.php.net/package/redis) page.

## Fedora

Fedora users can install the package from the official repository.

### Fedora ≥ 29, Version 5

Installation of the [php-pecl-redis5](https://apps.fedoraproject.org/packages/php-pecl-redis5) package:

~~~
dnf install php-pecl-redis5
~~~

## RHEL / CentOS

Installation of the [php-pecl-redis](https://apps.fedoraproject.org/packages/php-pecl-redis) package, from the [EPEL repository](https://fedoraproject.org/wiki/EPEL):

~~~
yum install php-pecl-redis
~~~

### openSUSE ≥ 15.1

Installation of the [php7-redis](https://software.opensuse.org/package/php7-redis?search_term=php7-redis) package:

~~~
zypper in php7-redis
~~~


# Installation on OSX

If the install fails on OSX, type the following commands in your shell before trying again:
~~~
MACOSX_DEPLOYMENT_TARGET=10.6
CFLAGS="-arch i386 -arch x86_64 -g -Os -pipe -no-cpp-precomp"
CCFLAGS="-arch i386 -arch x86_64 -g -Os -pipe"
CXXFLAGS="-arch i386 -arch x86_64 -g -Os -pipe"
LDFLAGS="-arch i386 -arch x86_64 -bind_at_load"
export CFLAGS CXXFLAGS LDFLAGS CCFLAGS MACOSX_DEPLOYMENT_TARGET
~~~

If that still fails and you are running Zend Server CE, try this right before "make": `./configure CFLAGS="-arch i386"`.

Taken from [Compiling phpredis on Zend Server CE/OSX ](http://www.tumblr.com/tagged/phpredis).

See also: [Install Redis & PHP Extension PHPRedis with Macports](http://www.lecloud.net/post/3378834922/install-redis-php-extension-phpredis-with-macports).

You can install it using MacPorts:

- [Get macports-php](https://www.macports.org/)
- `sudo port install php56-redis` (or php53-redis, php54-redis, php55-redis, php70-redis, php71-redis, php72-redis)

# Building on Windows

See [instructions from @char101](https://github.com/phpredis/phpredis/issues/213#issuecomment-11361242) on how to build phpredis on Windows.

