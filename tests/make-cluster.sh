#!/bin/bash

# make-cluster.sh
# This is a simple script used to automatically spin up a Redis cluster instance
# simplifying the process of running unit tests.
#
# Usage:
#   ./make-cluster.sh start [host]
#   ./make-cluster.sh stop [host]
#

BASEDIR=`pwd`
NODEDIR=$BASEDIR/nodes
MAPFILE=$NODEDIR/nodemap

# Host, nodes, replicas, ports, etc.  Change if you want different values
HOST="127.0.0.1"
NODES=12
REPLICAS=3
START_PORT=7000
END_PORT=`expr $START_PORT + $NODES`

# Helper to determine if we have an executable
checkExe() {
    if ! hash $1 > /dev/null 2>&1; then
        echo "Error:  Must have $1 on the path!"
        exit 1
    fi
}

# Run a command and output what we're running
verboseRun() {
    echo "Running: $@"
    $@
}

# Spawn a specific redis instance, cluster enabled 
spawnNode() {
    # Attempt to spawn the node
    verboseRun redis-server --cluster-enabled yes --dir $NODEDIR --port $PORT \
        --cluster-config-file node-$PORT.conf --daemonize yes --save \'\' \
        --bind $HOST --dbfilename node-$PORT.rdb

    # Abort if we can't spin this instance
    if [ $? -ne 0 ]; then 
        echo "Error:  Can't spawn node at port $PORT."
        exit 1
    fi
}

# Spawn nodes from start to end port
spawnNodes() {
    for PORT in `seq $START_PORT $END_PORT`; do
        # Attempt to spawn the node
        spawnNode $PORT

        # Add this host:port to our nodemap so the tests can get seeds
        echo "$HOST:$PORT" >> $MAPFILE
    done
}

# Check to see if any nodes are running
checkNodes() {
    echo -n "Checking port availability "
    
    for PORT in `seq $START_PORT $END_PORT`; do
        redis-cli -p $PORT ping > /dev/null 2>&1
        if [ $? -eq 0 ]; then
            echo "FAIL"
            echo "Error:  There appears to be an instance running at port $PORT"
            exit 1
        fi
    done
    
    echo "OK"
}

# Create our 'node' directory if it doesn't exist and clean out any previous
# configuration files from a previous run.
cleanConfigInfo() {
    verboseRun mkdir -p $NODEDIR
    verboseRun rm -f $NODEDIR/*
}

# Initialize our cluster with redis-trib.rb
initCluster() {
    TRIBARGS=""
    for PORT in `seq $START_PORT $END_PORT`; do
        TRIBARGS="$TRIBARGS $HOST:$PORT"
    done

    verboseRun redis-trib.rb create --replicas $REPLICAS $TRIBARGS

    if [ $? -ne 0 ]; then
        echo "Error:  Couldn't create cluster!"
        exit 1
    fi
}

# Attempt to spin up our cluster
startCluster() {
    # Make sure none of these nodes are already running
    checkNodes

    # Clean out node configuration, etc
    cleanConfigInfo

    # Attempt to spawn the nodes
    spawnNodes

    # Attempt to initialize the cluster
    initCluster
}

# Shut down nodes in our cluster
stopCluster() {
    for PORT in `seq $START_PORT $END_PORT`; do
        verboseRun redis-cli -p $PORT SHUTDOWN NOSAVE > /dev/null 2>&1
    done
}

# Make sure we have redis-server and redis-trib.rb on the path
checkExe redis-server
checkExe redis-trib.rb

# Override the host if we've got $2
if [[ ! -z "$2" ]]; then
   HOST=$2
fi

# Main entry point to start or stop/kill a cluster
case "$1" in
    start)
        startCluster
        ;;
    stop)
        stopCluster
        ;;
    *)
        echo "Usage $0 <start|stop> [host]"
        ;;
esac
