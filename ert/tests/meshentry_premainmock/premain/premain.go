package premain

import (
	"io/ioutil"
	"os"
	"syscall"
)

// Premain mocks Mesh's premain
func Premain() error {
	if err := os.Setenv("key", "value"); err != nil {
		return err
	}
	if err := syscall.Mount("/", "/folder1", "edg_memfs", 0, ""); err != nil {
		return err
	}
	if err := ioutil.WriteFile("/folder1/folder2/file", []byte("test"), 0); err != nil {
		return err
	}

	os.Args = make([]string, 2)
	os.Args[0] = "arg0"
	os.Args[1] = "arg1"

	return nil
}
