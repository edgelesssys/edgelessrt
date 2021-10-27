ARG PSW_VERSION=2.15.100.3
ARG DCAP_VERSION=1.12.100.3
ARG QUOTE_PROV_TAG=1.11.101.1

FROM ubuntu:20.04 AS common
RUN apt update && \
    apt install -y libssl-dev wget gnupg software-properties-common locales
RUN wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | apt-key add - && \
    apt-add-repository 'https://download.01.org/intel-sgx/sgx_repo/ubuntu main'
RUN wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add - && \
    apt-add-repository 'https://packages.microsoft.com/ubuntu/20.04/prod main' && \
    apt clean && apt autoclean
RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && \
    locale-gen
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

FROM common AS sgx
ARG PSW_VERSION
ARG DCAP_VERSION
ARG QUOTE_PROV_TAG
RUN apt update && \
    apt install -y --no-install-recommends \
    az-dcap-client \
    libsgx-ae-pce=$PSW_VERSION-focal1 \
    libsgx-ae-qe3=$DCAP_VERSION-focal1 \
    libsgx-ae-qve=$DCAP_VERSION-focal1 \
    libsgx-dcap-default-qpl=$DCAP_VERSION-focal1 \
    libsgx-dcap-ql=$DCAP_VERSION-focal1 \
    libsgx-dcap-ql-dev=$DCAP_VERSION-focal1 \
    libsgx-enclave-common=$PSW_VERSION-focal1 \
    libsgx-headers=$PSW_VERSION-focal1 \
    libsgx-launch=$PSW_VERSION-focal1 \
    libsgx-pce-logic=$DCAP_VERSION-focal1 \
    libsgx-qe3-logic=$DCAP_VERSION-focal1 \
    libsgx-urts=$PSW_VERSION-focal1 \
    && apt clean && apt autoclean
# move the shared libraries installed by libsgx-dcap-default-qpl and remove the package
# recreating the link /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.1 to /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.intel restores functionality of the original library
RUN mkdir /usr/lib/x86_64-linux-gnu/dcap && \
    cp /usr/lib/x86_64-linux-gnu/libsgx_default_qcnl_wrapper.so.$QUOTE_PROV_TAG /usr/lib/x86_64-linux-gnu/libdcap_quoteprov.so.$QUOTE_PROV_TAG /usr/lib/x86_64-linux-gnu/dcap && \
    apt remove -y libsgx-dcap-default-qpl && \
    ln -s /usr/lib/x86_64-linux-gnu/dcap/libsgx_default_qcnl_wrapper.so.$QUOTE_PROV_TAG /usr/lib/x86_64-linux-gnu/libsgx_default_qcnl_wrapper.so.1 && \
    ln -s /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.$QUOTE_PROV_TAG /usr/lib/x86_64-linux-gnu/dcap/libdcap_quoteprov.so.intel

FROM sgx AS base-dev
RUN wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc | apt-key add - && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'
RUN add-apt-repository ppa:git-core/ppa
RUN apt update && \
    apt install -y protobuf-compiler golang-goprotobuf-dev cmake git python ninja-build build-essential gdb ca-certificates zlib1g-dev doxygen nano vim curl clang-10 clang-tidy-10 && \
    apt clean && apt autoclean
# use same Go version as ertgo
ARG gofile=go1.16.7.linux-amd64.tar.gz
RUN wget https://golang.org/dl/$gofile && tar -C /usr/local -xzf $gofile && rm $gofile

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
RUN apt-key del 6D903995424A83A48D42D53DA8E5EF3A02600268 && \
    apt-key del BC528686B50D79E339D3721CEB3E94ADBE1229CF && \
    apt-key del 35BFD5E1AEFFA8C4996DDD0DAA65AD26261B320B && \
    apt-add-repository --remove ppa:git-core/ppa && \
    apt-add-repository --remove 'deb https://apt.kitware.com/ubuntu/ focal main' && \
    apt remove -y wget gnupg software-properties-common
ENTRYPOINT ["bash"]
