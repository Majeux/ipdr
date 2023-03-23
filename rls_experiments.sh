#!/bin/bash
# use: ./run_experiments.sh [sample size] [subfolder]
# ./pebbling-pdr --dir=./benchmark/rls/tfc/ --tfc=ham7tc --algo=ipdr --inc=binary_search --pebbling
bold=$(tput bold)
normal=$(tput sgr0)
EXEC="./pebbling-pdr"
BENCHMARKS="./benchmark/rls/tfc"
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
		exp="--experiment=$sample"
		# exp=""

		command="$EXEC --silent --algo=ipdr --inc=binary_search --pebbling $Z3 $folder $model $exp"

		echo "${bold}$command${normal}"
		$command
		echo -e "done\n"
	else
		echo "$m is not a valid .tfc file"
	fi
done
