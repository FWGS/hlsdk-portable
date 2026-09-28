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

cmake_minimum_required(VERSION 3.9)
project (CLDLL)

set (CLDLL_LIBRARY client)

add_definitions(-DCLIENT_DLL)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-fno-rtti) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX -DLINUX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp)
	if(NOT MINGW)
		add_definitions(-D_snprintf=snprintf -D_vsnprintf=vsnprintf)
	endif()
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

if (GOLDSOURCE_SUPPORT)
	add_definitions(-DGOLDSOURCE_SUPPORT)
endif()

if (USE_STATIC_FREEVGUI)
	add_definitions(-DINTERNAL_VGUI_SUPPORT)
endif()

set (CLDLL_SOURCES
	#../dlls/crossbow.cpp
	#../dlls/crowbar.cpp
	#../dlls/egon.cpp
	#../dlls/gauss.cpp
	#../dlls/handgrenade.cpp
	#../dlls/hornetgun.cpp
	#../dlls/mp5.cpp
	#../dlls/python.cpp
	#../dlls/rpg.cpp
	#../dlls/satchel.cpp
	#../dlls/shotgun.cpp
	#../dlls/squeakgrenade.cpp
	#../dlls/tripmine.cpp
	#../dlls/glock.cpp
	../dlls/wdoor/wpn_airgun.cpp
	../dlls/wdoor/wpn_ak47.cpp
	../dlls/wdoor/wpn_crossbow.cpp
	../dlls/wdoor/wpn_crowbar.cpp
	../dlls/wdoor/wpn_darkgrenade.cpp
	../dlls/wdoor/wpn_deagle.cpp
	../dlls/wdoor/wpn_displacer.cpp
	../dlls/wdoor/wpn_doubleuzi.cpp
	../dlls/wdoor/wpn_dualdbarrel.cpp
	../dlls/wdoor/wpn_egon.cpp
	../dlls/wdoor/wpn_fist.cpp
	../dlls/wdoor/wpn_gauss.cpp
	../dlls/wdoor/wpn_glock.cpp
	../dlls/wdoor/wpn_hammer.cpp
	../dlls/wdoor/wpn_handgrenade.cpp
	../dlls/wdoor/wpn_hornetgun.cpp
	../dlls/wdoor/wpn_kmedkit.cpp
	../dlls/wdoor/wpn_minigun.cpp
	../dlls/wdoor/wpn_mp5.cpp
	../dlls/wdoor/wpn_python.cpp
	../dlls/wdoor/wpn_redeemer.cpp
	../dlls/wdoor/wpn_rpg.cpp
	../dlls/wdoor/wpn_satchel.cpp
	../dlls/wdoor/wpn_sg550.cpp
	../dlls/wdoor/wpn_shotgun.cpp
	../dlls/wdoor/wpn_smg.cpp
	../dlls/wdoor/wpn_sniper.cpp
	../dlls/wdoor/wpn_squeakgrenade.cpp
	../dlls/wdoor/wpn_tripmine.cpp
	../dlls/wdoor/wpn_valvesword.cpp
	wdoor/game_fx.cpp
	wdoor/HUD_AirBar.cpp
	wdoor/HUD_ArmorBar.cpp
	wdoor/HUD_DarkHoles.cpp
	wdoor/HUD_GameOver.cpp
	wdoor/HUD_HealthBar.cpp
	wdoor/HUD_life.cpp
	wdoor/HUD_ModeShow.cpp
	wdoor/HUD_nvg.cpp
	wdoor/HUD_PassageBar.cpp
	wdoor/HUD_PasswordBord.cpp
	wdoor/HUD_RPGMenu.cpp
	wdoor/HUD_RTP_Bar.cpp
	wdoor/HUD_Scope.cpp
	wdoor/Particle.cpp
	wdoor/ParticleSystem.cpp
	wdoor/PSBeam.cpp
	wdoor/PSBlastCone.cpp
	wdoor/PSBlood.cpp
	wdoor/PSBubbles.cpp
	wdoor/PSBubbleTrail.cpp
	wdoor/PSDrips.cpp
	wdoor/PSFlatTrail.cpp
	wdoor/PSGravityPart.cpp
	wdoor/PSSparks.cpp
	wdoor/PSSparkShower.cpp
	wdoor/PSSpawnEffect.cpp
	wdoor/PSStreaks.cpp
	wdoor/PSTrail.cpp
	wdoor/PSWallGibs.cpp
	wdoor/r_particle.cpp
	wdoor/rain.cpp
	wdoor/RenderManager.cpp
	wdoor/RenderSystem.cpp
	wdoor/RotatingSystem.cpp
	wdoor/RSBeam.cpp
	wdoor/RSBeamStar.cpp
	wdoor/RSCylinder.cpp
	wdoor/RSDisk.cpp
	wdoor/RSLight.cpp
	wdoor/RSSprite.cpp
	wdoor/RSTeleparts.cpp
	wdoor/vgui_SelectMenu.cpp
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
	interpolation.cpp
	menu.cpp
	message.cpp
	MOTD.cpp
	parsemsg.cpp
	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_shared.c
	saytext.cpp
	scoreboard.cpp
	status_icons.cpp
	statusbar.cpp
	studio_util.cpp
	StudioModelRenderer.cpp
	text_message.cpp
	train.cpp
	tri.cpp
	util.cpp
	view.cpp
	../public/safe_snprintf.c
)

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

if (HAVE_STRLCPY)
	add_definitions(-DHAVE_STRLCPY)
else()
	list(APPEND CLDLL_SOURCES
		../external/openbsd/strlcpy.c)
endif()

if (HAVE_STRLCAT)
	add_definitions(-DHAVE_STRLCAT)
else()
	list(APPEND CLDLL_SOURCES
		../external/openbsd/strlcat.c)
endif()

include_directories (. hl/ wdoor/ ../dlls ../dlls/wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

if (GOLDSOURCE_SUPPORT)
	include_directories (../external/)
endif()


include_directories(../utils/vgui/include)

add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})
target_link_libraries(${CLDLL_LIBRARY} vcs_info)

if (GOLDSOURCE_SUPPORT)
	target_link_libraries( ${CLDLL_LIBRARY} ${CMAKE_DL_LIBS} )
endif()

target_link_libraries(${CLDLL_LIBRARY} vgui)

if(WIN32)
	target_link_libraries( ${CLDLL_LIBRARY} user32.lib )
	if (GOLDSOURCE_SUPPORT)
		target_link_libraries( ${CLDLL_LIBRARY} winmm.lib )
	endif()
endif()

set_target_properties (${CLDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

set_target_postfix(${CLDLL_LIBRARY})

if(HAVE_LTO)
	set_property(TARGET ${CLDLL_LIBRARY} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

install(TARGETS ${CLDLL_LIBRARY}
	RUNTIME DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	LIBRARY DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(CMAKE_BUILD_TYPE MATCHES "Release" AND NOT WIN32)
	add_custom_command(TARGET ${CLDLL_LIBRARY}
			POST_BUILD DEPENDS ${CLDLL_LIBRARY}
			COMMAND ${CMAKE_STRIP} -s $<TARGET_FILE:${CLDLL_LIBRARY}>)
endif()
