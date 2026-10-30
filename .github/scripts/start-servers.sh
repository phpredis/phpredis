#!/usr/bin/env bash
set -euo pipefail

backend=${1:?Expected redis, valkey or dragonfly}
server="$backend-server"
cli="$backend-cli"
if [[ $backend == dragonfly ]]; then
    server=redis-server
    cli=redis-cli
fi
state="${RUNNER_TEMP:?}/phpredis-servers"
acl="$PWD/tests/users.acl"
mkdir -p "$state" tests/nodes

start_server() {
    local name=$1
    shift
    mkdir -p "$state/$name"
    if [[ $backend == dragonfly ]]; then
        # The SETBIT boundary test allocates 256 MiB for one key.
        dragonfly --bind=127.0.0.1 --dir="$state/$name" --aclfile="$acl" \
            --proactor_threads=1 --maxmemory=1gb --maxclients=1000 \
            --version_check=false "$@" >"$state/$name/server.log" 2>&1 &
        return
    fi
    "$server" --bind 127.0.0.1 --daemonize yes --save '' \
        --dir "$state/$name" --pidfile "$state/$name/server.pid" \
        --logfile "$state/$name/server.log" \
        --aclfile "$acl" --acl-pubsub-default allchannels "$@"
}

wait_for() {
    local start=$SECONDS
    until "$cli" "$@" 2>&1 | grep -qE '^(PONG|cluster_state:ok)'; do
        if (( SECONDS - start >= 30 )); then
            echo "Timed out waiting for $cli $*" >&2
            return 1
        fi
        sleep .5
    done
}

socket_port=0
if [[ $backend == dragonfly ]]; then
    # shortcut: port=0 ignores ACLs; remove the TCP listener when Dragonfly fixes this.
    socket_port=6383
fi
for port in {6379..6382}; do
    start_server "$port" --port "$port"
done
start_server socket --port "$socket_port" --unixsocket /tmp/redis.sock

# testHighPorts probes with an unauthenticated PING before testing AUTH.
sed '/^user default /s/>phpredis/nopass/' "$acl" > "$state/highports.acl"
for port in {32767..32770}; do
    start_server "$port" --port "$port" --aclfile "$state/highports.acl"
done

openssl req -x509 -newkey rsa:2048 -nodes -days 1 -subj /CN=localhost \
    -addext 'subjectAltName=DNS:localhost,IP:127.0.0.1' \
    -keyout "$state/server.key" -out "$state/server.crt"
if [[ $backend == dragonfly ]]; then
    start_server tls --port 6378 --tls \
        --tls_cert_file "$state/server.crt" --tls_key_file "$state/server.key"
else
    start_server tls --port 0 --tls-port 6378 --tls-auth-clients no \
        --tls-cert-file "$state/server.crt" --tls-key-file "$state/server.key" \
        --tls-ca-cert-file "$state/server.crt"
fi

cluster_args=()
REDISCLI_AUTH=phpredis wait_for --user phpredis PING
if REDISCLI_AUTH=phpredis "$cli" --user phpredis CONFIG GET cluster-bus-port-protected-mode |
    grep -q '^cluster-bus-port-protected-mode$'; then
    # The test cluster's plaintext bus is bound to loopback only.
    cluster_args+=(--cluster-bus-port-protected-mode no)
fi

: > tests/nodes/nodemap
cluster_ports=({7000..7005})
if [[ $backend == dragonfly ]]; then
    cluster_ports=(7000)
    start_server 7000 --port 7000 --cluster_mode=emulated
fi
for port in "${cluster_ports[@]}"; do
    if [[ $backend != dragonfly ]]; then
        start_server "$port" --port "$port" --cluster-enabled yes \
            --cluster-config-file nodes.conf "${cluster_args[@]}"
    fi
    echo "127.0.0.1:$port" >> tests/nodes/nodemap
done

for port in {26379..26380}; do
    mkdir -p "$state/$port"
    cat > "$state/$port/sentinel.conf" <<EOF
port $port
bind 127.0.0.1
daemonize yes
dir $state/$port
pidfile $state/$port/server.pid
logfile $state/$port/server.log
sentinel monitor mymaster 127.0.0.1 6379 1
sentinel auth-pass mymaster phpredis
EOF
    "$server" "$state/$port/sentinel.conf" --sentinel
    wait_for -p "$port" PING
done

export REDISCLI_AUTH=phpredis
for port in {6379..6382} "${cluster_ports[@]}" {32767..32770}; do
    wait_for --user phpredis -p "$port" PING
done
wait_for --user phpredis -s /tmp/redis.sock PING
wait_for --user phpredis --tls --cacert "$state/server.crt" -p 6378 PING

if [[ $backend != dragonfly ]]; then
    "$cli" --user phpredis --cluster create 127.0.0.1:{7000..7005} \
        --cluster-replicas 1 --cluster-yes
fi
for port in "${cluster_ports[@]}"; do
    wait_for --user phpredis -p "$port" CLUSTER INFO
done
