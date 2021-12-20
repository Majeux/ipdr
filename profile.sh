#!/bin/bash

perf record -o output/profiling/perf.data -g --call-graph dwarf "$@"
