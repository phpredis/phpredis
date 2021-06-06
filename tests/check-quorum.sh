#!/bin/bash

for try in $(seq 0 3); do
    RES=$(redis-cli -p 26379 sentinel ckquorum mymaster 2>/dev/null);
    if [[ "$RES" == OK* ]]; then
        echo "Quorum detected, exiting";
        break;
    else 
        echo "No Quorum - $RES";
    fi;
    sleep 1;
done
