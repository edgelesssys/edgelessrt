ARG PSW_VERSION=2.17.100.3-focal1
ARG DCAP_VERSION=1.14.100.3-focal1

FROM ubuntu:20.04 AS sgx
ARG PSW_VERSION
ARG DCAP_VERSION
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates gnupg wget \
    && wget -qO- https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | apt-key add \
    && echo 'deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu focal main' >> /etc/apt/sources.list \
    && wget -qO- https://packages.microsoft.com/keys/microsoft.asc | apt-key add \
    && echo 'deb [arch=amd64] https://packages.microsoft.com/ubuntu/20.04/prod focal main' >> /etc/apt/sources.list \
    && apt-get update && apt-get install -y --no-install-recommends \
    az-dcap-client \
    libsgx-ae-id-enclave=$DCAP_VERSION \
    libsgx-ae-pce=$PSW_VERSION \
    libsgx-ae-qe3=$DCAP_VERSION \
    libsgx-dcap-default-qpl=$DCAP_VERSION \
    libsgx-dcap-ql=$DCAP_VERSION \
    libsgx-dcap-ql-dev=$DCAP_VERSION \
    libsgx-enclave-common=$PSW_VERSION \
    libsgx-headers=$PSW_VERSION \
    libsgx-launch=$PSW_VERSION \
    libsgx-pce-logic=$DCAP_VERSION \
    libsgx-qe3-logic=$DCAP_VERSION \
    libsgx-urts=$PSW_VERSION
# move the shared libraries installed by libsgx-dcap-default-qpl and remove the package
# recreating the link /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.1 to /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.intel restores functionality of the original library
RUN mkdir /usr/lib/x86_64-linux-gnu/dcap && \
    cp /usr/lib/x86_64-linux-gnu/libsgx_default_qcnl_wrapper.so.1 /usr/lib/x86_64-linux-gnu/libdcap_quoteprov.so.1 /usr/lib/x86_64-linux-gnu/dcap && \
    apt remove -y libsgx-dcap-default-qpl && \
    ln -s /usr/lib/x86_64-linux-gnu/dcap/libsgx_default_qcnl_wrapper.so.1 /usr/lib/x86_64-linux-gnu/libsgx_default_qcnl_wrapper.so.1 && \
    ln -s /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.1 /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.intel

FROM sgx AS base-dev
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    build-essential \
    cmake \
    clang-10 \
    clang-tidy-10 \
    curl \
    doxygen \
    gdb \
    git \
    libssl-dev \
    nano \
    ninja-build \
    pkg-config \
    vim \
    zlib1g-dev
# use same Go version as ertgo
RUN wget -qO- https://go.dev/dl/go1.18.1.linux-amd64.tar.gz | tar -C /usr/local -xz

FROM alpine/git:latest AS pull
RUN git clone --depth=1 https://github.com/edgelesssys/edgelessrt /edgelessrt
WORKDIR /edgelessrt
RUN git submodule update --init --depth=1 3rdparty/openenclave/openenclave 3rdparty/go 3rdparty/mystikos/mystikos 3rdparty/ttls
WORKDIR /edgelessrt/3rdparty/openenclave/openenclave
RUN git submodule update --init tools/oeedger8r-cpp 3rdparty/mbedtls/mbedtls 3rdparty/musl/musl 3rdparty/musl/libc-test

FROM base-dev AS build
COPY --from=pull /edgelessrt /edgelessrt
WORKDIR /edgelessrt/build
RUN cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTS=OFF .. && ninja install

FROM base-dev as release_develop
LABEL description="EdgelessRT is an SDK to build Trusted Execution Environment applications"
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/edgelessrt/share/pkgconfig
ENV CMAKE_PREFIX_PATH=/opt/edgelessrt/lib/openenclave/cmake
ENV PATH=${PATH}:/opt/edgelessrt/bin:/usr/local/go/bin
ENV CGO_CFLAGS="$CGO_CFLAGS -I/opt/edgelessrt/include"
ENV CGO_LDFLAGS="$CGO_LDFLAGS -L/opt/edgelessrt/lib/openenclave/host"
COPY --from=build /opt/edgelessrt /opt/edgelessrt
ENTRYPOINT ["bash"]

FROM sgx AS release_deploy
LABEL description="Containerized SGX for release"
ENV PATH=${PATH}:/opt/edgelessrt/bin/
COPY --from=build /opt/edgelessrt/bin/erthost /opt/edgelessrt/bin/erthost
ENTRYPOINT ["bash"]
