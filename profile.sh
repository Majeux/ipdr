#!/bin/bash

perf record -o output/_profiling/perf.data -g -ggdb --call-graph dwarf "$@"
