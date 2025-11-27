# Installation options

phpredis can be installed from PECL, from source, or with pre-built packages provided by most distributions.

## PIE (PHP Installer for Extensions)

If you use [PIE](https://github.com/cuaxin/pie) to install PHP extensions from [Packagist](https://packagist.org/packages/phpredis/phpredis):

```bash
pie install phpredis/phpredis
```

You can also install a specific tag:

```bash
pie install phpredis/phpredis:6.2.0
pie install phpredis/phpredis:6.3.0RC1
```
**NOTE**: PIE is recommended over PECL for installing extensions.

## PECL

Pull the latest stable release from [PECL](https://pecl.php.net/package/redis):

```bash
pecl install redis
```

Configure options can be passed as well:

```bash
pecl install --configureoptions="enable-redis-msgpack='yes' enable-redis-igbinary='yes'" redis
```

## Build from source

```bash
git clone https://github.com/phpredis/phpredis.git
cd phpredis
phpize
./configure [--enable-redis-igbinary] [--enable-redis-msgpack] [--enable-redis-lzf [--with-liblzf[=DIR]]] [--enable-redis-zstd] [--enable-redis-lz4]
make && make install
```

- `--enable-redis-igbinary` enables igbinary serialization.
- `--enable-redis-msgpack` enables msgpack serialization (requires php-msgpack >= 2.0.3).
- `--enable-redis-lzf` / `--with-liblzf` enable LZF compression.
- `--enable-redis-zstd` / `--enable-redis-lz4` enable additional compression codecs.

After `make install`, enable the extension (for example `echo "extension=redis.so" > /etc/php.d/redis.ini`) and confirm with `php -m | grep redis`.

# Binary packages

Most distributions provide a packaged extension. Package names vary by PHP version; replace the PHP version in the command if needed.

- Debian / Ubuntu: `sudo apt install php-redis`
- Fedora: `sudo dnf install php-redis` (provided as php-pecl-redis)
- RHEL / CentOS / Alma / Rocky: enable EPEL/Remi if required, then `sudo dnf install php-pecl-redis`
- openSUSE Leap / Tumbleweed: `sudo zypper install php8-redis` (or matching PHP slot)
- Arch Linux / Manjaro: `sudo pacman -S php-redis`
- Alpine: `sudo apk add php82-redis` (replace 82 with your PHP minor version)
- macOS (Homebrew): `brew install php && pecl install redis`
- Windows: download the DLL from [PECL](https://pecl.php.net/package/redis) or [windows.php.net](https://windows.php.net/downloads/pecl/releases/redis/)
- MacPorts: `sudo port install php82-redis` (or the desired PHP slot)

# Notes for Docker

If you are building on the official `php` Docker images with Debian/Alpine bases, you can install via PECL and enable the extension:

```bash
pecl install redis \
  && docker-php-ext-enable redis
```

# Additional resources

This extension exports [Redis](./README.md#class-redis) and [RedisException](./README.md#class-redisexception). A PHP stub for IDE completion is available at https://github.com/ukko/phpredis-phpdoc.
