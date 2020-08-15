# Rust sample
This sample shows how to build an enclave application written in Rust.

The directory `app` contains the Rust application code. It is a Cargo project that builds a static library. The library exports the function `rustmain` that can be called from C.

The enclave entry point is in `main.c` and just calls `rustmain`.

The sample can be built and run as follows:
```sh
mkdir build
cd build
cmake ..
make
erthost enclave.signed
```
When using simulation mode, use
```sh
OE_SIMULATION=1 erthost enclave.signed
```
to run the sample.
