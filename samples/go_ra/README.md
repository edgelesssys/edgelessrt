# Go remote attestation sample
This sample shows how to do remote attestation of a Go enclave. It consists of a server running in the enclave and a client that attests the server before sending a secret.

**Note: This sample only works on SGX-FLC systems.**

The server generates a self-signed certificate and a report for remote attestation using [GetRemoteReport()](https://pkg.go.dev/github.com/edgelesssys/ego/enclave#GetRemoteReport) that includes the certificate's hash. It thereby binds the certificate to the enclave's identity.

The server runs HTTPS and serves the following:
* `/cert` makes the TLS certificate explicitly available on the HTTP layer (for simplicity for this sample).
* `/report` returns the remote report.
* `/secret` receives the secret via a query parameter named `s`.

The client first gets the certificate and the report skipping TLS certificate verification. Then it verifies the certificate using the report via [VerifyRemoteReport()](https://pkg.go.dev/github.com/edgelesssys/ego/eclient#VerifyRemoteReport). From there on it can establish a secure connection to the enclave server and send its secret.

Some error handling in this sample is omitted for brevity.

The server can be built and run as follows:
```sh
mkdir build
cd build
cmake ..
make
erthost enclave.signed
```

The client can be run either using `ertgo` or a recent Go compiler. It expects the `signer ID` (`MRSIGNER`) as an argument. The `signer ID` can be derived from the signer's public key using `oesign`:
```sh
go run client.go -s `oesign signerid -k build/public.pem`
```
