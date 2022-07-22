# Partitioned application that uses a Go library

This sample shows how you can link a Go library to the enclave part of your partitioned application.

The sample is based on the [Open Enclave helloworld sample](https://github.com/openenclave/openenclave/tree/v0.18.1/samples/helloworld).
You should familiarize yourself with that first.
(It can be built with Edgeless RT. You don't need to have Open Enclave installed.)
The host part has been adopted without changes.
The rest has been reduced to the essential for brevity.

Here are the steps to add a Go library to an enclave:

1. Use ertgo to compile the Go code to a static library: [golib](golib)
2. Link the library to the enclave: [enclave/CMakeLists.txt](enclave/CMakeLists.txt)
3. Add ertlibc to the enclave for Go compatibility: [enclave/CMakeLists.txt](enclave/CMakeLists.txt) [helloworld.edl](helloworld.edl)
4. Increase NumHeapPages and NumTCS: [enclave/helloworld.conf](enclave/helloworld.conf)
5. Call an exported Go function: [enclave/enc.c](enclave/enc.c)

The sample can be built and run as follows:

```sh
mkdir build
cd build
cmake ..
make
host/helloworld_host enclave/enclave.signed
```
