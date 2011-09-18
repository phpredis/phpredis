#!/bin/bash

PORTS="6379 6380 6381 6382"
REDIS=redis-server

function start_node() {
	P=$1
	echo "starting node on port $P";
	CONFIG_FILE=`tempfile`
	cat > $CONFIG_FILE << CONFIG
port $P
CONFIG
	$REDIS $CONFIG_FILE > /dev/null 2>/dev/null &
	sleep 1
	rm -f $CONFIG_FILE
}

function stop_node() {

	P=$1
	PID=$2
	redis-cli -h localhost -p $P shutdown
	kill -9 $PID 2>/dev/null
}

function stop() {
	for P in $PORTS; do
		PID=`lsof -i :$P | tail -1 | cut -f 2 -d " "`
		if [ "$PID" != "" ]; then
			stop_node $P $PID
		fi
	done
}

function start() {
	for P in $PORTS; do
		start_node $P
	done
}

case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		stop
		start
		;;
	*)
		echo "Usage: $0 [start|stop|restart]"
		;;
esac
