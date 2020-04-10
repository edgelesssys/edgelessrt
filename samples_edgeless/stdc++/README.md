# stdc++ sample
This sample shows how to to link against `stdc++` instead of `oelibcxx` so that you can use (most of) the C++17 STL and link against libraries that use `stdc++`.

The sample can be built and run as follows:
```sh
mkdir build
cd build
cmake ..
make
ertdevhost enclave.signed
```
