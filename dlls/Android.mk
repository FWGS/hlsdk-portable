#HLSDK server Android port
#Copyright (c) nicknekit

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := server

include $(XASH3D_CONFIG)

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libserver_hardfp
endif

LOCAL_CFLAGS += -D_LINUX -DCLIENT_WEAPONS -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf \
	-fno-exceptions -DNO_VOICEGAMEMGR -w

LOCAL_CPPFLAGS := $(LOCAL_CFLAGS)

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
		    $(LOCAL_PATH)/. \
		    $(LOCAL_PATH)/tfc \
		    $(LOCAL_PATH)/../common \
		    $(LOCAL_PATH)/../engine/common \
		    $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../public \
		    $(LOCAL_PATH)/../pm_shared \
		    $(LOCAL_PATH)/../game_shared \
		    $(LOCAL_PATH)/../cl_dll
LOCAL_SRC_FILES := airtank.cpp \
	animating.cpp \
	animation.cpp \
	bmodels.cpp \
	buttons.cpp \
	cbase.cpp \
	client.cpp \
	combat.cpp \
	crowbar.cpp \
	defaultai.cpp \
	doors.cpp \
	effects.cpp \
	explode.cpp \
	func_break.cpp \
	func_tank.cpp \
	game.cpp \
	gamerules.cpp \
	ggrenade.cpp \
	globals.cpp \
	h_ai.cpp \
	h_battery.cpp \
	h_cine.cpp \
	h_cycler.cpp \
	h_export.cpp \
	healthkit.cpp \
	items.cpp \
	lights.cpp \
	monsters.cpp \
	monsterstate.cpp \
	mortar.cpp \
	multiplay_gamerules.cpp \
	nodes.cpp \
	observer.cpp \
	pathcorner.cpp \
	plane.cpp \
	plats.cpp \
	player.cpp \
	playermonster.cpp \
	schedule.cpp \
	scripted.cpp \
	skill.cpp \
	sound.cpp \
	soundent.cpp \
	spectator.cpp \
	squadmonster.cpp \
	subs.cpp \
	talkmonster.cpp \
	teamplay_gamerules.cpp \
	tempmonster.cpp \
	triggers.cpp \
	turret.cpp \
	util.cpp \
	weapons.cpp \
	world.cpp \
	xen.cpp \
	tfc/areadef.cpp \
	tfc/demoman.cpp \
	tfc/dispenser.cpp \
	tfc/engineer.cpp \
	tfc/pyro.cpp \
	tfc/sentry.cpp \
	tfc/spy.cpp \
	tfc/teleporter.cpp \
	tfc/tf_admin.cpp \
	tfc/tf_clan.cpp \
	tfc/tf_gamerules.cpp \
	tfc/tf_item.cpp \
	tfc/tf_sbar.cpp \
	tfc/tf_wpn_ac.cpp \
	tfc/tf_wpn_ar.cpp \
	tfc/tf_wpn_axe.cpp \
	tfc/tf_wpn_flame.cpp \
	tfc/tf_wpn_gl.cpp \
	tfc/tf_wpn_grenades.cpp \
	tfc/tf_wpn_ic.cpp \
	tfc/tf_wpn_nails.cpp \
	tfc/tf_wpn_ng.cpp \
	tfc/tf_wpn_railgun.cpp \
	tfc/tf_wpn_rpg.cpp \
	tfc/tf_wpn_sg.cpp \
	tfc/tf_wpn_srifle.cpp \
	tfc/tf_wpn_tranq.cpp \
	tfc/tfort.cpp \
	tfc/tfortmap.cpp \
	tfc/tforttm.cpp \
	   ../pm_shared/pm_debug.c \
	   ../pm_shared/pm_math.c \
	   ../pm_shared/pm_shared.c
#	   ../game_shared/voice_gamemgr.cpp

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
