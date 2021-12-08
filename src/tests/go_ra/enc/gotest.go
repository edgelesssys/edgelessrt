package main

import (
	"C"
	"bytes"
	"encoding/binary"
	"fmt"
	"math/rand"

	"github.com/edgelesssys/ego/enclave"
)

func main() {}

//export gotest
func gotest() int32 {
	fmt.Println("gotest")

	signer := []byte{ // from docs/GettingStartedDocs/buildandsign.md
		0xca, 0x9a, 0xd7, 0x33, 0x14, 0x48, 0x98, 0x0a,
		0xa2, 0x88, 0x90, 0xce, 0x73, 0xe4, 0x33, 0x63,
		0x83, 0x77, 0xf1, 0x79, 0xab, 0x44, 0x56, 0xb2,
		0xfe, 0x23, 0x71, 0x93, 0x19, 0x3a, 0x8d, 0xa}

	reportData := make([]byte, 64)
	rand.Read(reportData)

	reportBytes, err := enclave.GetRemoteReport(reportData)
	if err != nil {
		return -1
	}

	report, err := enclave.VerifyRemoteReport(reportBytes)
	if err != nil {
		return -2
	}

	if !bytes.Equal(report.Data, reportData) {
		return -3
	}
	if report.SecurityVersion != 2 {
		return -4
	}
	if !report.Debug {
		return -5
	}
	if len(report.UniqueID) != 32 {
		return -6
	}
	if bytes.Equal(report.UniqueID, make([]byte, 32)) {
		return -7
	}
	if !bytes.Equal(report.SignerID, signer) {
		return -8
	}
	if binary.LittleEndian.Uint16(report.ProductID) != 1234 {
		return -9
	}

	return 42 // success magic
}
