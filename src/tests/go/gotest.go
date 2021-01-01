package main

import (
	"C"
	"fmt"
	"runtime"
)

func main() {}

//export gotest
func gotest(simulate bool) int32 {
	fmt.Println("gotest")

	// expect OE to simulate a multicore system
	if runtime.NumCPU() < 2 {
		return -1
	}

	if testPanicRecover() != 2 {
		return -2
	}

	if !simulate && testNilRecover() != 2 {
		return -3
	}

	return 42 // success magic
}

func testPanicRecover() (r int) {
	defer func() {
		if recover() != nil {
			r = 2
		}
	}()
	panic("foo")
	return 3
}

func testNilRecover() (r int) {
	defer func() {
		if recover() != nil {
			r = 2
		}
	}()
	*(*int)(nil) = 0
	return 3
}
