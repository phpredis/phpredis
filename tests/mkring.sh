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


for P in $PORTS; do
	PID=`lsof -i :$P | tail -1 | cut -f 2 -d " "`
	if [ "$PID" != "" ]; then
		# Stop redis
		redis-cli -h localhost -p $P shutdown
	fi
	# Start redis
	start_node $P
done

