# IPDR
## The Algorithm
Implementation of the [IPDR](https://arxiv.org/abs/2308.12162) algorithm in C++. IPDR is incremental extension of the original [PDR (or IC3)](https://link.springer.com/chapter/10.1007/978-3-642-18275-4_7) algorithm by Aaron R. Bradley.

Each iteration of IPDR, PDR is run on an instance of a given problem and information from the previous iteration is reused to reduce runtime of the current.

## Dependencies
- [CMake](https://cmake.org/) v3.8 or higher
- C++17
- [Vcpkg](https://github.com/microsoft/vcpkg)
	- Installed as per the GitHub page in some folder `[VCPKGROOT] = ~/.../vcpkg`
   	- Inside [VCPKGROOT], run `./vcpkg install cxxopts fmt spdlog z3`
- Graphviz
	- Can be installed via `sudo apt install graphviz-dev`

## Compilation
After cloning this repository, initialize its submodules:
```
git submodule init
git submodule update
```
Create a build directory and enter it:
```
mkdir build
cd build
```
Vcpkg integrates with CMake via its toolchain file. To compile, run the following in the `build` directory:
```
cmake -DCMAKE_TOOLCHAIN_FILE=[VCPKGROOT]/scripts/buildsystems/vcpkg.cmake [OPTIONS] ..
make
```

`OPTIONS` consist of the following, which are turned off by default:
- `-DDO_LOG=on` enables logging. A `.log` file in the output's `analysis` folder.
- `-DDO_STATS=on` turns on the collection of statistics gathered during a run. A `.stats` file in the output's `analysis` folder.
- `-DDEBUG=on` turns off `-O3` optimization and and enables debug assertions.


After these steps you will find the `ipdr-engine` executable in the project root.

## Running

Use a command of the following form from the root of the program:
```
./ipdr-engine PROBLEM ALGORITHM MODE [OPTIONS]
```

Where:
- `PROBLEM = pebbling | peterson`
Select which example problem to run ipdr for. Finding a strategy for the Reversible Pebbling Game or the correctness of Peterson's Algorithm.

- `ALGORITHM = pdr | ipdr`
Select whether to perform our implementation of PDR on one instance of the problem or IPDR to evaluate multiple instances.

- `MODE = run | experiment`
Select whether to perform a single run ,or perform multiple and aggregate the results. In an experiment a control run is also performed (which simply executes PDR multiple times) to compare against.

- Use the `OPTIONS` to further configure the input transition system and algorithm. See `./ipdr-engine -h`.

## Implemented Sample Problems
IPDR has been implemented to solve two different problems.

###  [Reversible Pebbling Game for Quantum Memory Management](https://arxiv.org/abs/1904.02121)
An optimization problem which takes as input a quantum circuit and outputs a memory management strategy using a minimal amount of auxiliary wires (analogous to completing the Reversible Pebbling Game with a minimal  number of "pebbles").

Graphs are accepted in `.tfc` files. [Sample files](https://reversiblebenchmarks.github.io/) are listed in `./benchmark/rls-benchmarks.txt`.

### Peterson's Algorithm
An instance of Peterson's Algorithm is verified while allowing only a limited number of context switches (interleavings).

The input instance is defined by the `--procs` and `--max_switches` parameters, giving upon execution.

## Output
Result files are written into the `output` folder. Runs are sorted into folders and subfolders based on first the selected mode, then algorithm, then problem and then input.

Within 

## Examples
Using Constraining IPDR to find a pebbling strategy for the input `4b15g_1.tfc` listed in the `benchmark/rls-benchmarks.txt` :
```
./ipdr-engine pebbling ipdr run --inc=constrain --dir=./benchmark/rls/tfc --tfc=4b15g_1
```
The results are written to `./output/runs/ipdr/pebbling/4b15g_1/4b15g_1-pebbling-ipdr_constrain`.

Using Relaxing IPDR to verify Peterson's Algorithm for 2 processes, bounded to 2 context switches (interleavings):
```
./ipdr-engine peterson ipdr run --inc=relax --procs=2 --max_switches=2
```
The results are written to `./output/runs/ipdr/peter/2procs/2procs-peter_2switches-ipdr_relax`.
