# AIS `seqmaker` & `seqdiff`

Tools to parse dumps of AIS data in comma-separated values (`csv`) from standard input. Both tools, `seqmaker` and `seqdiff`, print a more verbose help screen when invoked with no arguments. A short summary is given below:
- `seqmaker`: Gathers lines of AIS data from standard input as sequences by MMSI. Sequences of a common MMSI are split by length and if consecutive points deviate significantly. The resulting sequences are split until they have the target length. Remaining parts are discarded.
- `seqdiff`: Determines adjacent differences of time and position of AIS data with a common MMSI, where the data stream is read from standard input.

## Compilation
We use [`cmake`](https://cmake.org/) as our build tool. Compile the project for example via:
```
$ cd ais_seqmaker/ && mkdir -p build
$ cd build/
$ cmake ..
$ make
```
We offer different build flags. Run a tool such as [`ccmake`](https://cmake.org/cmake/help/latest/manual/ccmake.1.html) to configure them.
Executables are placed in the `src` directory, e.g.,
```
$ build/src/seqmaker
```
