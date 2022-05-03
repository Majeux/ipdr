#!/bin/bash

for dir in ./*; do if [ -d $dir ]; then ./compare_run.sh $dir; echo ""; echo ""; fi done
