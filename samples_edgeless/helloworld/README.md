# helloworld sample
This sample shows how to build an enclave application written in C.

The directory `app` contains the application code. This is the part that runs within the secure enclave.

`enclave.conf` defines SGX parameters like heap size or maximum thread count.

The sample can be built and run as follows:
```sh
mkdir build
cd build
cmake ..
make
ertdevhost enclave.signed
```

## custom host
As stated [here](../README.md#ertdevhost) `ertdevhost` must not be used in production. For modern distributed applications, it is recommended to use [Edgeless Mesh](../README.md#edgeless-mesh). Alternatively, for a standalone application, you can write a custom host application. An example is given in the directory `custom_host`. It can be built and run as follows:
```sh
mkdir build
cd build
cmake ../custom_host
make
./helloworld_host enclave.signed
```
