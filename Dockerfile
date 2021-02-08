# syntax=docker/dockerfile:experimental

FROM ubuntu:18.04 AS common
RUN apt update && \
    apt install -y libssl-dev wget gnupg software-properties-common locales
RUN wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | apt-key add - && \
    apt-add-repository 'https://download.01.org/intel-sgx/sgx_repo/ubuntu bionic main'
RUN wget -qO - https://packages.microsoft.com/keys/microsoft.asc | apt-key add - && \
    apt-add-repository 'https://packages.microsoft.com/ubuntu/18.04/prod bionic main' && \
    apt clean && apt autoclean
RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && \
    locale-gen
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL en_US.UTF-8

FROM common AS sgx
RUN apt update && \
    apt install -y --no-install-recommends libsgx-dcap-ql libsgx-dcap-ql-dev libsgx-dcap-quote-verify libsgx-enclave-common libsgx-urts az-dcap-client && \
    apt clean && apt autoclean

FROM sgx AS sgx-dev
RUN apt update && \
    apt install -y libsgx-enclave-common-dev libsgx-ae-qve libsgx-ae-pce libsgx-ae-qe3 libsgx-qe3-logic libsgx-pce-logic && \
    apt clean && apt autoclean

FROM sgx-dev AS base-dev
RUN wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc | apt-key add - && \
    apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
RUN add-apt-repository ppa:git-core/ppa
RUN apt update && \
    apt install -y protobuf-compiler golang-goprotobuf-dev cmake git python ninja-build build-essential gdb ca-certificates zlib1g-dev doxygen nano vim curl clang-tidy-10 && \
    apt clean && apt autoclean

FROM alpine/git:latest AS pull
RUN git clone https://github.com/edgelesssys/edgelessrt /edgelessrt
WORKDIR /edgelessrt
RUN git submodule update --init 3rdparty/openenclave/openenclave 3rdparty/go
WORKDIR /edgelessrt/3rdparty/openenclave/openenclave
RUN git submodule update --init tools/oeedger8r-cpp 3rdparty/mbedtls/mbedtls

FROM base-dev AS build
COPY --from=pull /edgelessrt /edgelessrt
WORKDIR /edgelessrt/build
RUN cmake -GNinja -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && ninja install

FROM base-dev as release_develop
LABEL description="EdgelessRT is an SDK to build Trusted Execution Environment applications"
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/edgelessrt/share/pkgconfig
ENV CMAKE_PREFIX_PATH=/opt/edgelessrt/lib/openenclave/cmake
ENV PATH=${PATH}:/opt/edgelessrt/bin/:/opt/edgelessrt/go/bin/
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
    apt-add-repository --remove 'deb https://apt.kitware.com/ubuntu/ bionic main' && \
    apt remove -y wget gnupg software-properties-common
ENTRYPOINT ["bash"]
