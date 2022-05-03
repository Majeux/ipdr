#!/bin/bash

bold=$(tput bold)
normal=$(tput sgr0)

if [ $# -eq 0 ]; then 
	echo "Provide a model name"
else
	model=$1
	left=$(echo -e "${bold}delta${normal}\n$(sed '/Strategy for/Q' $1/100-delta/$1-100pebbles_delta.strategy)")
	right=$(echo -e "  ${bold}opt delta${normal}\n$(sed '/Strategy for/Q' $1/100-opt-delta/$1-100pebbles_opt_delta.strategy)")

	paste <(printf %s "$left") <(printf %s "$right") | column -s $'\t' -t
fi

