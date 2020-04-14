package main

import (
	"C"
	"fmt"
	"runtime"
)

func main() {}

//export gotest
func gotest() int32 {
	fmt.Println("gotest")

	// expect OE to simulate a multicore system
	if runtime.NumCPU() < 2 {
		return -1
	}

	return 42 // success magic
}
