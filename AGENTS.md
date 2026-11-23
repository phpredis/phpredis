## Overview

This project is PhpRedis, a PHP extension that exposes Redis, RedisCluster,
RedisArray, and RedisSentinel APIs.

### Project goals and constraints

- PhpRedis is a widely used C extension loaded into production PHP processes.
  Correctness and clarity are of utmost importance. The extension cannot crash
  in production because it could take down an entire PHP-FPM pool and cause
  customer downtime.
- After correctness, performance matters. Hot paths such as command packing,
  reply parsing, serializer/compressor handling, persistent connection reuse,
  and cluster routing should avoid unnecessary allocations and repeated work.
- Preserve the public PHP API and wire-protocol behavior unless the change is
  intentionally compatibility-breaking. Many users rely on subtle historical
  behavior.
- Prefer modular and composable code over duplicating logic. Given that this
  project is in C, that can typically be done without any performance cost.
- Be careful with ownership, refcounts, persistent allocations, and error paths.
  Most bugs in extensions become leaks, use-after-free defects, or process
  crashes.
- The PHP API surface can be found in the stubs under `*.stub.php`; generated
  arginfo headers should remain consistent with those stubs.
- This project supports optional serializers and compression backends. Changes
  should account for the combinations enabled by `config.m4`, especially JSON,
  igbinary, msgpack, lzf, lz4, zstd, and session support.
- For performance-critical code, it is often worth looking at codegen and
  trying more than one formulation. This can be done with `objdump -d` or by
  using specific compiler flags, such as `-S`.

### Commit messages

When committing code, hard-wrap commit message bodies at 80 characters rather
than relying on display wrapping. Explicit line breaks render more consistently
across platforms, matching the wrapping often provided by Git editor plugins
in Vim and Neovim.

### Publishing PECL releases

PECL uploads can reject valid UTF-8 release notes with `SQLSTATE[22007]`,
`1366 Incorrect string value`, and a reference to `releases.releasenotes`.
The accompanying "Invalid datetime format" text is misleading: this can be
a server-side character encoding limitation, not an XML or release date error.
For 6.4.0RC1, a non-ASCII contributor name triggered this failure; ASCII notes
allowed the release to be published.

- Keep release notes in `package.xml` ASCII-safe, including the matching notes
  under `<changelog>`. Use contributors' ASCII GitHub handles when their names
  contain non-ASCII characters, and use straight quotes and apostrophes.
- Preserve Unicode names in `CHANGELOG.md`, Git history, and annotated tags;
  the PECL workaround belongs in the package metadata.
- XML character references do not work around this issue: the XML parser
  decodes them back into Unicode before the database insert.
- After editing `package.xml`, regenerate the upload archive with
  `pecl package package.xml`. Check the parsed notes inside the resulting
  archive for non-ASCII characters and verify that the current release notes
  and their changelog entry agree, allowing for packaging whitespace changes.
  Valid XML and successful local packaging alone do not guarantee PECL will
  accept the notes.

### C style guide

- Do not add casts unless they are required for correctness, ABI compatibility,
  aliasing, intentional narrowing, or to change arithmetic semantics (e.g.
  forcing floating-point arithmetic). Avoid documentation/performative casts
  that merely restate the compiler's implicit conversions.
- Keep error handling explicit and local. If a function returns ownership or
  borrows memory, make that obvious from the surrounding code and naming.
- Prefer existing helpers in `library.c`, `redis_commands.c`, cluster helpers,
  and session code over adding a second ad hoc implementation of the same
  parsing, packing, or connection behavior.

### Building and running the modified project

- Before choosing a build, run, or test command, check whether `.agents.md`
  exists in the repository root. If it exists, read it and follow any
  user-specific local instructions in addition to this file.
- `.agents.md` is for local developer notes such as custom build scripts,
  local PHP source tree layouts, debugger helpers, or machine-specific
  configuration. It is intentionally ignored by git and must not be committed.
- Before building, agents should assume they need to run `phpize` and then
  `./configure` from the repository root. Do not assume a checked-in
  `Makefile` is current.
