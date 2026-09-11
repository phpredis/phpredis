# Installation with PIE from packagist

To pull latest stable released version, from [packagist](https://packagist.org/packages/phpredis/phpredis)

~~~
pie install phpredis/phpredis
~~~

# Installation from pecl

To pull latest stable released version, from [pecl](https://pecl.php.net/package/redis)

~~~
pecl install redis
~~~

Configure options can be passed as well:

```bash
pecl install --configureoptions="enable-redis-msgpack='yes' enable-redis-igbinary='yes'" redis
```

# Installation from sources

To build this extension for the sources tree:

~~~
git clone https://github.com/phpredis/phpredis.git
cd phpredis
phpize
./configure [--enable-redis-igbinary] [--enable-redis-msgpack] [--enable-redis-lzf [--with-liblzf[=DIR]]] [--enable-redis-zstd] [--enable-redis-lz4]
make && make install
~~~

If you would like phpredis to serialize your data using the igbinary library, run configure with `--enable-redis-igbinary`.
If you would like to use the msgpack serializer, run configure with `--enable-redis-msgpack` (note:  Requires php-msgpack >= 2.0.3)
The extension also may compress data before sending it to Redis server, if you run configure with `--enable-redis-lzf`. If you want to use lzf library pre-installed into your system use `--with-liblzf` configuration option to specify the path where to search files.
`make install` copies `redis.so` to an appropriate location, but you still need to enable the module in the PHP config file. To do so, either edit your php.ini or add a redis.ini file in `/etc/php5/conf.d` with the following contents: `extension=redis.so`.

You can generate a debian package for PHP5, accessible from Apache 2 by running `./mkdeb-apache2.sh` or with `dpkg-buildpackage` or `svn-buildpackage`.

This extension exports a single class, [Redis](./README.md#class-redis) (and [RedisException](./README.md#class-redisexception) used in case of errors). Check out https://github.com/ukko/phpredis-phpdoc for a PHP stub that you can use in your IDE for code completion.


# Binary packages

Most distributions provides pre-build binary packages of this extension.

## Windows:

For releases with Windows ZIPs attached to the [GitHub release](https://github.com/phpredis/phpredis/releases), use:

```console
pie install phpredis/phpredis
```

PIE downloads the ZIP matching the target PHP version, thread safety, compiler,
and architecture, and installs its DLL. These packages target x64 PHP 8.0–8.5,
in both TS and NTS configurations. PIE itself requires PHP 8.1 or later; when
running it with a newer PHP, use `--with-php-path=C:\path\to\php.exe` to target
a different installation, including PHP 8.0.

The initial Windows packages include the PHP serializer and Redis session
handlers. They do not include igbinary, msgpack, JSON serialization, or
LZF/LZ4/Zstd compression. Configure options cannot add features to a prebuilt
DLL. Use `php --ri redis` to inspect the installed extension's features.

Older releases may not have GitHub Windows assets. For manual installation,
the [PECL DLL downloads](https://pecl.php.net/package/redis) and
[Windows PECL archive](https://windows.php.net/downloads/pecl/releases/redis/)
are also available.

### Publishing Windows packages (maintainers)

The [Windows workflow](.github/workflows/windows.yml) builds and smoke-tests
the ZIPs on pushes, pull requests, and manual runs. These runs keep packages
as workflow artifacts and do not upload them to a release.

Publishing a GitHub release (including a prerelease) runs the same matrix for
the release tag. After every build and smoke test succeeds, the workflow
attaches the packages to that release using `GITHUB_TOKEN`. Windows PIE
installs become available once the upload completes. The tag must contain
this workflow; existing tags are not backfilled automatically. Failed runs
can be rerun from Actions.

The workflow uses the PHP project's
[Windows builder and release actions](https://github.com/php/php-windows-builder),
which produce PIE's required ZIP and DLL names, for example
`php_redis-6.1.0-8.3-nts-vs16-x86_64.zip`. Keep the builder's default PIE naming
scheme; its PECL naming mode is not suitable here. No download URL override
in `composer.json` is needed.

This upload-after-publication workflow requires mutable GitHub releases.
If immutable releases are enabled, switch to the builder's
[draft-release workflow](https://github.com/php/php-windows-builder#workflow-for-tags)
so the assets are attached before publication.

Future optional dependency builds need a separate selection design: PIE's
Windows asset naming scheme has no feature-variant field. Adding ZIPs for
every configure-option combination would not by itself let PIE select one.

## Fedora

Fedora users can install the package from the official repository.

### Fedora ≥ 40, Version 6

Installation of the [php-pecl-redis6](https://src.fedoraproject.org/rpms/php-pecl-redis6/) package:

~~~
dnf install php-redis
~~~

## CentOS / RHEL and clones

Installation of the php-pecl-redis6 package, available for PHP ≥ 8.3:

~~~
dnf install php-redis
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
- `sudo port install php56-redis` (or php53-redis, php54-redis, php55-redis, php70-redis, php71-redis, php72-redis, php73-redis, php74-redis)

# Building on Windows

See [instructions from @char101](https://github.com/phpredis/phpredis/issues/213#issuecomment-11361242) on how to build phpredis on Windows.
