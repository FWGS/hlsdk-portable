# SoHL android makefile
# Copyright (c) mittorn



LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := server

include $(XASH3D_CONFIG)

LOCAL_CFLAGS += -D_LINUX -DCLIENT_WEAPONS \
	-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf \
	-Wno-write-strings -Wno-invalid-offsetof -Wno-conversion-null -DNO_VOICEGAMEMGR

LOCAL_C_INCLUDES := $(LOCAL_PATH)/. \
		    $(LOCAL_PATH)/../common \
		    $(LOCAL_PATH)/../engine/common \
		    $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../public \
		    $(LOCAL_PATH)/../pm_shared \
		    $(LOCAL_PATH)/../game_shared

LOCAL_SRC_FILES := aflock.cpp agrunt.cpp schedule.cpp airtank.cpp \
		alias.cpp animating.cpp animation.cpp apache.cpp barnacle.cpp barney.cpp \
		bigmomma.cpp bloater.cpp bmodels.cpp bullsquid.cpp buttons.cpp cbase.cpp \
		client.cpp combat.cpp controller.cpp crossbow.cpp crowbar.cpp defaultai.cpp \
		doors.cpp effects.cpp egon.cpp explode.cpp flyingmonster.cpp func_break.cpp \
		func_tank.cpp game.cpp gamerules.cpp gargantua.cpp gauss.cpp genericmonster.cpp \
		ggrenade.cpp globals.cpp gman.cpp h_ai.cpp handgrenade.cpp hassassin.cpp \
		h_battery.cpp h_cine.cpp h_cycler.cpp headcrab.cpp healthkit.cpp h_export.cpp \
		hgrunt.cpp hornet.cpp hornetgun.cpp houndeye.cpp ichthyosaur.cpp islave_deamon.cpp \
		items.cpp leech.cpp lights.cpp locus.cpp maprules.cpp monstermaker.cpp monsters.cpp \
		monsterstate.cpp mortar.cpp movewith.cpp mp5.cpp multiplay_gamerules.cpp nihilanth.cpp \
		nodes.cpp observer.cpp osprey.cpp pathcorner.cpp plane.cpp plats.cpp player.cpp playermonster.cpp \
		python.cpp rat.cpp roach.cpp rpg.cpp satchel.cpp scientist.cpp scripted.cpp shotgun.cpp \
		singleplay_gamerules.cpp skill.cpp sound.cpp soundent.cpp spectator.cpp squadmonster.cpp \
		squeakgrenade.cpp stats.cpp subs.cpp talkmonster.cpp teamplay_gamerules.cpp tempmonster.cpp \
		tentacle.cpp triggers.cpp tripmine.cpp turret.cpp util.cpp weapons.cpp world.cpp xen.cpp \
		zombie.cpp glock.cpp \
	   ../pm_shared/pm_debug.c ../pm_shared/pm_math.c ../pm_shared/pm_shared.c



ifeq ($(TARGET_ARCH_ABI),armeabi-v7a-hard)
LOCAL_MODULE_FILENAME = libserver_hardfp
endif

LOCAL_CPPFLAGS := $(LOCAL_CFLAGS)

include $(BUILD_SHARED_LIBRARY)
