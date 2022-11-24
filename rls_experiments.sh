#!/bin/bash
# use: ./run_experiments.sh [sample size] [subfolder]
# ./pebbling-pdr --delta --dir=./benchmark/rls/tfc/small --tfc=mod5d1 --optimize=dec --experiment=3 -s
bold=$(tput bold)
normal=$(tput sgr0)
EXEC="./pebbling-pdr"
BENCHMARKS="./benchmark/rls/tfc"

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

		command="$EXEC --silent --algo=ipdr --pebbling $folder $model $exp"

		echo "${bold}$command${normal}"
		$command
		echo -e "done\n"
	else
		echo "$m is not a valid .tfc file"
	fi
done
