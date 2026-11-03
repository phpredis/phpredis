#!/usr/bin/env bash
set -euo pipefail

# Arguments are the command prefix: php, or valgrind [options] php.
mode=${1:?Expected a test command}
logs="${RUNNER_TEMP:?}/phpredis-tests/$mode"
mkdir -p "$logs"
status=0
read -r -a suites <<< "${PHPREDIS_TEST_SUITES:-Redis RedisArray RedisCluster RedisSentinel}"

for suite in "${suites[@]}"; do
    args=(--class "$suite" --auth phpredis --nocolors)
    if [[ $suite != RedisSentinel ]]; then
        args+=(--user phpredis)
    fi
    echo "::group::$mode: $suite"
    if "$@" tests/TestRedis.php "${args[@]}" 2>&1 | tee "$logs/$suite.log"; then
        result=PASS
    else
        result=FAIL
        status=1
        echo "::error::$mode: $suite failed"
    fi
    echo "::endgroup::"
    echo "- $mode / $suite: $result" >> "$GITHUB_STEP_SUMMARY"
done

exit "$status"
