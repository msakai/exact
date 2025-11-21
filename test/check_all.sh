#!/bin/bash

# usage: ../test/run_all.sh 3 "testruns" /home/jod/workspace/exact-dev/build_testruns/Exact

logfolder="~/Tmp/Exact/$1"
SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

declare -a arr_configs=(
"default"
"arbitrary"
"noproof"
"mindiv"
"rto"
"slack+1"
"noassumps"
)

for idx in "${!arr_configs[@]}"; do
    config=${arr_configs[$idx]}
    mkdir -p `dirname $logfolder/$config.txt`
    echo "*** $config ***"
    tail -n 2 $logfolder/$config.txt
done
