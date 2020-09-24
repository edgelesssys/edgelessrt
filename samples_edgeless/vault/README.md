# Hashicorp Vault sample
This sample shows how to port an existing Go application to `Edgeless RT`.

To build Vault for the enclave, first compile the (unmodified) Vault project to a static library using the Edgeless Go compiler:
```sh
git clone https://github.com/hashicorp/vault
cp invokemain.go vault
cd vault
ertgo build -buildmode=c-archive main.go invokemain.go
cd ..
```
This will produce `main.a`.

Now you can build the enclave:
```sh
mkdir build
cd build
cmake -DGOLIB=../vault/main.a ..
make
```

Run the Vault enclave:
```sh
erthost enclave.signed server -dev
```
