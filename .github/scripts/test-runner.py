"""Check suite selection and failure propagation through tee."""

from itertools import product
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


runner = Path(__file__).with_name("run-tests.sh").resolve()
suites = ["Redis", "RedisArray", "RedisCluster", "RedisSentinel"]
with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    php = root / "php"
    php.write_text('''#!/bin/bash
echo "$*"
if [[ $3 == "$FAIL_SUITE" ]]; then
    [[ -z $FAIL_DETAIL ]] || echo "$FAIL_DETAIL" >&2
    exit 23
fi
''')
    php.chmod(0o755)
    valgrind = root / "valgrind"
    valgrind.write_text('#!/bin/bash\n[[ $1 == --error-exitcode=1 ]] || exit 2\nshift\nexec "$@"\n')
    valgrind.chmod(0o755)
    commands = (["php"], ["valgrind", "--error-exitcode=1", "php"])
    for command, selection in product(commands, ("", "Redis RedisCluster")):
        selected = selection.split() if selection else suites
        reasons = ("", "Uncaught exception 'WRONGPASS' (testPing) at RedisTest.php:187",
                   "==42== Invalid read of size 8")
        for failure, reason in product(["", *suites], reasons):
            shutil.rmtree(root / "phpredis-tests", ignore_errors=True)
            summary = root / "summary"
            summary.write_text("")
            result = subprocess.run(
                ["bash", str(runner), *command],
                env={**os.environ, "PATH": f"{root}:{os.environ['PATH']}",
                     "RUNNER_TEMP": directory, "GITHUB_STEP_SUMMARY": str(summary),
                     "FAIL_SUITE": failure, "FAIL_DETAIL": reason,
                     "PHPREDIS_TEST_SUITES": selection},
                capture_output=True, text=True,
            )
            assert result.returncode == (failure in selected), result.stdout + result.stderr
            errors = [line for line in result.stdout.splitlines() if line.startswith("::error::")]
            detail = f"{command[0]}: {failure} failed (exit 23). {reason or f'See {failure} log above'}"
            if failure in selected:
                assert errors == ["::error::" + detail], result.stdout
                assert f"::endgroup::\n::error::{detail}\n" in result.stdout, result.stdout
            else:
                assert not errors, result.stdout
            assert summary.read_text().splitlines() == [
                f"- {detail}" if suite == failure else f"- {command[0]} / {suite}: PASS"
                for suite in selected
            ]
            logs = root / "phpredis-tests" / command[0]
            assert sorted(p.stem for p in logs.glob("*.log")) == sorted(selected)
            for suite in selected:
                expected = f"tests/TestRedis.php --class {suite} --auth phpredis --nocolors"
                if suite != "RedisSentinel":
                    expected += " --user phpredis"
                if suite == failure and reason:
                    expected += "\n" + reason
                log = root / "phpredis-tests" / command[0] / f"{suite}.log"
                assert log.read_text().strip() == expected

print("Test runner: default and selected suites checked with PHP and Valgrind prefixes")
