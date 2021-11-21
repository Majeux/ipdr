#!/bin/bash
bold=$(tput bold)
normal=$(tput sgr0)
EXEC="./pebbling-pdr"

BENCHMARKS="./benchmark/rls/tfc"
if [ $# -eq 0 ]; then
	SUBF=""
else
	SUBF=$1
fi
echo "$BENCHMARKS/$SUBF/"

for file in $BENCHMARKS/$SUBF/*; do
	if [ -f $file ]; then
		fbase=$(basename $file)
		filename="${fbase%.*}"
		extension="${fbase##*.}"

		if [ "$extension" = "tfc" ]; then
			echo -e "${bold}============\n============\n============\n"
			echo -e "Running pdr for $filename\n\n"
			if [ -z $SUBF ]; then
				ARGS=""
			else
				ARGS="-b $SUBF"
			fi
			command="$EXEC -d $filename 100 $ARGS"

			echo "${bold}$command${normal}"
			$command
			echo -e "\n\n"
			command2="$EXEC -do $filename 100 $ARGS"
			echo "${bold}$command2${normal}"
			$command2
		fi
	fi
done
