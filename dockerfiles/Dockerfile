FROM ghcr.io/edgelesssys/edgelessrt/build-base:v0.4.10 AS build

# don't run `apt-get update` because required packages are cached in build-base for reproducibility
RUN apt-get install -y --no-install-recommends \
  build-essential \
  ca-certificates \
  clang-11 \
  cmake \
  git \
  libssl-dev \
  ninja-build \
  wget

ARG erttag=v0.4.10
RUN git clone -b $erttag --depth=1 https://github.com/edgelesssys/edgelessrt \
  && mkdir build

RUN cd edgelessrt && export SOURCE_DATE_EPOCH=$(git log -1 --pretty=%ct) && cd /build \
  && cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF /edgelessrt \
  && ninja install \
  && cpack -G DEB \
  && DEBNAME=$(ls edgelessrt_*_amd64.deb) \
  && mv $DEBNAME ${DEBNAME%.*}_ubuntu-22.04.deb

FROM scratch AS export
COPY --from=build /build/edgelessrt_*_amd64_ubuntu-22.04.deb /


FROM ubuntu:jammy-20250619 AS release_deploy
LABEL description="Containerized SGX for release"

ARG PSW_VERSION=2.26.100.0-jammy1
ARG DCAP_VERSION=1.23.100.0-jammy1
RUN apt-get update && apt-get install -y --no-install-recommends ca-certificates libcurl4 wget \
  && wget -Petc https://github.com/intel/SGXDataCenterAttestationPrimitives/raw/DCAP_1.23/QuoteGeneration/qcnl/linux/sgx_default_qcnl_azure.conf \
  && wget -qO /etc/apt/keyrings/intel-sgx-keyring.asc https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key \
  && echo 'deb [signed-by=/etc/apt/keyrings/intel-sgx-keyring.asc arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu jammy main' > /etc/apt/sources.list.d/intel-sgx.list \
  && apt-get update && apt-get install -y --no-install-recommends \
  libsgx-ae-id-enclave=$DCAP_VERSION \
  libsgx-ae-pce=$PSW_VERSION \
  libsgx-ae-qe3=$DCAP_VERSION \
  libsgx-dcap-default-qpl=$DCAP_VERSION \
  libsgx-dcap-ql=$DCAP_VERSION \
  libsgx-enclave-common=$PSW_VERSION \
  libsgx-launch=$PSW_VERSION \
  libsgx-pce-logic=$DCAP_VERSION \
  libsgx-qe3-logic=$DCAP_VERSION \
  libsgx-urts=$PSW_VERSION

COPY --from=build /opt/edgelessrt/bin/erthost /edgelessrt/dockerfiles/configure-qpl /opt/edgelessrt/bin/
ENV PATH=$PATH:/opt/edgelessrt/bin


FROM release_deploy as release_develop
LABEL description="EdgelessRT is an SDK to build Trusted Execution Environment applications"
RUN apt-get update && apt-get install -y \
  build-essential \
  clang-11 \
  clang-tidy-11 \
  cmake \
  curl \
  gdb \
  git \
  libssl-dev \
  nano \
  ninja-build \
  pkg-config \
  vim \
  zlib1g-dev \
  # use same Go version as ertgo
  && wget -qO- https://go.dev/dl/go1.23.5.linux-amd64.tar.gz | tar -C /usr/local -xz
COPY --from=build /opt/edgelessrt /opt/edgelessrt
ENV PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/edgelessrt/share/pkgconfig
ENV CMAKE_PREFIX_PATH=/opt/edgelessrt/lib/openenclave/cmake
ENV PATH=${PATH}:/opt/edgelessrt/bin:/usr/local/go/bin
ENV CGO_CFLAGS="$CGO_CFLAGS -I/opt/edgelessrt/include"
ENV CGO_LDFLAGS="$CGO_LDFLAGS -L/opt/edgelessrt/lib/openenclave/host"
