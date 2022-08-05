# Half-Life SDK for GoldSource and Xash3D [![Build Status](https://github.com/FWGS/hlsdk-portable/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/FWGS/hlsdk-portable/actions/workflows/build.yml) [![Windows Build Status](https://ci.appveyor.com/api/projects/status/github/FWGS/hlsdk-portable?svg=true)](https://ci.appveyor.com/project/a1batross/hlsdk-portable)

Half-Life SDK for GoldSource & Xash3D with some bugfixes.

# Obtaining source code

Either clone the repository via [git](`https://git-scm.com/downloads`) or just download ZIP via **Code** button on github. The first option is more preferable as it also allows you to search through the repo history, switch between branches and clone the vgui submodule.

To clone the repository with git type in Git Bash (on Windows) or in terminal (on Unix-like operating systems):

```
git clone --recursive https://github.com/FWGS/hlsdk-portable
```

# Build Instructions

## Windows

### Prerequisites

Install and run [Visual Studio Installer](https://visualstudio.microsoft.com/downloads/). The installer allows you to choose specific components. Select `Desktop development with C++`. You can untick everything you don't need in Installation details, but you must keep `MSVC` ticked. You may also keep `C++ CMake tools for Windows` ticked as you'll need **cmake**. Alternatively you can install **cmake** from the [cmake.org](https://cmake.org/download/) and during installation tick *Add to the PATH...*.

### Opening command prompt

If **cmake** was installed with Visual Studio Installer, you'll need to run `Developer command prompt for VS` via Windows `Start` menu. If **cmake** was installed with cmake installer, you can run the regular Windows `cmd`.

Inside the prompt navigate to the hlsdk directory, using `cd` command, e.g.
```
cd C:\Users\username\projects\hlsdk-portable
```

Note: if hlsdk-portable is unpacked on another disk, nagivate there first:
```
D:
cd projects\hlsdk-portable
```

### Building

Сonfigure the project:
```
cmake -A Win32 -B build
```
Once you configure the project you don't need to call `cmake` anymore unless you modify `CMakeLists.txt` files or want to reconfigure the project with different parameters.

The next step is to compile the libraries:
```
cmake --build build --config Release
```
`hl.dll` and `client.dll` will appear in the `build/dlls/Release` and `build/cl_dll/Release` directories.

If you have a mod and want to automatically install libraries to the mod directory, set **GAMEDIR** variable to the directory name and **CMAKE_INSTALL_PREFIX** to your Half-Life or Xash3D installation path:
```
cmake -A Win32 -B build -DGAMEDIR=mod -DCMAKE_INSTALL_PREFIX="C:\Program Files (x86)\Steam\steamapps\common\Half-Life"
```
Then call `cmake` with `--target install` parameter:
```
cmake --build build --config Release --target install
```

#### Choosing Visual Studio version

You can explicitly choose a Visual Studio version on the configuration step by specifying cmake generator:
```
cmake -G "Visual Studio 16 2019" -A Win32 -B build
```

### Editing code in Visual Studio

After the configuration step, `HLSDK-PORTABLE.sln` should appear in the `build` directory. You can open this solution in Visual Studio and continue developing there.

## Windows. Using Microsoft Visual Studio 6

Microsoft Visual Studio 6 is very old, but if you still have it installed, you can use it to build this hlsdk. There are no project files, but two `.bat` files, for server and client libraries. They require variable **MSVCDir** to be set to the installation path of Visual Studio:

```
set MSVCDir=C:\Program Files\Microsoft Visual Studio
cd dlls && compile.bat && cd ../cl_dll && compile.bat
```

`hl.dll` and `client.dll` will appear in `dlls/` and `cl_dll/` diretories. The libraries built with msvc6 should be compatible with Windows XP.

## Linux. Using Steam Runtime in chroot

### Prerequisites

The official way to build Steam compatible games for Linux is through steam-runtime.

Install schroot. On Ubuntu or Debian:

```
sudo apt install schroot
```

Clone https://github.com/ValveSoftware/steam-runtime and follow instructions: [download](https://github.com/ValveSoftware/steam-runtime/blob/e014a74f60b45a861d38a867b1c81efe8484f77a/README.md#downloading-a-steam-runtime) and [setup](https://github.com/ValveSoftware/steam-runtime/blob/e014a74f60b45a861d38a867b1c81efe8484f77a/README.md#using-schroot) the chroot.

```
sudo ./setup_chroot.sh --i386 --tarball ./com.valvesoftware.SteamRuntime.Sdk-i386-scout-sysroot.tar.gz
```

### Building

Now you can use cmake and make prepending the commands with `schroot --chroot steamrt_scout_i386 --`:
```
schroot --chroot steamrt_scout_i386 -- cmake -B build-in-steamrt -S .
schroot --chroot steamrt_scout_i386 -- cmake --build build-in-steamrt
```

## Linux. Build without Steam Runtime

### Prerequisites

Install C++ compilers, cmake and x86 development libraries for C, C++ and SDL2. On Ubuntu/Debian:
```
sudo apt install cmake build-essential gcc-multilib g++-multilib libsdl2-dev:i386
```

### Building

```
cmake -B build -S .
cmake --build build
```

Note that the libraries built this way might be not compatible with Steam Half-Life. If you have such issue you can configure it to build statically with c++ and gcc libraries:
```
cmake .. -DCMAKE_C_FLAGS="-static-libstdc++ -static-libgcc"
```
To ensure portability it's still better to build using Steam Runtime or another chroot of some older distro.

## Linux. Build in your own chroot

### Prerequisites

Use the most suitable way for you to create an old distro 32-bit chroot. E.g. on Ubuntu/Debian you can use debootstrap.

```
sudo apt install debootstrap schroot
sudo mkdir -p /var/choots
sudo debootstrap --arch=i386 jessie /var/chroots/jessie-i386 # On Ubuntu type trusty instead of jessie
sudo chroot /var/chroots/jessie-i386
```

```
# inside chroot
apt install cmake build-essential gcc-multilib g++-multilib libsdl2-dev
exit
```

Create and adapt the following config in /etc/schroot/chroot.d/jessie.conf (you can choose a different name):

```
[jessie]
type=directory
description=Debian jessie i386
directory=/var/chroots/jessie-i386/
users=yourusername
groups=adm
root-groups=root
preserve-environment=true
personality=linux32
```

Insert your actual user name in place of `yourusername`.

### Building

Prepend any make or cmake call with `schroot -c jessie --`:
```
schroot --chroot jessie -- cmake -B build-in-chroot -S .
schroot --chroot jessie -- cmake --build build-in-chroot
```

## Android

TODO

## Other platforms

Building on other Unix-like platforms (e.g. FreeBSD) is supported.

### Prerequisites

Install C and C++ compilers (like gcc or clang), cmake and make (or gmake)

### Building

```
cmake -B build -S .
cmake --build build
```

### Building with waf

To use waf, you need to install python (2.7 minimum)

```
(./waf configure -T release)
(./waf)
```

## Build options

Some useful build options that can be set during the cmake step.

* **GOLDSOURCE_SUPPORT** - allows to turn off/on the support for GoldSource input. Set to **ON** by default on Windows and Linux, **OFF** on other platforms.
* **USE_VGUI** - whether to use VGUI library. **OFF** by default. You need to init `vgui_support` submodule in order to build with VGUI.

This list is incomplete. Look at `CMakeLists.txt` to see all available options.

Prepend option names with `-D` when passing to cmake. Boolean options can take values **OFF** and **ON**. Example:

```
cmake .. -DUSE_VGUI=ON -DGOLDSOURCE_SUPPORT=ON -DCROWBAR_IDLE_ANIM=ON -DCROWBAR_FIX_RAPID_CROWBAR=ON
```
