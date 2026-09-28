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
	wdoor/BIOHOMO_aniki.cpp
	wdoor/cshl623_ent.cpp
	wdoor/main_cg_event.cpp
	wdoor/main_cg_event2.cpp
	wdoor/main_cg_event3.cpp
	wdoor/main_cg_event4.cpp
	wdoor/main_cg_event5.cpp
	wdoor/main_cg_event6.cpp
	wdoor/main_cg_event7.cpp
	wdoor/main_cg_event8.cpp
	wdoor/msg_fx.cpp
	wdoor/NPC_agrunt.cpp
	wdoor/NPC_andylow.cpp
	wdoor/NPC_apache.cpp
	wdoor/NPC_barnacle.cpp
	wdoor/NPC_barnacle_demon_boss.cpp
	wdoor/NPC_barnacle_holy.cpp
	wdoor/NPC_barney.cpp
	wdoor/NPC_barney_tr.cpp
	wdoor/NPC_bigmomma.cpp
	wdoor/NPC_bloodsucker.cpp
	wdoor/NPC_blues.cpp
	wdoor/NPC_bryan.cpp
	wdoor/NPC_bullsquid.cpp
	wdoor/NPC_chainsaw_boss.cpp
	wdoor/NPC_cof_ms0.cpp
	wdoor/NPC_cof_ms1.cpp
	wdoor/NPC_cof_ms2.cpp
	wdoor/NPC_cof_ms3.cpp
	wdoor/NPC_cof_ms4.cpp
	wdoor/NPC_cof_ms5.cpp
	wdoor/NPC_cof_ms6.cpp
	wdoor/NPC_cof_ms7.cpp
	wdoor/NPC_cof_ms8.cpp
	wdoor/NPC_cof_ms9.cpp
	wdoor/NPC_cof_ms10.cpp
	wdoor/NPC_controller.cpp
	wdoor/NPC_crasher_boss.cpp
	wdoor/NPC_cthonian.cpp
	wdoor/NPC_dengor.cpp
	wdoor/NPC_devil_wing.cpp
	wdoor/NPC_dman.cpp
	wdoor/NPC_doma.cpp
	wdoor/NPC_doma_boss.cpp
	wdoor/NPC_doraemon_boss.cpp
	wdoor/NPC_dragon.cpp
	wdoor/NPC_empty_zombie.cpp
	wdoor/NPC_fassn.cpp
	wdoor/NPC_freeman.cpp
	wdoor/NPC_gargantua.cpp
	wdoor/NPC_giant.cpp
	wdoor/NPC_gman.cpp
	wdoor/NPC_gman_boss.cpp
	wdoor/NPC_god623_boss.cpp
	wdoor/NPC_gonome.cpp
	wdoor/NPC_gordon.cpp
	wdoor/NPC_hassassin.cpp
	wdoor/NPC_hassault.cpp
	wdoor/NPC_headcrab.cpp
	wdoor/NPC_hgrunt.cpp
	wdoor/NPC_hgrunt_ally.cpp
	wdoor/NPC_hime.cpp
	wdoor/NPC_hornet.cpp
	wdoor/NPC_houndeye.cpp
	wdoor/NPC_hrhr.cpp
	wdoor/NPC_hrhr_baby.cpp
	wdoor/NPC_hunt_sworder.cpp
	wdoor/NPC_hydra_boss.cpp
	wdoor/NPC_ichthyosaur.cpp
	wdoor/NPC_islave.cpp
	wdoor/NPC_islave_boss.cpp
	wdoor/NPC_jojo.cpp
	wdoor/NPC_kadoma.cpp
	wdoor/NPC_key.cpp
	wdoor/NPC_laoba.cpp
	wdoor/NPC_leech.cpp
	wdoor/NPC_lelite.cpp
	wdoor/NPC_luigi.cpp
	wdoor/NPC_majo.cpp
	wdoor/NPC_mario.cpp
	wdoor/NPC_misaliya.cpp
	wdoor/NPC_nirvana_death.cpp
	wdoor/NPC_nobita.cpp
	wdoor/NPC_nurse.cpp
	wdoor/NPC_otis.cpp
	wdoor/NPC_police.cpp
	wdoor/NPC_purple_guy.cpp
	wdoor/NPC_revenant.cpp
	wdoor/NPC_saintna.cpp
	wdoor/NPC_scientist.cpp
	wdoor/NPC_scihead_boss.cpp
	wdoor/NPC_sewblade.cpp
	wdoor/NPC_shadow.cpp
	wdoor/NPC_sicker_623.cpp
	wdoor/NPC_sicker_dog.cpp
	wdoor/NPC_skull_bear.cpp
	wdoor/NPC_slime.cpp
	wdoor/NPC_snake_women.cpp
	wdoor/NPC_stonedevil.cpp
	wdoor/NPC_stukabat.cpp
	wdoor/NPC_takeboy.cpp
	wdoor/NPC_tank.cpp
	wdoor/NPC_tentacle.cpp
	wdoor/NPC_thrower.cpp
	wdoor/NPC_turret.cpp
	wdoor/NPC_tyant_boss.cpp
	wdoor/NPC_unknow_melon.cpp
	wdoor/NPC_unknow_ms1.cpp
	wdoor/NPC_unknow_ms2.cpp
	wdoor/NPC_unknow_ms3.cpp
	wdoor/NPC_unknow_ms4.cpp
	wdoor/NPC_vanlve.cpp
	wdoor/NPC_willam.cpp
	wdoor/NPC_wisebeast.cpp
	wdoor/NPC_worker.cpp
	wdoor/NPC_zdeadeye.cpp
	wdoor/NPC_zombie.cpp
	wdoor/NPC_zombie_barney.cpp
	wdoor/NPC_zombie_biohazard.cpp
	wdoor/NPC_zombie_nurse.cpp
	wdoor/NPC_zombie_soldier.cpp
	wdoor/wpn_airgun.cpp
	wdoor/wpn_ak47.cpp
	wdoor/wpn_axe.cpp
	wdoor/wpn_crossbow.cpp
	wdoor/wpn_crowbar.cpp
	wdoor/wpn_darkgrenade.cpp
	wdoor/wpn_deagle.cpp
	wdoor/wpn_displacer.cpp
	wdoor/wpn_doubleuzi.cpp
	wdoor/wpn_dualdbarrel.cpp
	wdoor/wpn_egon.cpp
	wdoor/wpn_fist.cpp
	wdoor/wpn_gauss.cpp
	wdoor/wpn_glock.cpp
	wdoor/wpn_hammer.cpp
	wdoor/wpn_handgrenade.cpp
	wdoor/wpn_hornetgun.cpp
	wdoor/wpn_kmedkit.cpp
	wdoor/wpn_minigun.cpp
	wdoor/wpn_mp5.cpp
	wdoor/wpn_python.cpp
	wdoor/wpn_redeemer.cpp
	wdoor/wpn_rpg.cpp
	wdoor/wpn_satchel.cpp
	wdoor/wpn_sg550.cpp
	wdoor/wpn_shotgun.cpp
	wdoor/wpn_smg.cpp
	wdoor/wpn_sniper.cpp
	wdoor/wpn_squeakgrenade.cpp
	wdoor/wpn_tripmine.cpp
	wdoor/wpn_valvesword.cpp
	airtank.cpp
	aflock.cpp
	animating.cpp
	animation.cpp
	#apache.cpp
	#barnacle.cpp
	#barney.cpp
	#bigmomma.cpp
	bloater.cpp
	bmodels.cpp
	#bullsquid.cpp
	buttons.cpp
	cbase.cpp
	client.cpp
	combat.cpp
	#controller.cpp
	#crossbow.cpp
	#crowbar.cpp
	defaultai.cpp
	doors.cpp
	effects.cpp
	#egon.cpp
	explode.cpp
	flyingmonster.cpp
	func_break.cpp
	func_tank.cpp
	game.cpp
	gamerules.cpp
	#gargantua.cpp
	#gauss.cpp
	genericmonster.cpp
	ggrenade.cpp
	globals.cpp
	#glock.cpp
	#gman.cpp
	h_ai.cpp
	h_battery.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	#handgrenade.cpp
	#hassassin.cpp
	#headcrab.cpp
	healthkit.cpp
	#hgrunt.cpp
	#hornet.cpp
	#hornetgun.cpp
	#houndeye.cpp
	#ichthyosaur.cpp
	#islave.cpp
	items.cpp
	#leech.cpp
	lights.cpp
	maprules.cpp
	monstermaker.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	#mp5.cpp
	multiplay_gamerules.cpp
	nihilanth.cpp
	nodes.cpp
	observer.cpp
	osprey.cpp
	pathcorner.cpp
	plane.cpp
	plats.cpp
	player.cpp
	playermonster.cpp
	#python.cpp
	rat.cpp
	roach.cpp
	#rpg.cpp
	#satchel.cpp
	schedule.cpp
	#scientist.cpp
	scripted.cpp
	#shotgun.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sound.cpp
	soundent.cpp
	spectator.cpp
	squadmonster.cpp
	#squeakgrenade.cpp
	subs.cpp
	talkmonster.cpp
	teamplay_gamerules.cpp
	tempmonster.cpp
	#tentacle.cpp
	triggers.cpp
	#tripmine.cpp
	#turret.cpp
	util.cpp
	vehicle.cpp
	weapons.cpp
	world.cpp
	xen.cpp
	#zombie.cpp
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

install(TARGETS ${SVDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32)
	add_custom_command(TARGET ${SVDLL_LIBRARY}
			POST_BUILD DEPENDS ${SVDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${SVDLL_LIBRARY}>)
endif()
