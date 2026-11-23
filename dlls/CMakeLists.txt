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

cmake_minimum_required(VERSION 3.9)
project (SVDLL)

set (SVDLL_LIBRARY server)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-fno-rtti) # GCC/Clang flag
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
	if(CMAKE_BUILD_TYPE MATCHES "Release"
		OR (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo"
		AND CMAKE_SYSTEM_NAME STREQUAL "Android"))
		add_compile_options(-fno-unwind-tables -fno-asynchronous-unwind-tables) # GCC/Clang flag
		add_compile_options(-fomit-frame-pointer) # GCC/Clang flag
		add_compile_options(-ftree-vectorize) # GCC/Clang flag
		add_compile_options(-funsafe-math-optimizations) # GCC/Clang flag
	endif()
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

set (SVDLL_SOURCES
	aflock.cpp
	ammo.cpp
	animating.cpp
	animation.cpp
	apache.cpp
	bmodels.cpp
	buttons.cpp
	cbase.cpp
	client.cpp
	combat.cpp
	counterterrorist.cpp
	defaultai.cpp
	doors.cpp
	effects.cpp
	explode.cpp
	flyingmonster.cpp
	func_break.cpp
	func_tank.cpp
	game.cpp
	gamerules.cpp
	genericitem.cpp
	genericmonster.cpp
	ggrenade.cpp
	globals.cpp
	h_ai.cpp
	h_battery.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	healthkit.cpp
	hgrunt.cpp
	hintmessage.cpp
	hostage.cpp
	hostage_localnav.cpp
	items.cpp
	lights.cpp
	maprules.cpp
	monstermaker.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	multiplay_gamerules.cpp
	nodes.cpp
	particles.cpp
	pathcorner.cpp
	plane.cpp
	plats.cpp
	player.cpp
	rat.cpp
	roach.cpp
	schedule.cpp
	scientist.cpp
	scripted.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sound.cpp
	soundent.cpp
	spectator.cpp
	squadmonster.cpp
	subs.cpp
	talkmonster.cpp
	teamplay_gamerules.cpp
	terrorist.cpp
	triggers.cpp
	tripmine.cpp
	turret.cpp
	util.cpp
	weapons.cpp
	world.cpp
	wpn_ak47.cpp
	wpn_aug.cpp
	wpn_awp.cpp
	wpn_blowtorch.cpp
	wpn_briefcase.cpp
	wpn_c4.cpp
	wpn_camera.cpp
	wpn_deagle.cpp
	wpn_elite.cpp
	wpn_famas.cpp
	wpn_fiberopticcamera.cpp
	wpn_fiveseven.cpp
	wpn_flashbang.cpp
	wpn_g3sg1.cpp
	wpn_galil.cpp
	wpn_glock18.cpp
	wpn_hegrenade.cpp
	wpn_holster.cpp
	wpn_knife.cpp
	wpn_laws.cpp
	wpn_m249.cpp
	wpn_m3.cpp
	wpn_m4a1.cpp
	wpn_m60.cpp
	wpn_mac10.cpp
	wpn_medkit.cpp
	wpn_mp5navy.cpp
	wpn_p228.cpp
	wpn_p90.cpp
	wpn_radio.cpp
	wpn_radiocontrolledbomb.cpp
	wpn_scout.cpp
	wpn_sg550.cpp
	wpn_sg552.cpp
	wpn_shield.cpp
	wpn_smokegrenade.cpp
	wpn_syringe.cpp
	wpn_tmp.cpp
	wpn_ump45.cpp
	wpn_usp.cpp
	wpn_xm1014.cpp
	wpn_zipline.cpp
	czeror/cz_music.cpp
	czeror/cz_seqapi.cpp
	czeror/cz_sequence.cpp
	../game_shared/voice_gamemgr.cpp
	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_shared.c
	../public/safe_snprintf.c
)

if (HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcpy.c)
endif()

if (HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND SVDLL_SOURCES
		../external/openbsd/strlcat.c)
endif()

include_directories (. wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

if(MSVC)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		hl.def)
endif()

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})
target_link_libraries(${SVDLL_LIBRARY} vcs_info)

set_target_properties (${SVDLL_LIBRARY} PROPERTIES POSITION_INDEPENDENT_CODE 1)

if(APPLE AND NOT "${SERVER_LIBRARY_NAME_OSX}" STREQUAL "")
	# on Apple platforms we always use gamedll_osx liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_OSX}")
elseif(NOT WIN32 AND NOT "${SERVER_LIBRARY_NAME_LINUX}" STREQUAL "")
	# on non-Windows platforms we use gamedll_linux liblist/gameinfo key, if it's set
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME_LINUX}")
else()
	# on Windows or if no gamedll_osx or gamedll_linux is set, use default gamedll key
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")
endif()

set_target_postfix_with_name(${SVDLL_LIBRARY} ${SVDLL_NAME})

if(HAVE_LTO)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if(XASH_PS4)
	# signed module and names of its functions, see cmake/PS4.cmake
	ps4_add_module(${SVDLL_LIBRARY} "${GAMEDIR}/${SERVER_INSTALL_DIR}/")
else()
	install(TARGETS ${SVDLL_LIBRARY}
		RUNTIME DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
		LIBRARY DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
			GROUP_READ GROUP_EXECUTE
			WORLD_READ WORLD_EXECUTE)
endif()

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32 AND NOT XASH_PS4)
	if(APPLE)
		# Apple strip treats -s as a symbol list file, -x keeps exported symbols
		set(STRIP_FLAGS -x)
	else()
		set(STRIP_FLAGS -s)
	endif()
	add_custom_command(TARGET ${SVDLL_LIBRARY}
			POST_BUILD DEPENDS ${SVDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} ${STRIP_FLAGS} $<TARGET_FILE:${SVDLL_LIBRARY}>)
endif()
