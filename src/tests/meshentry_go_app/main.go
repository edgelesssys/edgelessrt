package main

import (
	"io/ioutil"
	"os"
)

func main() {
	if len(os.Args) != 2 {
		panic("arg len")
	}
	if os.Args[0] != "arg0" {
		panic("arg 0")
	}
	if os.Args[1] != "arg1" {
		panic("arg 1")
	}
	if os.Getenv("key") != "value" {
		panic("env")
	}
	if os.Getenv("EDG_FOO") != "bar" {
		panic("EDG_FOO")
	}
	if os.Getenv("ABC_FOO") != "" {
		panic("ABC_FOO")
	}
	data, err := ioutil.ReadFile("/folder1/folder2/file")
	if err != nil {
		panic(err)
	}
	if string(data) != "test" {
		panic("file")
	}
}

func init() {
	if len(os.Args) < 1 {
		panic("init: Args")
	}
}
