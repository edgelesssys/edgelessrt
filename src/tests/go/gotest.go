package main

import "C"

import (
	"fmt"
	"net"
	"os"
	"os/exec"
	"runtime"
	"syscall"
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

	if _, err := net.ResolveIPAddr("ip", "localhost"); err != nil {
		return -4
	}
	if _, err := net.ResolveIPAddr("ip", "127.0.0.1"); err != nil {
		return -5
	}

	if _, err := os.Stat("/"); err != nil {
		return -6
	}
	if _, err := os.Lstat("/"); err != nil {
		return -7
	}

	if exec.Command("").Run().Error() != "exec not supported" {
		return -8
	}

	if _, err := net.LookupCNAME("localhost"); err != nil {
		return -9
	}

	if syscall.Getpid() <= 0 {
		return -10
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
