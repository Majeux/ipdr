#!/bin/bash

exp_folder="./output/experiments"
while read -r model
do
	if [ -d "$exp_folder/$model" ]
	then
		for run_folder in "$exp_folder/$model"/*
		do
			if [ -d "$run_folder" ]
			then
				for tex in "$run_folder/"*.tex
				do
					if [ -f "$tex" ]
					then
						filename=$(basename "$tex")
						name=${filename%.*}
						echo "\\begin{table}"
						echo "\\centering"
						cat "$tex"
						name=${name//^/\\\^\{\}}
						name=${name//_/\\_}
						echo "\\caption{\\texttt{$name}}"
						echo "\\end{table}"
						echo ""
					fi
				done
			fi
		done
	fi
done
