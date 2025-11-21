#!/bin/bash

logfolder="~/Tmp/Exact/$2"
binary=$3
options="--timeout=$1 $4"


SCRIPTPATH="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

errors=0
tested=0
completed=0

echo "###########################"
echo "########## START ##########"
echo "###########################"
echo ""
echo "data: $logfolder"
echo "binary: $binary"
echo "options: $options"
echo ""

declare -a arr_dec=(
"cnf/ec-rand4regsplit-v030-n1.cnf*UNSATISFIABLE"
"cnf/edgecase1.cnf*UNSATISFIABLE"
"cnf/edgecase2.cnf*OPTIMUM"
"cnf/simple.cnf*OPTIMUM"
"wcnf/edgecase1.wcnf*UNSATISFIABLE"
"mps/stein9inf.mps*UNSATISFIABLE"
"opb/dec/256ebits_any.opb*UNSATISFIABLE"
"opb/dec/256ebits_0.opb*UNSATISFIABLE"
"opb/dec/128ebits_any.opb*UNSATISFIABLE"
"opb/dec/128ebits_0.opb*UNSATISFIABLE"
"opb/dec/22array_alg_ineq7.opb*UNSATISFIABLE"
"opb/dec/21array_alg_ineq7.opb*UNSATISFIABLE"
"opb/dec/32array_alg_ineq5.opb*UNSATISFIABLE"
"opb/dec/air01.0.s.opb*OPTIMUM"
"opb/dec/air01.0.u.opb*UNSATISFIABLE"
"opb/dec/air02.0.s.opb*OPTIMUM"
"opb/dec/air02.0.u.opb*UNSATISFIABLE"
"opb/dec/air06.0.s.opb*OPTIMUM"
"opb/dec/air06.0.u.opb*UNSATISFIABLE"
"opb/dec/bm23.0.s.opb*OPTIMUM"
"opb/dec/bm23.0.u.opb*UNSATISFIABLE"
"opb/dec/cracpb1.0.s.opb*OPTIMUM"
"opb/dec/cracpb1.0.u.opb*UNSATISFIABLE"
"opb/dec/diamond.0.d.opb*UNSATISFIABLE"
"opb/dec/lp4l.0.s.opb*OPTIMUM"
"opb/dec/lp4l.0.u.opb*UNSATISFIABLE"
"opb/dec/p0040.0.s.opb*OPTIMUM"
"opb/dec/p0040.0.u.opb*UNSATISFIABLE"
"opb/dec/p0291.0.s.opb*OPTIMUM"
"opb/dec/p0291.0.u.opb*UNSATISFIABLE"
"opb/dec/pipex.0.s.opb*OPTIMUM"
"opb/dec/pipex.0.u.opb*UNSATISFIABLE"
"opb/dec/sentoy.0.s.opb*OPTIMUM"
"opb/dec/sentoy.0.u.opb*UNSATISFIABLE"
"opb/dec/stein9.0.s.opb*OPTIMUM"
"opb/dec/stein9.0.u.opb*UNSATISFIABLE"
"opb/dec/stein15.0.s.opb*OPTIMUM"
"opb/dec/stein15.0.u.opb*UNSATISFIABLE"
"opb/opt/testnlc.opb*UNSATISFIABLE"
"wbo/example3.wbo*UNSATISFIABLE"
)

runtype="default"

echo "########## $runtype #########"
echo ""

for j in "${arr_dec[@]}"; do
    formula="$(cut -d'*' -f1 <<<$j)"
    logfile="$logfolder/$runtype/$formula"
    mkdir -p `dirname $logfile`
    echo -n "" > $logfile.proof
    echo -n "" > $logfile.formula
    formula="$SCRIPTPATH/instances/$formula"
    if [ ! -f "$formula" ]; then
        echo "$formula does not exist."
        exit 1
    fi
    obj="$(cut -d'*' -f2 <<<$j)"
    echo "running $binary $formula $options --proof-log=$logfile"
    output=`$binary $formula $options --proof-log=$logfile 2>&1`
    error=`echo "$output" | awk '/Error:|.*Assertion.*/ {print $2}'`
    if [ "$error" != "" ] ; then
        errors=`expr 1000 + $errors`
        echo "parsed error: $error"
    fi
    result=`echo "$output" | awk '/UNSATISFIABLE|OPTIMUM/ {print $2}'`
    if [ "$result" != "" ] && [ "$result" != "$obj" ] ; then
        errors=`expr 100 + $errors`
        echo "wrong output: $result vs $obj"
    fi
    echo "verifying veripb $logfile.formula $logfile.proof --arbitraryPrecision --no-requireUnsat"
    wc -l $logfile.proof
    veripb $logfile.formula $logfile.proof --arbitraryPrecision --no-requireUnsat
    errors=`expr $? + $errors`
    echo $errors
    tested=`expr 1 + $tested`
    echo $tested
    echo ""
done

echo "tested: $tested"
echo "errors: $errors"

if [ $errors -eq 0 ]; then
		exit 0
else
    exit 1
fi
