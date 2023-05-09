# Edgeless RT
![ERT logo](docs/ert_logo.svg)

[![Unit Tests][unit-tests-badge]][unit-tests]
[![GitHub license][license-badge]](LICENSE)
[![Discord Chat][discord-badge]][discord]

[Edgeless RT](https://edgeless.systems) is an SDK for Trusted Execution Environments (TEE) built on top of [Open Enclave](https://github.com/openenclave/openenclave). It adds support for modern programming languages (in particular Go) and facilitates the porting of existing applications.
Currently, hardware-wise, Edgeless RT focuses on [Intel SGX](https://software.intel.com/en-us/sgx). Support for other TEEs will follow as it becomes available in Open Enclave.

Key features of Edgeless RT are:
* Comprehensive support for Go, most existing code runs without changes
  * Preferably use [EGo](https://github.com/edgelesssys/ego) to build confidential Go apps.
  * Use Edgeless RT if you need more control, e.g., you may want to link some Go code to your C++ app.
* Extended C/C++ support
  * More libc and POSIX functions
  * More C++17 STL
  * pthread and std::thread
  * libstdc++ for better compatibility with existing code
* Seamless integration with [MarbleRun](https://github.com/edgelesssys/marblerun) to create distributed confidential applications
* Experimental support for Rust

## Quick Start
If you're on Ubuntu 18.04 or above and don't want to build the SDK yourself, you can install the binary release:
```bash
wget -qO- https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add
sudo add-apt-repository "deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu `lsb_release -cs` main"
wget https://github.com/edgelesssys/edgelessrt/releases/download/v0.3.9/edgelessrt_0.3.9_amd64.deb
sudo apt install ./edgelessrt_0.3.9_amd64.deb build-essential libssl-dev
sudo snap install cmake --classic
```
Then proceed with [Use](#use).

## Build
On Ubuntu 20.04 build with:
```bash
sudo apt install build-essential clang-11 cmake gdb libssl-dev ninja-build python3
mkdir build
cd build
cmake -GNinja ..
ninja
```
To set a custom installation path (default: `/opt/edgelessrt`), add, e.g., `-DCMAKE_INSTALL_PREFIX=~/edgelessrt-install`.

## SGX packages
To run your applications in SGX mode, install these packages:
```bash
wget -qO- https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add
sudo add-apt-repository "deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu `lsb_release -cs` main"
sudo apt install libsgx-dcap-ql libsgx-enclave-common libsgx-launch
```

## Test
After building, run the following command in the build directory to confirm everything works as expected:

```bash
ctest
```

In simulation mode run this command instead:
```bash
OE_SIMULATION=1 ctest
```

## Install
From the build directory run:
```bash
ninja install
```
Or if you do not have write permissions for the installation path:
```bash
sudo ninja install
```

## Use
To use the SDK you need to source the `openenclaverc` file to setup environment variables:
```bash
. /opt/edgelessrt/share/openenclave/openenclaverc
```

Now you are ready to build applications with Edgeless RT! To start, check out the [samples](samples).

Also see the [C API documentation](https://edgelesssys.github.io/edgelessrt) and/or the [Go API documentation](https://pkg.go.dev/github.com/edgelesssys/ego).

## Debug

### Logging
Set the environment variable `OE_LOG_LEVEL` to `NONE`, `FATAL`, `ERROR` (default), `WARNING`, `INFO`, or `VERBOSE` to increase or decrease the log level. Set `OE_LOG_DETAILED=1` to enrich the log output with timestamps, thread ids, and stacktrace-like error propagations.

### gdb
![debugging with vscode](docs/go_debugging_vscode.gif)

You can use Open Enclave's `oegdb` to debug enclave code built with Edgeless RT. `oegdb` is automatically installed with Edgeless RT. It also supports Go enclaves.

`oegdb` works great with Visual Studio Code (vscode). For example, use the following configuration to debug the in-enclave Go code from our [HashiCorp Vault sample](samples/vault) in vscode:

```json
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "(oegdb) Launch",
      "miDebuggerPath": "/opt/edgelessrt/bin/oegdb",
      "type": "cppdbg",
      "request": "launch",
      "program": "/opt/edgelessrt/bin/erthost",
      "args": ["enclave.signed","server","-dev"],
      "stopAtEntry": false,
      "cwd": "${workspaceFolder}/samples/vault/build/",
      "environment": [],
      "externalConsole": false,
      "MIMode": "gdb",
      "setupCommands": [
          {
              "description": "Enable pretty-printing for gdb",
              "text": "-enable-pretty-printing",
              "ignoreFailures": true
          },
          {
              "text": "handle SIGILL nostop"
          }
      ]
    }
  ]
}
```

## Contribute

Read [CONTRIBUTING.md](CONTRIBUTING.md) for information on issue reporting, code guidelines, and our PR process.

<!-- refs -->
[unit-tests]: https://github.com/edgelesssys/edgelessrt/actions
[unit-tests-badge]: https://github.com/edgelesssys/edgelessrt/workflows/Unit%20Tests/badge.svg
[license-badge]: https://img.shields.io/github/license/edgelesssys/edgelessrt
[discord]: https://discord.gg/rH8QTH56JN
[discord-badge]: https://img.shields.io/badge/chat-on%20Discord-blue
