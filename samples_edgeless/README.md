# Edgeless RT samples
These samples show how to write a new application or port an existing one to `Edgeless RT`.

* [helloworld](helloworld/README.md) is a minimal example of an enclave application written in C.
* [stdc++](stdc++/README.md) shows how to link against `stdc++` so that you can use (most of) the C++17 STL and link against libraries that use `stdc++`.
* [go](go/README.md) is a minimal example of an enclave application written in Go.

## ertdevhost
`ertdevhost` is a tool that is used during development to run enclaves. It transparently forwards all commandline arguments and environment variables to the enclave application. The enclave has unlimited access to the host's filesystem.

To use `ertdevhost`, the enclave must be linked against `ertdeventry`. The samples are configured to do so.

Once your application is ready to go into production, you can either switch to [Edgeless Mesh](#edgeless-mesh) or write a custom enclave entry function and a corresponding host app. The [helloworld](helloworld/README.md) sample includes an example of the latter.

## Edgeless Mesh
Edgeless Mesh is the preferred way to run enclave applications in production: it seamlessly integrates with Edgeless RT and enables the creation of distributed confidential applications. Learn more at <https://edgeless.systems>.
