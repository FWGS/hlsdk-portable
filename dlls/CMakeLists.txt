#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Edited by Roy for HL:Invasion
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
project (SVDLL)

set (SVDLL_LIBRARY server)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

if (USE_GSTREAMER AND (${CMAKE_SYSTEM_NAME} STREQUAL "Linux"))
	add_definitions(-DUSE_GSTREAMER)
endif()

if (USE_MINIAUDIO)
	add_definitions(-DUSE_MINIAUDIO)
endif()

if (USE_FMOD AND (WIN32 OR NOT (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")))
	add_definitions(-DUSE_FMOD)
endif()

set (SVDLL_SOURCES
	aflock.cpp
	agrunt.cpp
	airtank.cpp
	animating.cpp
	animation.cpp
	apache.cpp
	barnacle.cpp
	barney.cpp
	bigmomma.cpp
	bloater.cpp
	bmodels.cpp
	briquet.cpp
	bullsquid.cpp
	buttons.cpp
	cbase.cpp
	client.cpp
	combat.cpp
	controller.cpp
	crowbar.cpp
	defaultai.cpp
	diablo.cpp
	doors.cpp
	effects.cpp
	explode.cpp
	fgrenade.cpp
	flybee.cpp
	flyingmonster.cpp
	fog.cpp
	fsniper.cpp
	func_break.cpp
	func_tank.cpp
	func_vgui.cpp
	game.cpp
	gamerules.cpp
	gargantua.cpp
	gauss.cpp
	genericmonster.cpp
	ggrenade.cpp
	globals.cpp
	gman.cpp
	h_ai.cpp
	h_battery.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	handgrenade.cpp
	hassassin.cpp
	headcrab.cpp
	healthkit.cpp
	hgrunt.cpp
	glock.cpp
	hornet.cpp
	houndeye.cpp
	ichthyosaur.cpp
	irgun.cpp
	islave.cpp
	items.cpp
	leech.cpp
	lflammes.cpp
	lights.cpp
	luciole.cpp
	m16.cpp
	maprules.cpp
	miroir.cpp
	monstermaker.cpp
	monsters.cpp
	monsters.h
	monsterstate.cpp
	mortar.cpp
	mp5.cpp
	multiplay_gamerules.cpp
	music.cpp
	nihilanth.cpp
	nodes.cpp
	osprey.cpp
	pathcorner.cpp
	plane.cpp
	plats.cpp
	player.cpp
	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_shared.c
	python.cpp
	radiomsg.cpp
	rat.cpp
	roach.cpp
	rpg.cpp
	rpggrunt.cpp
	satchel.cpp
	schedule.cpp
	scientist.cpp
	scripted.cpp
	shotgun.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sniper.cpp
	sound.cpp
	soundent.cpp
	spectator.cpp
	squadmonster.cpp
	squeakgrenade.cpp
	subs.cpp
	supergun.cpp
	t_sub.cpp
	talkmonster.cpp
	tank.cpp
	teamplay_gamerules.cpp
	tempmonster.cpp
	tentacle.cpp
	triggers.cpp
	tripmine.cpp
	turret.cpp
	util.cpp
	util.h
	weapons.cpp
	world.cpp
	xen.cpp
	zombie.cpp
)

if (USE_GSTREAMER AND (${CMAKE_SYSTEM_NAME} STREQUAL "Linux"))
	#if we're using gstreamer music
	if (NOT 64BIT)
		set(ENV{PKG_CONFIG_PATH} "/usr/lib/i386-linux-gnu/pkgconfig")
	endif()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GLIB REQUIRED glib-2.0)

	set(GSTREAMER_MINIMUM_VERSION 1.0.5)
	pkg_check_modules(GST1_TEST gstreamer-1.0)
	if ( GST1_TEST_FOUND AND NOT ${GST1_TEST_VERSION} VERSION_LESS ${GSTREAMER_MINIMUM_VERSION} )
	    pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
	    add_definitions(-DGST_API_VERSION_1=1)
	endif()
                 
	link_directories(
	  ${GSTREAMER_LIBRARY_DIRS}
	  ${GLIB_LIBRARY_DIRS}
	)
	set(GST_LIBRARIES
	  ${GSTREAMER_LIBRARIES}
	  ${GSTREAMER-APP_LIBRARIES}
	  ${GSTREAMER-AUDIO_LIBRARIES}
	  ${GSTREAMER-PBUTILS_LIBRARIES}
	  ${GSTREAMER-FFT_LIBRARIES}
	    
	  pthread
	  ${GLIB_LIBRARIES}
	  ${GLIB_GIO_LIBRARIES}
	  ${GLIB_GOBJECT_LIBRARIES}
	)
endif()

include_directories (. wpn_shared ../common ../engine ../pm_shared ../game_shared ../public ${GLIB_INCLUDE_DIRS} ${GSTREAMER_INCLUDE_DIRS})

if(MSVC)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		hl.def)
endif()

if(USE_VOICEMGR)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		../game_shared/voice_gamemgr.cpp)
else()
	add_definitions(-DNO_VOICEGAMEMGR)
endif()

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})
if (USE_GSTREAMER AND (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")) #More gstreamer linkage stuff
	target_link_libraries(${SVDLL_LIBRARY} ${GST_LIBRARIES})
endif()

set_target_properties (${SVDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")

	set_target_properties(${SVDLL_LIBRARY} PROPERTIES
		OUTPUT_NAME ${SVDLL_NAME}
		PREFIX "")
endif()

if(MSVC)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

install( TARGETS ${SVDLL_LIBRARY}
	DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)