- Agents can assume the default `phpize`, `php-config`, and PHP development
  headers are already available.
- A broad configure flow for agents is:

```bash
phpize
./configure \
  --enable-redis \
  --enable-redis-igbinary \
  --enable-redis-msgpack \
  --enable-redis-lzf \
  --enable-redis-lz4 \
  --enable-redis-zstd \
  --with-liblz4=/usr \
  --with-libzstd=/usr
make
```

- Session support and JSON serializer support are enabled by default. Use
  `--disable-redis-session` or `--disable-redis-json` only when a task
  specifically needs those configurations.
- After `configure` completes, build with `make`.

### Running the extension

- For agent runs, prefer `php -n` so the process starts without any ambient
  `ini` state, then load the required extensions explicitly.
- Load optional serializer extensions before `redis.so` when the build enables
  them.
- A typical invocation is:

```bash
php -n \
  -dextension=json.so \
  -dextension=igbinary.so \
  -dextension=msgpack.so \
  -dextension=.libs/redis.so \
  <script> <args>
```

- Replace `<script> <args>` with the PHP file and arguments you want to run.
- If the environment does not expose those shared extensions on the default
  extension search path, use absolute paths to the `.so` files instead of bare
  extension names.
- `.libs/redis.so` is the usual build output path after `make`.

### Running tests

The PhpRedis test suite is a pure-PHP runner and expects Redis-compatible
servers to be available for the relevant classes.

```bash
php -n \
  -dextension=json.so \
  -dextension=igbinary.so \
  -dextension=msgpack.so \
  -dextension=.libs/redis.so \
  tests/TestRedis.php --class redis
```

- Use `--class redis`, `--class redisarray`, `--class rediscluster`, or
  `--class redissentinel` to choose a test group.
- Use `--test <name>` to run a single test method.
- Use `--host`, `--port`, `--tls-port`, `--user`, and `--auth` when the Redis
  server layout differs from the defaults.
- RedisArray tests expect multiple standalone Redis instances, normally on
  ports `6379`, `6380`, `6381`, and `6382`.
- RedisCluster tests require a working cluster. The helper scripts in `tests/`
  can be used when appropriate, but local `.agents.md` instructions should take
  precedence for machine-specific Redis layouts.
- Session tests can depend on helper scripts and specific `ini` settings. Unless
  you are working on session behavior, do not treat unrelated session setup
  failures as signal for the current change.

#### Variable names and older PHP builds under Valgrind

Prefer descriptive variable names in PHP tests, especially for timing values
such as `$startTime` and `$endTime`. Keep the `$endTime` names in the standalone
and cluster tests; do not shorten them back to `$et`.

Older PHP release builds used by CI can hit
[PHP GH-9068](https://github.com/php/php-src/issues/9068): Valgrind's replacement
string comparator clobbers registers that the compiled PHP caller expects to
preserve. The variable name `et` and string literal `'g2'` have the same hash,
so even `$et = 0; var_dump('g2');` crashes affected PHP 8.2.3 builds under
Valgrind without PhpRedis loaded. Adding valid userland test code can
therefore trigger a crash during compilation, before any tests execute.

Longer names avoid this known collision; they do not fix PHP or guarantee that
other collisions are impossible. For similar failures, reduce the input and
verify it against the affected CI image before attributing it to this bug.
Do not suppress errors or disable Valgrind to get a passing run. Rebuilding CI
PHP with the upstream fix is the permanent solution.

### Building in a full PHP source tree

It's possible we are in the `ext/redis` path within a full PHP source tree.
This is easy to check because there will be a parent directory like
`php-x.y.z[-debug]`.

When this is the case, the project can be built by running `make` in the root
of the build tree and then run with `sapi/cli/php`, again from the root of the
build tree.

The tests can also be run with the `sapi/cli/php` binary which will not require
specifying any specific extensions, as they are baked into the binary.

#### Cleaning intermediate build artifacts

When in a PHP source tree the build may fail due to intermediate artifacts in
`ext/redis`. This can be fixed by deleting the generated build artifacts and
running `make` again. Do not remove source files or user changes while cleaning.
