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
NOASK=0
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
    # ACL file if we have one
    if [ ! -z "$ACLFILE" ]; then
        ACLARG="--aclfile $ACLFILE"
    fi

    # Attempt to spawn the node
    verboseRun redis-server --cluster-enabled yes --dir $NODEDIR --port $PORT \
        --cluster-config-file node-$PORT.conf --daemonize yes --save \'\' \
        --bind $HOST --dbfilename node-$PORT.rdb $ACLARG

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

    if [ -f "$ACLFILE" ]; then
        cp $ACLFILE $NODEDIR/$ACLFILE
    fi
}

# Initialize our cluster with redis-trib.rb
initCluster() {
    TRIBARGS=""
    for PORT in `seq $START_PORT $END_PORT`; do
        TRIBARGS="$TRIBARGS $HOST:$PORT"
    done

    if [[ ! -z "$USER" ]]; then
        USERARG="--user $USER"
    fi
    if [[ ! -z "$PASS" ]]; then
        PASSARG="-a $PASS"
    fi

    if [[ "$1" -eq "1" ]]; then
        echo yes | redis-cli $USERARG $PASSARG -p $START_PORT --cluster create $TRIBARGS --cluster-replicas $REPLICAS
    else
        verboseRun redis-cli $USERARG $PASSARG -p $START_PORT --cluster create $TRIBARGS --cluster-replicas $REPLICAS
    fi

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
    initCluster $1
}

# Shut down nodes in our cluster
stopCluster() {
    for PORT in `seq $START_PORT $END_PORT`; do
        verboseRun redis-cli -p $PORT SHUTDOWN NOSAVE > /dev/null 2>&1
    done
}

# Shut down nodes by killing them
killCluster() {
    for PORT in `seq $START_PORT $END_PORT`; do
        PID=$(ps aux|grep [r]edis-server|grep $PORT|awk '{print $2}')
        echo -n "Killing $PID: "
        if kill $PID; then
            echo "OK"
        else
            echo "ERROR"
        fi
    done
}

printUsage() {
    echo "Usage: make-cluster [OPTIONS] <start|stop|kill>"
    echo
    echo "  Options"
    echo
    echo "  -u Redis username to use when spawning cluster"
    echo "  -p Redis password to use when spawning cluster"
    echo "  -a Redis acl filename to use when spawning cluster"
    echo "  -y Automatically send 'yes' when starting cluster"
    echo "  -h This message"
    echo
    exit 0
}

# We need redis-server
checkExe redis-server

while getopts "u:p:a:hy" OPT; do
    case $OPT in
        h)
            printUsage
            ;;
        a)
            if [ ! -f "$OPTARG" ]; then
                echo "Error:  '$OPTARG' is not a filename!"
                exit -1
            fi
            ACLFILE=$OPTARG
            ;;
        u)
            USER=$OPTARG
            ;;
        p)
            PASS=$OPTARG
            ;;
        h)
            HOST=$OPTARG
            ;;
        y)
            NOASK=1
            ;;
        *)
            echo "Unknown option: $OPT"
            exit 1
            ;;
    esac
done

shift "$((OPTIND - 1))"

if [[ $# -lt 1 ]]; then
    echo "Error:  Must pass an operation (start or stop)"
    exit -1
fi

case "$1" in
    start)
        startCluster $NOASK
        ;;
    stop)
        stopCluster
        ;;
    kill)
        killCluster
        ;;
    *)
        echo "Usage: make-cluster.sh [options] <start|stop>"
        exit 1
        ;;
esac
