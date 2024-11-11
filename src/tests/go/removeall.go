// Copyright 2024 Edgeless Systems GmbH. All rights reserved.
// Copyright 2018 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

package main

import (
	"fmt"
	"os"
	"path/filepath"
)

func testRemoveAll() error {
	tmpDir := "/removeall-test"
	if err := os.Mkdir(tmpDir, 0o400); err != nil {
		return err
	}

	if err := os.RemoveAll(""); err != nil {
		return fmt.Errorf("RemoveAll(\"\"): %v; want nil", err)
	}

	file := filepath.Join(tmpDir, "file")
	path := filepath.Join(tmpDir, "_TestRemoveAll_")
	fpath := filepath.Join(path, "file")
	dpath := filepath.Join(path, "dir")

	// Make a regular file and remove
	fd, err := os.Create(file)
	if err != nil {
		return fmt.Errorf("create %q: %s", file, err)
	}
	fd.Close()
	if err = os.RemoveAll(file); err != nil {
		return fmt.Errorf("RemoveAll %q (first): %s", file, err)
	}
	if _, err = os.Lstat(file); err == nil {
		return fmt.Errorf("Lstat %q succeeded after RemoveAll (first)", file)
	}

	// Make directory with 1 file and remove.
	if err := os.MkdirAll(path, 0777); err != nil {
		return fmt.Errorf("MkdirAll %q: %s", path, err)
	}
	fd, err = os.Create(fpath)
	if err != nil {
		return fmt.Errorf("create %q: %s", fpath, err)
	}
	fd.Close()
	if err = os.RemoveAll(path); err != nil {
		return fmt.Errorf("RemoveAll %q (second): %s", path, err)
	}
	if _, err = os.Lstat(path); err == nil {
		return fmt.Errorf("Lstat %q succeeded after RemoveAll (second)", path)
	}

	// Make directory with file and subdirectory and remove.
	if err = os.MkdirAll(dpath, 0777); err != nil {
		return fmt.Errorf("MkdirAll %q: %s", dpath, err)
	}
	fd, err = os.Create(fpath)
	if err != nil {
		return fmt.Errorf("create %q: %s", fpath, err)
	}
	fd.Close()
	fd, err = os.Create(dpath + "/file")
	if err != nil {
		return fmt.Errorf("create %q: %s", fpath, err)
	}
	fd.Close()
	if err = os.RemoveAll(path); err != nil {
		return fmt.Errorf("RemoveAll %q (third): %s", path, err)
	}
	if _, err := os.Lstat(path); err == nil {
		return fmt.Errorf("Lstat %q succeeded after RemoveAll (third)", path)
	}

	return nil
}
