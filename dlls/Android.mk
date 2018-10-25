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
	-fno-exceptions -DNO_VOICEGAMEMGR -DCROWBAR_IDLE_ANIM -w

LOCAL_CPPFLAGS := $(LOCAL_CFLAGS)

LOCAL_C_INCLUDES := $(SDL_PATH)/include \
		    $(LOCAL_PATH)/. \
		    $(LOCAL_PATH)/wpn_shared \
		    $(LOCAL_PATH)/poke646 \
		    $(LOCAL_PATH)/../common \
		    $(LOCAL_PATH)/../engine/common \
		    $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../public \
		    $(LOCAL_PATH)/../pm_shared \
		    $(LOCAL_PATH)/../game_shared

LOCAL_SRC_FILES := agrunt.cpp airtank.cpp \
           aflock.cpp \
           animating.cpp \
           animation.cpp \
           apache.cpp \
           barnacle.cpp \
           barney.cpp \
           bigmomma.cpp \
           bloater.cpp \
           bmodels.cpp \
           bullsquid.cpp \
           buttons.cpp \
           cbase.cpp \
           client.cpp \
           combat.cpp \
           controller.cpp \
           crossbow.cpp \
           crowbar.cpp \
           defaultai.cpp \
           doors.cpp \
           effects.cpp \
           explode.cpp \
           flyingmonster.cpp \
           func_break.cpp \
           func_tank.cpp \
           game.cpp \
           gamerules.cpp \
	   gauss.cpp \
           gargantua.cpp \
           genericmonster.cpp \
           ggrenade.cpp \
	   glock.cpp \
           globals.cpp \
           gman.cpp \
           h_ai.cpp \
           h_battery.cpp \
           h_cycler.cpp \
           h_cine.cpp \
           h_export.cpp \
           hassassin.cpp \
           headcrab.cpp \
           healthkit.cpp \
           hgrunt.cpp \
           hornet.cpp \
           houndeye.cpp \
           ichthyosaur.cpp \
           islave.cpp \
           items.cpp \
           leech.cpp \
           lights.cpp \
           maprules.cpp \
	   mp5.cpp \
           monstermaker.cpp \
           monsters.cpp \
           monsterstate.cpp \
           mortar.cpp \
           multiplay_gamerules.cpp \
           nihilanth.cpp \
           nodes.cpp \
	   observer.cpp \
           osprey.cpp \
           pathcorner.cpp \
           plane.cpp \
           plats.cpp \
           player.cpp \
	   playermonster.cpp \
           rat.cpp \
           roach.cpp \
           rpg.cpp \
           satchel.cpp \
           schedule.cpp \
           scientist.cpp \
           scripted.cpp \
           shotgun.cpp \
           singleplay_gamerules.cpp \
           skill.cpp \
           sound.cpp \
           soundent.cpp \
           spectator.cpp \
           squadmonster.cpp \
           squeakgrenade.cpp \
           subs.cpp \
           talkmonster.cpp \
           teamplay_gamerules.cpp \
           tentacle.cpp \
           tempmonster.cpp \
           triggers.cpp \
           tripmine.cpp \
           turret.cpp \
           util.cpp \
           weapons.cpp \
           world.cpp \
           xen.cpp \
           zombie.cpp \
	   ../pm_shared/pm_debug.c \
	   ../pm_shared/pm_math.c \
	   ../pm_shared/pm_shared.c \
	   poke646/firetrail.cpp \
	   poke646/genericmodel.cpp \
	   poke646/mp3.cpp \
	   poke646/nail.cpp \
	   poke646/robocop.cpp \
	   poke646/xsbeam.cpp
#	   ../game_shared/voice_gamemgr.cpp

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
