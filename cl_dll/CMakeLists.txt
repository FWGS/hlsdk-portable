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
project (CLDLL)

set (CLDLL_LIBRARY client)

include(CheckIncludeFile)
check_include_file("tgmath.h" HAVE_TGMATH_H)
if(HAVE_TGMATH_H)
	add_definitions(-DHAVE_TGMATH_H)
endif()

add_definitions(-DCLIENT_WEAPONS -DCLIENT_DLL)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-Wno-write-strings) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX -DLINUX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp)
	if(NOT MINGW)
		add_definitions(-D_snprintf=snprintf -D_vsnprintf=vsnprintf)
	endif()
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if (GOLDSOURCE_SUPPORT)
	add_definitions(-DGOLDSOURCE_SUPPORT)
endif()


set (CLDLL_SOURCES
	../dlls/crossbow.cpp
	../dlls/crowbar.cpp
	../dlls/egon.cpp
	../dlls/gauss.cpp
	../dlls/handgrenade.cpp
	../dlls/hornetgun.cpp
	../dlls/mp5.cpp
	../dlls/python.cpp
	../dlls/rpg.cpp
	../dlls/satchel.cpp
	../dlls/shotgun.cpp
	../dlls/squeakgrenade.cpp
	../dlls/tripmine.cpp
	../dlls/glock.cpp
	ev_hldm.cpp
	hl/hl_baseentity.cpp
	hl/hl_events.cpp
	hl/hl_objects.cpp
	hl/hl_weapons.cpp
	ammo.cpp
	ammo_secondary.cpp
	ammohistory.cpp
	battery.cpp
	cdll_int.cpp
	com_weapons.cpp
	death.cpp
	demo.cpp
	entity.cpp
	ev_common.cpp
	events.cpp
	flashlight.cpp
	GameStudioModelRenderer.cpp
	geiger.cpp
	health.cpp
	hud.cpp
	hud_msg.cpp
	hud_redraw.cpp
	hud_spectator.cpp
	hud_update.cpp
	in_camera.cpp
	input.cpp
	input_goldsource.cpp
	input_mouse.cpp
	input_xash3d.cpp
	menu.cpp
	message.cpp
	overview.cpp
	parsemsg.cpp
	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_shared.c
	saytext.cpp
	status_icons.cpp
	statusbar.cpp
	studio_util.cpp
	StudioModelRenderer.cpp
	text_message.cpp
	train.cpp
	tri.cpp
	util.cpp
	view.cpp
	scoreboard.cpp
	MOTD.cpp)

include_directories (. hl/ ../dlls ../dlls/wpn_shared ../common ../engine ../pm_shared ../game_shared ../public ../utils/false_vgui/include)

if(USE_VOICEMGR)
	#set(CLDLL_SOURCES
	#	${CLDLL_SOURCES}
	#	../game_shared/voice_banmgr.cpp
	#	../game_shared/voice_status.cpp)
endif()

add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})
if (GOLDSOURCE_SUPPORT)
	target_link_libraries( ${CLDLL_LIBRARY} ${CMAKE_DL_LIBS} )
endif()

if(WIN32)
	target_link_libraries( ${CLDLL_LIBRARY} user32.lib )
	if (GOLDSOURCE_SUPPORT)
		target_link_libraries( ${CLDLL_LIBRARY} winmm.lib )
	endif()
endif()

set_target_properties (${CLDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

if(APPLE OR WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set(CLDLL_NAME "client")
	set_target_properties(${CLDLL_LIBRARY} PROPERTIES
		OUTPUT_NAME ${CLDLL_NAME}
		PREFIX "")
endif()

install( TARGETS ${CLDLL_LIBRARY}
	DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE )
