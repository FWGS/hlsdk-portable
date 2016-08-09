# Half-Life SDK for Xash3D [![Build Status](https://travis-ci.org/FWGS/hlsdk-xash3d.svg)](https://travis-ci.org/FWGS/hlsdk-xash3d)

Half-Life SDK for Xash3D & GoldSource with some fixes.

## How to build

### CMake as most universal way

```
mkdir build && cd build
cmake ../
```

You may enable or disable some build options by -Dkey=value. All available build options are defined in CMakeLists.txt at root directory.

See below, if CMake is not suitable for you:

### Windows

TODO

### Linux

TODO

### OS X

TODO

### FreeBSD

```
    cd dlls
    gmake CXX=clang++ CC=clang
    cd ../cl_dll
    gmake CXX=clang++ CC=clang
```

### Android

Just typical `ndk-build`.

