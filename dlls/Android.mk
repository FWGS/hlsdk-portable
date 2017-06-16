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
		    $(LOCAL_PATH)/hs \
		    $(LOCAL_PATH)/. \
		    $(LOCAL_PATH)/wpn_shared \
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
           gargantua.cpp \
           genericmonster.cpp \
           ggrenade.cpp \
           globals.cpp \
           glock.cpp \
           gman.cpp \
           h_ai.cpp \
           h_battery.cpp \
           h_cycler.cpp \
           h_cine.cpp \
           h_export.cpp \
           handgrenade.cpp \
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
           monstermaker.cpp \
           monsters.cpp \
           monsterstate.cpp \
           mortar.cpp \
           mp5.cpp \
           multiplay_gamerules.cpp \
           nihilanth.cpp \
           nodes.cpp \
           osprey.cpp \
           pathcorner.cpp \
           plane.cpp \
           plats.cpp \
           player.cpp \
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
	   hs/ak47.cpp \
           hs/beamkatana.cpp \
           hs/boombox.cpp \
           hs/bot_combat.cpp \
           hs/bot.cpp \
           hs/botcam.cpp \
           hs/bow.cpp \
           hs/chrischan.cpp \
           hs/cod_gamerules.cpp \
           hs/coop_gamerules.cpp \
           hs/creeper.cpp \
           hs/dosh.cpp \
           hs/fotns.cpp \
           hs/gayglenn.cpp \
           hs/goldengun.cpp \
           hs/grinman.cpp \
           hs/heavyrain_gamerules.cpp \
           hs/homestuck.cpp \
           hs/hs_model.cpp \
           hs/jackal.cpp \
           hs/jason.cpp \
           hs/jihad.cpp \
           hs/modman.cpp \
           hs/monhunt_gamerules.cpp \
           hs/mw2.cpp \
           hs/nstar.cpp \
           hs/scipg.cpp \
           hs/shytplay_gamerules.cpp \
           hs/sinistar.cpp \
           hs/soda_can.cpp \
           hs/test_gamerules.cpp \
           hs/xmast.cpp \
           hs/zapper.cpp \
	   ../pm_shared/pm_debug.c \
	   ../pm_shared/pm_math.c \
	   ../pm_shared/pm_shared.c
#	   ../game_shared/voice_gamemgr.cpp

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
