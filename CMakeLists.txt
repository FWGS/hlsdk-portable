#
# Copyright (c) 2016 Alibek Omarov
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 2.8.12)

if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.15.0")
    cmake_policy(SET CMP0091 NEW)
endif()

# Install custom module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

include(VSForceXPToolchain) # Force XP toolchain for Visual Studio

project (HLSDK-XASH3D)

#--------------
# USER DEFINES \
################\
option(USE_VGUI "Enable VGUI1. UNDONE" OFF)
option(USE_VGUI2 "Enable VGUI2. UNDONE" OFF)
option(USE_VOICEMGR "Enable VOICE MANAGER." OFF)
option(BUILD_CLIENT "Build client dll" ON)
option(BUILD_SERVER "Build server dll" ON)
option(GOLDSOURCE_SUPPORT "Build goldsource compatible client library" OFF)
option(64BIT "Disable auto -m32 appending to compiler flags" OFF)
set(GAMEDIR "bshift" CACHE STRING "Gamedir path")
set(SERVER_INSTALL_DIR "dlls" CACHE STRING "Where put server dll")
set(CLIENT_INSTALL_DIR "cl_dlls" CACHE STRING "Where put client dll")
set(SERVER_LIBRARY_NAME "bshift" CACHE STRING "Library name for Linux/MacOS/Windows")

#-----------------
# MAIN BUILD CODE \
###################\

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT 64BIT)
	if(MSVC)
		error("UNDONE: set 32 build flags")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS}")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
	endif()
	set(CMAKE_SIZEOF_VOID_P 4)
endif()

if(64BIT AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR "You enabled XASH_64BIT, but compiler can't create 64 bit code!")
endif()

# Xash3D FWGS Library Naming Scheme compliance
# see documentation: https://github.com/FWGS/xash3d-fwgs/blob/master/Documentation/library-naming.md
include(LibraryNaming)

if(64BIT)
	message(STATUS "Building for 64 Bit")
else()
	message(STATUS "Building for 32 Bit")
endif()

if (MINGW)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--add-stdcall-alias")
endif()

# add_compile_options for older cmake versions
if(${CMAKE_VERSION} VERSION_LESS "3.0.2")
	macro(add_compile_options)
		set(list_var "${ARGV}")
		foreach(arg IN LISTS list_var)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${arg}")
		endforeach()
	endmacro()
endif()

if(NOT MSVC)
	#add_compile_options(-Wempty-body) # GCC/Clang flag
	add_compile_options(-Wreturn-type) # GCC/Clang flag
else()
	add_definitions(-D_CRT_SILENCE_NONCONFORMING_TGMATH_H)
endif()


if(BUILD_CLIENT)
	add_subdirectory(cl_dll)
endif()

if(BUILD_SERVER)
	add_subdirectory(dlls)
endif()

if(NOT BUILD_SERVER AND NOT BUILD_CLIENT)
	error("Nothing to build")
endif()
