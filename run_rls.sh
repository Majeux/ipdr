#!/bin/bash
bold=$(tput bold)
normal=$(tput sgr0)
EXEC="./pebbling-pdr"
BENCHMARKS="./benchmark/rls"

for file in $BENCHMARKS/*; do
	if [ -f $file ]; then
		fbase=$(basename $file)
		filename="${fbase%.*}"
		extension="${fbase##*.}"

		if [ "$extension" = "tfc" ]; then
			echo -e "${bold}============\n============\n============\n"
			echo -e "Running pdr for $filename\n\n"
			command="$EXEC -d $filename 100"
			echo "${bold}$command${normal}"
			$command
			echo -e "\n\n"
			command2="$EXEC -do $filename 100"
			echo "${bold}$command2${normal}"
			$command2
		fi
	fi
done
