# ERT patch for Open Enclave submodule
Edgeless RT is based on Open Enclave. We try to implement most parts in our own libraries (see src folder in the root directory) and upstream required changes to OE. However, some of these changes are too specific to ERT and wouldn't make sense on its own or break some features of OE (that we don't use). Thus, we keep them as a patch file that is automatically applied on build.

If you need to make changes to OE for a new ERT feature, you can set up your environment like this:
```sh
mkdir build
cd build
cmake -GNinja -DOEDEV=ON ..
```
Enabling OEDEV will leave the `openenclave` subfolder as is (instead of copying it to the build folder and applying the patch), so modifications can immediately built and tested.

Open a second instance of your IDE at `3rdparty/openenclave/openenlave` and apply the patch:
```sh
patch -p1 < ../ert.patch
git commit -a -m ert
```
Now you can make changes to both ERT and OE. You only need to build ERT which automatically builds the modified OE.

Once you are done, create a new patch file (within `3rdparty/openenclave/openenclave`):
```sh
git diff HEAD^ > ../ert.patch
```
