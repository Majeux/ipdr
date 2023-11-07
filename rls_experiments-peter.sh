#!/bin/bash
# use: ./run_experiments.sh [sample size] [subfolder]
# ./pebbling-pdr --dir=./benchmark/rls/tfc/ --tfc=ham7tc --algo=ipdr --inc=binary_search --pebbling
bold=$(tput bold)
normal=$(tput sgr0)
EXEC="./pebbling-pdr"
BENCHMARKS="./benchmark/rls/tfc"
MODE="peterson ipdr experiment"
INC="--inc=relax"
Z3=""
SEEDS=("1004116894,674395564,392236153,999267838,1385010657,1874259453,185577622,2046848429,1293318392,784654507" "1284338210,1454693642,1053420622,1593077573,1013342442,985605802,478986771,875591177,1149455113,444317512" "693825191,504369732,958993891")

if [ $# -eq 0 ]
then
	echo "give a sample size"
	exit 1
fi
sample=$1

models=()
while read -r line
do
	if [ ${line:0:1} != "#" ]
	then
		words=($line)
		P=${words[0]}
		switches=${words[1]}

		echo -e "${bold}============\n============\n"
		echo -e "Running experiment for $P procs and $switches switches\n"
		bound="--max_switches=$switches"
		model="--procs=$P"
		its="--iterations=$sample"
		# exp=""
		seed=${SEEDS[$i]}

		command="$EXEC $MODE $INC --silent $model $bound $its --z3pdr --seeds=$seed"

		echo "${bold}$command${normal}"
		$command
		echo -e "done\n"
	fi
done
