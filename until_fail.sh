#!/bin/bash
count=0
while "$@" 
do
    count=$((count + 1))
    echo -e "------------ Finished trial $count -----------\n"
done