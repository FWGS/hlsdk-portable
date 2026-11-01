# CMake toolchain file for PlayStation 4 with OpenOrbis PS4 toolchain
#
# OO_PS4_TOOLCHAIN=/path/to/OpenOrbis/PS4Toolchain CC=clang-18 CXX=clang++-18 \
#     cmake -DCMAKE_TOOLCHAIN_FILE=cmake/PS4Toolchain.cmake -DCMAKE_BUILD_TYPE=Release -B build-ps4 -S .
#
# The toolchain path can be given with -DOO_PS4_TOOLCHAIN=... as well. OpenOrbis libraries are
# built with LLVM 18, so clang and lld 18 are needed: lld is taken from the directory of the compiler.

set(CMAKE_SYSTEM_NAME FreeBSD)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT OO_PS4_TOOLCHAIN)
	set(OO_PS4_TOOLCHAIN "$ENV{OO_PS4_TOOLCHAIN}")
endif()

if(NOT EXISTS "${OO_PS4_TOOLCHAIN}/link.x")
	message(FATAL_ERROR "OpenOrbis toolchain not found, set OO_PS4_TOOLCHAIN to its PS4Toolchain directory")
endif()

# configuration checks build their own projects with this file, they need the path too
set(OO_PS4_TOOLCHAIN "${OO_PS4_TOOLCHAIN}" CACHE PATH "OpenOrbis PS4Toolchain directory")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES OO_PS4_TOOLCHAIN)

set(PS4_TARGET x86_64-pc-freebsd12-elf)
set(CMAKE_C_COMPILER_TARGET ${PS4_TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${PS4_TARGET})
set(CMAKE_SYSROOT "${OO_PS4_TOOLCHAIN}")

set(CMAKE_FIND_ROOT_PATH "${OO_PS4_TOOLCHAIN}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# __ORBIS__ selects XASH_PS4 in build.h
set(CMAKE_C_FLAGS_INIT "-D__ORBIS__ -fPIC -funwind-tables -isystem ${OO_PS4_TOOLCHAIN}/include")
set(CMAKE_CXX_FLAGS_INIT "-D__ORBIS__ -fPIC -funwind-tables -isystem ${OO_PS4_TOOLCHAIN}/include/c++/v1 -isystem ${OO_PS4_TOOLCHAIN}/include")

# the clang driver takes ld.lld from PATH, which might be a newer version
# with different --wrap behaviour, so the one next to the compiler is used
if(DEFINED ENV{CC})
	string(REPLACE " " ";" PS4_CC_COMMAND "$ENV{CC}")
	list(GET PS4_CC_COMMAND 0 PS4_CC_NAME)
else()
	set(PS4_CC_NAME clang)
endif()
find_program(PS4_CC_PATH NAMES ${PS4_CC_NAME})
if(PS4_CC_PATH)
	get_filename_component(PS4_CC_PATH "${PS4_CC_PATH}" REALPATH)
	get_filename_component(PS4_LLVM_BIN "${PS4_CC_PATH}" DIRECTORY)
endif()

set(PS4_LINKER_FLAGS "-fuse-ld=lld -nostdlib")
if(PS4_LLVM_BIN AND EXISTS "${PS4_LLVM_BIN}/ld.lld")
	string(APPEND PS4_LINKER_FLAGS " --ld-path=${PS4_LLVM_BIN}/ld.lld")
endif()

# OpenOrbis images are PIE, use toolchain linker script and no ELF interpreter
string(APPEND PS4_LINKER_FLAGS " -Wl,--no-dynamic-linker -Wl,-m,elf_x86_64 -Wl,--eh-frame-hdr")
string(APPEND PS4_LINKER_FLAGS " -Wl,--script,${OO_PS4_TOOLCHAIN}/link.x -L${OO_PS4_TOOLCHAIN}/lib")

# only configuration checks are linked as programs
# (startup code needs libc, which is linked after objects otherwise)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${PS4_LINKER_FLAGS} -Wl,-pie ${OO_PS4_TOOLCHAIN}/lib/crt1.o -lc -lSceSystemService -lkernel")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${PS4_LINKER_FLAGS}")

set(CMAKE_C_STANDARD_LIBRARIES_INIT "-lc -lSceNet -lkernel")
set(CMAKE_CXX_STANDARD_LIBRARIES_INIT "-lc++ -lc -lSceNet -lkernel")
