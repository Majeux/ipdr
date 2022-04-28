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
elif [ $# -eq 1 ]
then
	SUBF=""
else
	SUBF=$2
fi

SAMPLE=$1

echo "$BENCHMARKS/$SUBF/"

for file in "$BENCHMARKS/$SUBF"/*; do
	if [ -f "$file" ]; then
		fbase=$(basename "$file")
		filename="${fbase%.*}"
		extension="${fbase##*.}"

		if [ "$extension" = "tfc" ]; then
			echo -e "${bold}============\n============\n"
			echo -e "Running experiment for $filename\n\n"
			if [ -z "$SUBF" ]; then
				folderarg="--dir=$BENCHMARKS"
			else
				folderarg="--dir=$BENCHMARKS/$SUBF"
			fi

			command="$EXEC --silent --delta $folderarg --tfc=$filename --optimize=dec --experiment=$SAMPLE"

			echo "${bold}$command${normal}"
			$command
			echo -e "done\n"
		fi
	fi
done
