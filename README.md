# pebbling-pdr
todo complete
## Requirements
`vcpkg install cxxopts fmt ghc_filesystem spdlog z3`

## Compiling 
`cmake -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake [OPTIONS]`

Default `OPTIONS` turn of logging, stat collection and debug assertions.

## Running
Help:
`./pebbling-pdr -h`

General format:
`./pebbling-pdr problem algorithm mode [OPTIONS]`

`problem = pebbling | peterson`
`algorithm = pdr | ipdr`
`mode = run | experiment`

`OPTIONS` to configure the input transition system, algorithm ...
