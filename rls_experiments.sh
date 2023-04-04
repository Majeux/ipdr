#!/bin/bash
# use: ./run_experiments.sh [sample size] [subfolder]
# ./pebbling-pdr --dir=./benchmark/rls/tfc/ --tfc=ham7tc --algo=ipdr --inc=binary_search --pebbling
bold=$(tput bold)
normal=$(tput sgr0)
EXEC="./pebbling-pdr"
BENCHMARKS="./benchmark/rls/tfc"
MODE="pebbling ipdr experiment"
INC="--inc=constrain"
Z3=""

if [ $# -eq 0 ]
then
	echo "give a sample size"
	exit 1
fi
sample=$1

echo "searching in: $BENCHMARKS/"

models=()
while read -r m
do
	if [ -f "$BENCHMARKS/$m.tfc" ]
	then
		echo -e "${bold}============\n============\n"
		echo -e "Running experiment for $filename\n"
		folder="--dir=$BENCHMARKS"
		model="--tfc=$m"
		its="--iterations=$sample"
		# exp=""

		command="$EXEC $MODE $INC --silent --ctg-depth=1 --max-ctgs=3 $Z3 $folder $model $its"

		echo "${bold}$command${normal}"
		$command
		echo -e "done\n"
	else
		echo "$m is not a valid .tfc file"
	fi
done
