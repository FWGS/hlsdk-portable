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

cmake_minimum_required(VERSION 2.6.0)
project (CLDLL)

set (CLDLL_LIBRARY client)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-write-strings -DLINUX -D_LINUX -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -DCLIENT_WEAPONS -DCLIENT_DLL -w")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

set (CLDLL_SOURCES 
	gearbox/hud_nightvision.cpp
	../dlls/gearbox/displacer.cpp
	../dlls/gearbox/knife.cpp
	../dlls/gearbox/eagle.cpp
	../dlls/gearbox/grapple.cpp
	../dlls/gearbox/m249.cpp
	../dlls/gearbox/penguin.cpp
	../dlls/gearbox/pipewrench.cpp
	../dlls/gearbox/shockrifle.cpp
	../dlls/gearbox/sniperrifle.cpp
	../dlls/gearbox/sporelauncher.cpp
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
#SRCS+=./inputw32.cpp
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
	input_xash3d.cpp
	scoreboard.cpp
	MOTD.cpp)
include_directories (. hl/ ../dlls ../dlls/wpn_shared ../common ../engine ../pm_shared ../game_shared ../public ../dlls/gearbox )

if(USE_VOICEMGR)
	#set(CLDLL_SOURCES
	#	${CLDLL_SOURCES}
	#	../game_shared/voice_banmgr.cpp
	#	../game_shared/voice_status.cpp)
endif()

add_library (${CLDLL_LIBRARY} SHARED ${CLDLL_SOURCES})

set_target_properties (${CLDLL_SHARED} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

