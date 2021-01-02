# syntax=docker/dockerfile:experimental

FROM ghcr.io/edgelesssys/edgelessrt-dev AS build
RUN git clone https://github.com/edgelesssys/edgelessrt /edgelessrt
WORKDIR /edgelessrt/samples/go_ra/build
RUN cmake ..
RUN --mount=type=secret,id=signingkey,dst=/edgelessrt/samples/go_ra/build/private.pem,required=true make

FROM ghcr.io/edgelesssys/edgelessrt-deploy
LABEL description="ERT sample image"
COPY --from=build /edgelessrt/samples/go_ra/build/enclave.signed /go_ra/
ENTRYPOINT ["erthost", "/go_ra/enclave.signed"]
