#!/bin/bash

EXEC="../build/pebbling-pdr"
BENCHMARKS="./rls"

for file in $BENCHMARKS/*; do
	if [ -f $file ]; then
		fbase=$(basename $file)
		filename="${fbase%.*}"
		extension="${fbase##*.}"

		if [ "$extension" = "tfc" ]; then
			command="$EXEC -d $filename 100"
			echo $command
			$command
			echo $command
			command="$EXEC -do $filename 100"
			$command
		fi
	fi
done
