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
		    $(LOCAL_PATH)/gearbox

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
           egon.cpp \
           explode.cpp \
           flyingmonster.cpp \
           func_break.cpp \
           func_tank.cpp \
           game.cpp \
           gamerules.cpp \
           gargantua.cpp \
           gauss.cpp \
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
           hornetgun.cpp \
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
	   observer.cpp \
           osprey.cpp \
           pathcorner.cpp \
           plane.cpp \
           plats.cpp \
           player.cpp \
	   playermonster.cpp \
           python.cpp \
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
	   gearbox/blkop_apache.cpp \
	   gearbox/blkop_osprey.cpp \
	   gearbox/cleansuit_scientist.cpp \
	   gearbox/ctf_gamerules.cpp \
	   gearbox/ctf_items.cpp \
	   gearbox/ctf_powerups.cpp \
	   gearbox/displacer.cpp \
	   gearbox/drillsergeant.cpp \
	   gearbox/eagle.cpp \
	   gearbox/fgrunt_medic.cpp \
	   gearbox/fgrunt_torch.cpp \
	   gearbox/fgrunt.cpp \
	   gearbox/func_tank_of.cpp \
	   gearbox/gearbox_effects.cpp \
	   gearbox/gearbox_subs.cpp \
	   gearbox/gearbox_triggers.cpp \
	   gearbox/gearbox_utils.cpp \
	   gearbox/generic_items.cpp \
	   gearbox/geneworm.cpp \
	   gearbox/gonome.cpp \
	   gearbox/grapple_tonguetip.cpp \
	   gearbox/grapple.cpp \
	   gearbox/houndeye_dead.cpp \
	   gearbox/islave_dead.cpp \
	   gearbox/knife.cpp \
	   gearbox/loader.cpp \
	   gearbox/m249.cpp \
	   gearbox/massn.cpp \
	   gearbox/nuclearbomb.cpp \
	   gearbox/otis.cpp \
	   gearbox/penguin.cpp \
	   gearbox/pipewrench.cpp \
	   gearbox/pitdrone.cpp \
	   gearbox/pitworm.cpp \
	   gearbox/recruit.cpp \
	   gearbox/ropes.cpp \
	   gearbox/shock.cpp \
	   gearbox/shockrifle.cpp \
	   gearbox/shockroach.cpp \
	   gearbox/skeleton.cpp \
	   gearbox/sniperrifle.cpp \
	   gearbox/spore_ammo.cpp \
	   gearbox/sporegrenade.cpp \
	   gearbox/sporelauncher.cpp \
	   gearbox/strooper.cpp \
	   gearbox/voltigore.cpp \
	   gearbox/zombie_barney.cpp \
	   gearbox/zombie_soldier.cpp \
	   ../pm_shared/pm_debug.c \
	   ../pm_shared/pm_math.c \
	   ../pm_shared/pm_shared.c
#	   ../game_shared/voice_gamemgr.cpp

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
