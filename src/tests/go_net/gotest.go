package main

import "C"

import (
	"fmt"
	"net"
)

func main() {}

//export gotest
func gotest() int32 {
	fmt.Println("gotest")

	li, err := net.Listen("tcp", "127.0.0.1:")
	if err != nil {
		return -1
	}

	go func() {
		cl, err := net.Dial("tcp", li.Addr().String())
		if err != nil {
			panic(err)
		}
		if _, err := cl.Write([]byte("foo")); err != nil {
			panic(err)
		}
		if err := cl.Close(); err != nil {
			panic(err)
		}
	}()

	se, err := li.Accept()
	if err != nil {
		return -2
	}
	bu := make([]byte, 3)
	se.Read(bu)
	se.Close()
	li.Close()
	if string(bu) != "foo" {
		return -3
	}

	return 42 // success magic
}
