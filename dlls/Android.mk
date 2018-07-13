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
		    $(LOCAL_PATH)/wpn_shared \
		    $(LOCAL_PATH)/../common \
		    $(LOCAL_PATH)/../engine/common \
		    $(LOCAL_PATH)/../engine \
		    $(LOCAL_PATH)/../public \
		    $(LOCAL_PATH)/../pm_shared \
		    $(LOCAL_PATH)/../game_shared \
		    $(LOCAL_PATH)/.. \
		    $(LOCAL_PATH)/3wave \
		    $(LOCAL_PATH)/aghl \
		    $(LOCAL_PATH)/dmc

LOCAL_SRC_FILES := animating.cpp \
           animation.cpp \
           bmodels.cpp \
           buttons.cpp \
           cbase.cpp \
           client.cpp \
           combat.cpp \
           doors.cpp \
           effects.cpp \
           explode.cpp \
           func_break.cpp \
           game.cpp \
           gamerules.cpp \
           globals.cpp \
           h_ai.cpp \
           h_export.cpp \
           lights.cpp \
           maprules.cpp \
           monsters.cpp \
	   mpstubb.cpp \
           multiplay_gamerules.cpp \
           pathcorner.cpp \
           plane.cpp \
           plats.cpp \
           player.cpp \
           schedule.cpp \
           singleplay_gamerules.cpp \
           skill.cpp \
           sound.cpp \
           spectator.cpp \
           subs.cpp \
           teamplay_gamerules.cpp \
           triggers.cpp \
           util.cpp \
           weapons.cpp \
	   world.cpp \
	   3wave/flags.cpp \
	   3wave/grapple.cpp \
	   3wave/runes.cpp \
	   3wave/threewave_gamerules.cpp \
	   aghl/agarena.cpp \
	   aghl/agglobal.cpp \
	   aghl/aglms.cpp \
	   aghl/agspectator.cpp \
	   aghl/agtimer.cpp \
	   dmc/quake_gun.cpp \
	   dmc/quake_items.cpp \
	   dmc/quake_nail.cpp \
	   dmc/quake_player.cpp \
	   dmc/quake_rocket.cpp \
	   dmc/quake_weapons_all.cpp \
	   dmc/observer.cpp \
	   ../pm_shared/pm_debug.c \
	   ../pm_shared/pm_math.c \
	   ../pm_shared/pm_shared.c
#	   ../game_shared/voice_gamemgr.cpp

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
