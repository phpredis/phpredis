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
    code=0
    "$@" tests/TestRedis.php "${args[@]}" 2>&1 | tee "$logs/$suite.log" || code=${PIPESTATUS[0]}
    echo "::endgroup::"
    if (( code == 0 )); then
        echo "- $mode / $suite: PASS" >> "$GITHUB_STEP_SUMMARY"
        continue
    fi
    status=1
    # Surface the first fatal line so flaky failures are diagnosable from the annotation.
    reason=$(grep -m 1 -E '^(Uncaught |Assertion failed:|(PHP )?Fatal error:|==[0-9]+== (Invalid |Use of uninitialised|Conditional jump|ERROR SUMMARY: [1-9]))' "$logs/$suite.log" || true)
    detail="$mode: $suite failed (exit $code). ${reason:-See $suite log above}"
    echo "::error::${detail//'%'/'%25'}"
    echo "- $detail" >> "$GITHUB_STEP_SUMMARY"
done

exit "$status"
