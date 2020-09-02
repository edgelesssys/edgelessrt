package main

import (
	"C"
	"bytes"
	"fmt"

	"github.com/edgelesssys/ertgolib/ertenclave"
)

func main() {}

//export gotest
func gotest() int32 {
	fmt.Println("gotest")

	for _, f := range []func() ([]byte, []byte, error){ertenclave.GetUniqueSealKey, ertenclave.GetProductSealKey} {
		key, info, err := f()
		if err != nil {
			return -1
		}
		if len(key) <= 0 || len(info) <= 0 {
			return -2
		}
		keyByInfo, err := ertenclave.GetSealKey(info)
		if err != nil {
			return -3
		}
		if !bytes.Equal(keyByInfo, key) {
			return -4
		}
	}

	return 42 // success magic
}
