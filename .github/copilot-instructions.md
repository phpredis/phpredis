# GitHub Copilot Instructions for PhpRedis

> Repository-wide instructions for GitHub Copilot (agent, chat, and review).
> Keep changes small, reversible, and aligned with PhpRedis practices.

## What this project is

PhpRedis is a PHP extension (C + PHP test suites) for Redis/Valkey/KeyDB.
Project homepage: https://github.com/phpredis/phpredis

## Golden rules for Copilot

1. **Never commit build outputs** or local environment files.
   - Do **not** add or modify:
     - `redis.la`, `*.lo`, `*.o`, `*.a`, `*.so`, `*.dylib`, `*.dll`
     - `modules/`, `modules/*`
     - `autom4te.cache/`, `build/`, `Release/`, `Debug/`
     - generated `configure`, `config.log`, `config.status`, `libtool`
     - `Makefile*`, `install-sh`, `ltmain.sh`, `aclocal.m4`
     - OS/editor temp files (`.DS_Store`, swap files, etc.)
     - any files under `tests/nodes/*` created locally (e.g. `*.conf`)
2. **Do not run `phpize && ./configure && make` and commit results.**
   - You may build **locally** to validate, but **never** commit outputs.
3. **Do not “fix” `.gitignore`** to hide accidental commits. Revert them.
4. **Do not edit generated headers** (e.g., `*_arginfo.h`) by hand. Edit
   the corresponding `*.stub.php` and regenerate locally when needed.
5. **Prefer minimal, surgical PRs** with focused scope and tests.
6. **Follow project style** and avoid mass reformatting.

## Build (local validation only; do not commit outputs)

```sh
phpize
./configure --enable-redis
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu || echo 2)"
# Optional quick load check (path depends on OS/build):
php -dextension="$(pwd)/modules/redis.so" --ri redis
```

## Stub files

Whenever any of our `*.stub.php` files are modified, they'll need to be
regenerated with PHP's `gen_stub.php` script. When they are regenerated
both the `*.stub.php` and corresponding `*_arginfo` files *should* be committed.

`gen_stub.php` can be downloaded from public locations but is also included in
any non-eol system wide PHP version.

On a system with `php` and `php-config` one can typically run it like so:

```sh
# Run inside of the `phpredis` source directory:
# On Linux it's typically here
php $(php-config --extension-dir)/build/gen_stub.php *.stub.php

# Run inside of the `phpredis` source directory:
# On macOS it's typically here
php $(php-config --prefix)/lib/php/build/gen_stub.php *.stub.php
```

## Optional PhpRedis features

PhpRedis has several optional features including `lz4`, `lzf`, `zstd`,
`igbinary`, and `msgpack` support. Each requires their respective dependencies
although liblzf is included as a submodule so can just be pulled that way.

Build PhpRedis with every option like so:

```sh
phpize
./configure \
    --enable-redis-zstd \
    --enable-redis-lzf \
    --enable-redis-lz4 \
    --enable-redis-msgpack \
    --enable-redis-igbinary
    # --with-liblzf can be used to use system-wide liblzf
make -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu || echo 2)"
php -dextension="$(pwd)/modules/redis.so" --ri redis
```

## Running tests

PhpRedis uses a bespoke test harness. Redis just needs to be running locally.
Running the tests is simple and described below.

```sh
# For standalone Redis
php tests/TestRedis.php --class Redis [--port <port>]
php tests/TestRedis.php --class RedisCluster [--port <any-cluster-seed-port>]
php tests/TestRedis.php --class RedisSentinel [--port <sentinel-port>]
```

### Running specific tests

With any of the above test invocations you can limit to a specific test with the
`--test <name>` option. This is a substring match so for example

```sh
# Will run any test with "get" in the name
php tests/TestRedis.php --class Redis --test get
```

### Regression tests

Where appropriate adding a regression test and a comment with the issue number
is preferred. In most cases you only need to add the test to `RedisTest` and the
cluster tests will by default also run it.

Remember that if the command takes multiple keys they all have to hash to the
same slot in cluster mode, so you can do this by picking key names like
`{foo}1`, `{foo}2`, etc.

## Existing PhpRedis CI workflows

Building and running the tests can be useful for any PR but PhpRedis already has
a very extensive CI setup which will be triggered on PRs. It is not necessary to
replicate this with extensive local tests unless there is a good reason.
