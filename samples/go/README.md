# Go sample
This sample shows how to build an enclave application written in Go.

The directory `app` contains the Go application code:
* `hello.go` contains the application's main function. This is ordinary Go code.
* `invokemain.go` exports a function that can be called from C, which serves as the application's entry point. This function just invokes the real main function.

The split between the two source files is not strictly necessary, but it demonstrates how a Go application can be ported without changing its source code.

`enclave.conf` defines SGX parameters like heap size or maximum thread count. Note that Go requires relatively much heap space because its allocator reserves memory in large chunks.

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
