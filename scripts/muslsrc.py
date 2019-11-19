#!/usr/bin/env python3
# EDG: This script adds all musl source files to libc/CMakeLists.txt that do not cause build errors.

import glob, os, subprocess, tempfile

exclude_srcs = ("malloc/", "mman/", "thread/", "internal/syscall_ret.c", "internal/version.c", "stdio/__lockfile.c")
oe_dir = os.path.realpath(os.path.join(os.path.dirname(__file__), ".."))
cmakelists_file = os.path.join(oe_dir, "libc", "CMakeLists.txt")
musl_src_dir = os.path.join(oe_dir, "3rdparty", "musl", "musl", "src")
muslsrc_prefix = "${MUSLSRC}/"

with open(cmakelists_file) as f:
    cmakelists_content = f.read()

# get current musl source files from CMakeLists
cmakelists_lines = [x.strip() for x in cmakelists_content.splitlines()]
current_src_files = [x[len(muslsrc_prefix):] for x in cmakelists_lines if x.startswith(muslsrc_prefix) and x.endswith(".c")]

# get all musl source files that are not currently in CMakeLists
src_files = glob.glob(os.path.join(musl_src_dir, "*", "*.c"))
src_files = [os.path.relpath(x, musl_src_dir) for x in src_files]
src_files = [x for x in src_files if x not in current_src_files and not x.startswith(exclude_srcs)]

src_files.sort()

insert_pos = cmakelists_content.find("\n    ${PLATFORM_SRC}\n")
assert insert_pos > 0

with tempfile.TemporaryDirectory() as builddir:
    with tempfile.TemporaryDirectory() as installdir:
        subprocess.run(["cmake", "-DHAS_QUOTE_PROVIDER=OFF", f"-DCMAKE_INSTALL_PREFIX={installdir}", oe_dir], check=True, cwd=builddir)
        libpath = os.path.join(installdir, "lib", "openenclave", "enclave", "*.a")

        # rebuild and remove files from src_files until no more file causes link errors
        prev_src_files = []
        while src_files != prev_src_files:
            prev_src_files = src_files.copy()

            # create a CMakeLists file containing the new source files
            with open(cmakelists_file, "w") as f:
                f.write(cmakelists_content[:insert_pos] + f"\n\n    {muslsrc_prefix}" + f"\n    {muslsrc_prefix}".join(src_files) + cmakelists_content[insert_pos:])

            subprocess.run(["make", f"-j{os.cpu_count()}"], check=True, cwd=builddir)
            subprocess.run(["make", f"-j{os.cpu_count()}", "install"], check=True, cwd=builddir)

            # --whole-archive will take every object file from the archives so that "multiple definition" errors appear even for unused symbols
            err = subprocess.run("ld --whole-archive " + libpath, shell=True, stderr=subprocess.PIPE).stderr.decode()

            src_files = [x for x in src_files if x not in err]
