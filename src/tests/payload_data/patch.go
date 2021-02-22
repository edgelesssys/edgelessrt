package main

import (
	"debug/elf"
	"encoding/binary"
	"errors"
	"os"
)

func main() {
	if len(os.Args) < 2 {
		panic(errors.New("need filename as second parameter"))
	}
	filename := os.Args[1]

	// Open ELF file
	file, err := os.OpenFile(filename, os.O_RDWR, 0644)
	if err != nil {
		panic(err)
	}
	defer file.Close()

	offsetToFoo := appendFoo(file)
	offsetToOEInfo := getOEInfoSection(file)
	patchOEInfoSection(file, offsetToFoo, offsetToOEInfo)
}

func appendFoo(file *os.File) uint64 {
	// Get current size, which we will use as an offset later
	fileStat, err := file.Stat()
	if err != nil {
		panic(err)
	}
	offset := fileStat.Size()

	// Append "foo" to the file
	if _, err := file.WriteAt([]byte("foo"), offset); err != nil {
		panic(err)
	}

	// Return the offset to "foo"
	return uint64(offset)
}

func getOEInfoSection(file *os.File) int64 {
	elfFile, err := elf.NewFile(file)
	defer elfFile.Close()

	if err != nil {
		panic(err)
	}

	// Get offset of .oeinfo section
	oeInfo := elfFile.Section(".oeinfo")
	if oeInfo == nil {
		panic(errors.New("could not find .oeinfo section"))
	}

	return int64(oeInfo.Offset)
}

func patchOEInfoSection(file *os.File, offsetToFoo uint64, offsetToOEInfo int64) {
	// Calculate 64-bit little-endian offset & size (as amd64 is little-endian)
	offset := make([]byte, 8)
	size := make([]byte, 8)
	binary.LittleEndian.PutUint64(offset, offsetToFoo)
	binary.LittleEndian.PutUint64(size, uint64(len("foo")))

	// write offset to .oeinfo + 2048 bytes
	n, err := file.WriteAt(offset, offsetToOEInfo+2048)
	if err != nil {
		panic(err)
	} else if n != 8 {
		panic(errors.New("a wrong amount of bytes has been written"))
	}

	// write size to .oeinfo + 2056 bytes
	n, err = file.WriteAt(size, offsetToOEInfo+2056)
	if err != nil {
		panic(err)
	} else if n != 8 {
		panic(errors.New("a wrong amount of bytes has been written"))
	}
}
