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

if(NOT ${CMAKE_VERSION} VERSION_LESS "3.15.0")
	cmake_policy(SET CMP0091 NEW)
endif()

# Install custom module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(VSForceXPToolchain) # Force XP toolchain for Visual Studio

project (HLSDK-PORTABLE)

#--------------
# USER DEFINES \
################\
option(USE_VGUI "Enable VGUI1." OFF)
option(USE_VGUI2 "Enable VGUI2. UNDONE" OFF)
option(USE_NOVGUI_MOTD "Prefer non-VGUI MOTD when USE_VGUI is enabled" OFF)
option(USE_NOVGUI_SCOREBOARD "Prefer non-VGUI Scoreboard when USE_VGUI is enabled" OFF)
option(USE_VOICEMGR "Enable VOICE MANAGER." OFF)
option(BUILD_CLIENT "Build client dll" ON)
option(BUILD_SERVER "Build server dll" ON)

if (CMAKE_SIZEOF_VOID_P EQUAL 4 OR
	((WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	AND (CMAKE_SYSTEM_PROCESSOR STREQUAL "x64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"
	OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64")))
	option(64BIT "Disable auto -m32 appending to compiler flags" OFF)
	option(GOLDSOURCE_SUPPORT "Build goldsource compatible client library" ON)
else()
	option(64BIT "Disable auto -m32 appending to compiler flags" ON)
	option(GOLDSOURCE_SUPPORT "Build goldsource compatible client library" OFF)
endif()

option(BARNACLE_FIX_VISIBILITY "Enable barnacle tongue length fix" OFF)
option(CLIENT_WEAPONS "Enable client local weapons prediction" ON)
option(CROWBAR_IDLE_ANIM "Enable crowbar idle animation" OFF)
option(CROWBAR_DELAY_FIX "Enable crowbar attack delay fix" OFF)
option(CROWBAR_FIX_RAPID_CROWBAR "Enable rapid crowbar fix" OFF)
option(GAUSS_OVERCHARGE_FIX "Enable gauss overcharge fix" OFF)
option(TRIPMINE_BEAM_DUPLICATION_FIX "Enable fix of tripmine beam duplication on level transition" OFF)
option(HANDGRENADE_DEPLOY_FIX "Enable handgrenade deploy animation fix after finishing a throw" OFF)
option(WEAPONS_ANIMATION_TIMES_FIX "Enable animation times fix for some weapons" OFF)
option(OEM_BUILD "Enable OEM Build" OFF)
option(HLDEMO_BUILD "Enable Demo Build" OFF)

set(GAMEDIR "valve" CACHE STRING "Gamedir path")
set(SERVER_INSTALL_DIR "dlls" CACHE STRING "Where put server dll")
set(CLIENT_INSTALL_DIR "cl_dlls" CACHE STRING "Where put client dll")
set(SERVER_LIBRARY_NAME "hl" CACHE STRING "Library name for PC platforms")
message(STATUS "Half-Life")

#-----------------
# MAIN BUILD CODE \
###################\

if(HLDEMO_BUILD AND OEM_BUILD)
	message(FATAL_ERROR "Don't mix Demo and OEM builds!")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT 64BIT)
	if(MSVC)
		error("UNDONE: set 32 build flags")
	else()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
	endif()
	set(CMAKE_SIZEOF_VOID_P 4)
endif()

if(64BIT AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(FATAL_ERROR "You enabled XASH_64BIT, but compiler can't create 64 bit code!")
endif()

# Xash3D FWGS Library Naming Scheme compliance
# see documentation: https://github.com/FWGS/xash3d-fwgs/blob/master/Documentation/extensions/library-naming.md
include(LibraryNaming)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "Building for 64 Bit")
else()
	message(STATUS "Building for 32 Bit")
endif()

if(CLIENT_WEAPONS)
	message(STATUS "Client weapons enabled.")
	add_definitions(-DCLIENT_WEAPONS)
endif()

if(BARNACLE_FIX_VISIBILITY)
	message(STATUS "Barnacle tongue fix enabled")
	add_definitions(-DBARNACLE_FIX_VISIBILITY)
endif()

if(CROWBAR_IDLE_ANIM)
	message(STATUS "Crowbar idle animation enabled")
	add_definitions(-DCROWBAR_IDLE_ANIM)
endif()

if(CROWBAR_DELAY_FIX)
	message(STATUS "Crowbar attack delay fix enabled")
	add_definitions(-DCROWBAR_DELAY_FIX)
endif()

if(CROWBAR_FIX_RAPID_CROWBAR)
	message(STATUS "Rapid crowbar fix enabled")
	add_definitions(-DCROWBAR_FIX_RAPID_CROWBAR)
endif()

if(GAUSS_OVERCHARGE_FIX)
	message(STATUS "Gauss overcharge fix enabled")
	add_definitions(-DGAUSS_OVERCHARGE_FIX)
endif()

if(TRIPMINE_BEAM_DUPLICATION_FIX)
	message(STATUS "Tripmine beam duplication fix enabled")
	add_definitions(-DTRIPMINE_BEAM_DUPLICATION_FIX)
endif()

if(HANDGRENADE_DEPLOY_FIX)
	message(STATUS "Handgrenade deploy animation fix enabled")
	add_definitions(-DHANDGRENADE_DEPLOY_FIX)
endif()

if(WEAPONS_ANIMATION_TIMES_FIX)
	message(STATUS "Weapons animation times fix enabled")
	add_definitions(-DWEAPONS_ANIMATION_TIMES_FIX)
endif()

if(OEM_BUILD)
	message(STATUS "OEM build enabled")
	add_definitions(-DOEM_BUILD)
endif()

if(HLDEMO_BUILD)
	message(STATUS "Demo build enabled")
	add_definitions(-DHLDEMO_BUILD)
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

if(VITA)
	add_compile_options(-fno-use-cxa-atexit)
endif()

check_include_file("tgmath.h" HAVE_TGMATH_H)
if(HAVE_TGMATH_H)
	if(NOT MSVC)
		set(CMAKE_REQUIRED_LIBRARIES "m")
	endif()
	check_c_source_compiles("#include <tgmath.h>
	const float val = 2, val2 = 3;
	int main(){ return (int)(-asin(val) + cos(val2)); }" HAVE_VALID_TGMATH_H )
	if(${HAVE_VALID_TGMATH_H})
		add_definitions(-DHAVE_TGMATH_H)
	endif()
endif()

if(BUILD_CLIENT)
	message(STATUS "Building client enabled")
	add_subdirectory(cl_dll)
endif()

if(BUILD_SERVER)
	message(STATUS "Building server enabled")
	add_subdirectory(dlls)
endif()

if(NOT BUILD_SERVER AND NOT BUILD_CLIENT)
	message(FATAL_ERROR "Nothing to build")
endif()
