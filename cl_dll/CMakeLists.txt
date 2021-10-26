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

if (USE_VGUI)
	add_definitions(-DUSE_VGUI)
	if (USE_NOVGUI_MOTD)
		add_definitions(-DUSE_NOVGUI_MOTD)
	endif()
	if (USE_NOVGUI_SCOREBOARD)
		add_definitions(-DUSE_NOVGUI_SCOREBOARD)
	endif()
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
	view.cpp)

if (USE_VGUI)
	list(APPEND CLDLL_SOURCES
		vgui_int.cpp
		vgui_ClassMenu.cpp
		vgui_ConsolePanel.cpp
		vgui_ControlConfigPanel.cpp
		vgui_CustomObjects.cpp
		vgui_MOTDWindow.cpp
		vgui_SchemeManager.cpp
		vgui_ScorePanel.cpp
		vgui_TeamFortressViewport.cpp
		vgui_SpectatorPanel.cpp
		vgui_teammenu.cpp
		voice_status.cpp
		../game_shared/vgui_checkbutton2.cpp
		../game_shared/vgui_grid.cpp
		../game_shared/vgui_helpers.cpp
		../game_shared/vgui_listbox.cpp
		../game_shared/vgui_loadtga.cpp
		../game_shared/vgui_scrollbar2.cpp
		../game_shared/vgui_slider2.cpp
		../game_shared/voice_banmgr.cpp
		)
	if (USE_NOVGUI_MOTD)
		list(APPEND CLDLL_SOURCES MOTD.cpp)
	endif()
	if (USE_NOVGUI_SCOREBOARD)
		list(APPEND CLDLL_SOURCES scoreboard.cpp)
	endif()
else()
	list(APPEND CLDLL_SOURCES
		MOTD.cpp
		scoreboard.cpp)
endif()

include_directories (. hl/ ../dlls ../dlls/wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

if (USE_VGUI)
	SET(CMAKE_SKIP_RPATH TRUE)
	link_directories(${CMAKE_SOURCE_DIR}/vgui-dev/lib)
	include_directories(../vgui-dev/include)
else()
	include_directories(../utils/false_vgui/include)
endif()

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

if (USE_VGUI)
	if (WIN32)
		add_library(vgui SHARED IMPORTED)
		set_property(TARGET vgui PROPERTY IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/vgui-dev/lib/win32_vc6/vgui.dll")
		set_property(TARGET vgui PROPERTY IMPORTED_IMPLIB "${CMAKE_SOURCE_DIR}/vgui-dev/lib/win32_vc6/vgui.lib")
		target_link_libraries(${CLDLL_LIBRARY} vgui)
		target_link_libraries(${CLDLL_LIBRARY} ws2_32)
	elseif(APPLE)
		target_link_libraries(${CLDLL_LIBRARY} "-Wl,--no-undefined -L${CMAKE_SOURCE_DIR}/vgui-dev/lib vgui.dylib")
	else()
		target_link_libraries(${CLDLL_LIBRARY} :vgui.so)
	endif()
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

if(MSVC)
	set_property(TARGET ${CLDLL_LIBRARY} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

install( TARGETS ${CLDLL_LIBRARY}
	DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE )
