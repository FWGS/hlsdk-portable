#
# Copyright (c) 2015 Pavlo Lavrenenko
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

cmake_minimum_required(VERSION 2.6.0)
project (SVDLL)

set (SVDLL_LIBRARY server)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-exceptions -w")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

file( GLOB_RECURSE SVDLL_SOURCES *.cpp ) 
file( GLOB_RECURSE PM_SOURCES ../pm_shared/*.c )
list( APPEND SVDLL_SOURCES ${PM_SOURCES} )

set( REMOVE_UNNEEDED_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/AI_BaseNPC_Schedule.cpp
	${CMAKE_CURRENT_LIST_DIR}/mpstubb.cpp )
list( REMOVE_ITEM SVDLL_SOURCES ${REMOVE_UNNEEDED_SOURCES} )
	
include_directories (. wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

add_definitions( -D_LINUX -DCLIENT_WEAPONS -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf  )

if(USE_VOICEMGR)
	list( APPEND SVDLL_SOURCES ../game_shared/voice_gamemgr.cpp)
else()
	add_definitions(-DNO_VOICEGAMEMGR)
endif()

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})

set_target_properties (${SVDLL_SHARED} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

