#!/bin/bash

perf record -o output/_profiling/perf.data -g --call-graph dwarf "$@"
