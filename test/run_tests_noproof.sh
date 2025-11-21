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

declare -a arr_opt=(
"wcnf/driverlog01bc.wcsp.dir.wcnf*2245"
"wcnf/normalized-factor-size=9-P=383-Q=509.opb.wcnf*383"
"wcnf/WCNF_pathways_p02.wcnf*3"
"wcnf/archlinux.dimacs.wcnf*11744"
"wcnf/414.wcsp.dir.wcnf*38478"
"wcnf/g2_n56e57_n61e85.wcnf*39"
"wcnf/synthetic-20.wcnf*6720"
"wcnf/f49-DC_TotalLoss.seq-A-2-2-EDCBAir.wcnf*85435860665"
"wcnf/edgecase2.wcnf*0"
"wcnf/edgecases.wcnf*2"
"lp/hole.lp*2917/2"
"mps/disctom.mps*-5000"
"mps/hypothyroid-k1.mps*-2851"
"mps/mod008.mps*307"
"mps/neos8.mps*-3719"
"mps/neos-3004026-krka.mps*0"
"mps/neos-3437289-erdre.mps*0"
"mps/supportcase4.mps*0"
"mps/issue_3.mps*3"
"opb/opt/veripb2_test.opb*-1"
"opb/opt/empty.opb*0"
"opb/opt/normalized-single-obj-f47-DC-Side1.seq-B-2-1-EDCBAir.opb*-1593213266"
"opb/opt/enigma.opb*0"
"opb/opt/stein9.opb*5"
"opb/opt/stein15.opb*9"
"opb/opt/stein27.opb*18"
"opb/opt/stein45.opb*30"
"opb/opt/p0033.opb*3089"
"opb/opt/p0040.opb*62027"
"opb/opt/p0201.opb*7615"
"opb/opt/p0282.opb*258411"
"opb/opt/p0291.opb*7609041"
"opb/opt/p0548.opb*8691"
"opb/opt/mod008.opb*307"
"opb/opt/mod010.opb*6548"
"opb/opt/air01.opb*6796"
"opb/opt/air02.opb*7810"
"opb/opt/air03.opb*340160"
"opb/opt/air06.opb*49649"
"opb/opt/pipex.opb*788263"
"opb/opt/sentoy.opb*-7772"
"opb/opt/bm23.opb*34"
"opb/opt/l152lav.opb*4722"
"opb/opt/lp4l.opb*2967"
"opb/opt/lseu.opb*1120"
"opb/opt/cracpb1.opb*22199"
"opb/opt/rs_iss_3.opb*2"
"opb/opt/knapPI_16_1000_1000_1_-27147.opb*-27147"
"opb/opt/testlin.opb*0"
"opb/opt/mult.opb*5"
"wbo/example1.wbo*2"
"wbo/example2.wbo*2"
)

echo "########## no proofs ##########"
echo ""
for j in "${arr_opt[@]}"; do
    formula="$(cut -d'*' -f1 <<<$j)"
    logfile="$logfolder/noproofs/$formula"
    mkdir -p `dirname $logfile`
    echo -n "" > $logfile.proof
    echo -n "" > $logfile.formula
    formula="$SCRIPTPATH/instances/$formula"
    if [ ! -f "$formula" ]; then
        echo "$formula does not exist."
        exit 1
    fi
    obj="$(cut -d'*' -f2 <<<$j)"
    echo "running $binary $formula $options --seed=3"
    output=`$binary $formula $options --seed=3 2>&1`
    error=`echo "$output" | awk '/Error:|UNSATISFIABLE|.*Assertion.*/ {print $2}'`
    if [ "$error" != "" ] ; then
        errors=`expr 1000 + $errors`
        echo "parsed error: $error"
    fi
    objective=`echo "$output" | awk '/^o/ {print $2}'`
    result=`echo "$output" | awk '/^s/ {print $2}'`
    if [ "$objective" != "" ]; then
        if (( "$objective" > "$obj" )) && [ "$result" == "OPTIMUM" ] || (( "$objective" < "$obj" )) ; then
            errors=`expr 100 + $errors`
            echo "wrong objective: $objective vs $obj"
        fi
    fi
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
