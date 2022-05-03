#!/bin/bash

operators=("2,3" "3,4" "4,5" "5,7", "6,7" "8,7" "10,7" "12,7" "16,23")

for op in "${operators[@]}"; do
	command=(./pebbling-pdr --showonly "--hop=$op" 100)
	echo "${command[@]}"
	"${command[@]}"
done
