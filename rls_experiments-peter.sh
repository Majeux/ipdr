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

if [ $# -eq 0 ]
then
	echo "give a sample size"
	exit 1
fi
sample=$1

echo "searching in: $BENCHMARKS/"

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

		command="$EXEC $MODE $INC --silent $model $bound $its"

		echo "${bold}$command${normal}"
		$command
		echo -e "done\n"
	fi
done
