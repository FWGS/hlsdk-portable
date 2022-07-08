#HLSDK server Android port
#Copyright (c) nicknekit
#Edited by Roy for HL:Invasion
#Warning: Completely UNTESTED!
#Warning: Won't work properly as this mod requires TRUE VGUI! (If you can edit it in - you're more than welcome!)

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
		    $(LOCAL_PATH)/../game_shared

LOCAL_SRC_FILES := aflock.cpp \
	agrunt.cpp \
	airtank.cpp \
	animating.cpp \
	animation.cpp \
	apache.cpp \
	barnacle.cpp \
	barney.cpp \
	bigmomma.cpp \
	bloater.cpp \
	bmodels.cpp \
	briquet.cpp \
	bullsquid.cpp \
	buttons.cpp \
	cbase.cpp \
	client.cpp \
	combat.cpp \
	controller.cpp \
	crowbar.cpp \
	defaultai.cpp \
	diablo.cpp \
	doors.cpp \
	effects.cpp \
	explode.cpp \
	fgrenade.cpp \
	flybee.cpp \
	flyingmonster.cpp \
	fog.cpp \
	fsniper.cpp \
	func_break.cpp \
	func_tank.cpp \
	func_vgui.cpp \
	game.cpp \
	gamerules.cpp \
	gargantua.cpp \
	gauss.cpp \
	genericmonster.cpp \
	ggrenade.cpp \
	globals.cpp \
	gman.cpp \
	h_ai.cpp \
	h_battery.cpp \
	h_cine.cpp \
	h_cycler.cpp \
	h_export.cpp \
	handgrenade.cpp \
	hassassin.cpp \
	headcrab.cpp \
	healthkit.cpp \
	hgrunt.cpp \
	glock.cpp \
	hornet.cpp \
	houndeye.cpp \
	ichthyosaur.cpp \
	irgun.cpp \
	islave.cpp \
	items.cpp \
	leech.cpp \
	lflammes.cpp \
	lights.cpp \
	luciole.cpp \
	m16.cpp \
	maprules.cpp \
	miroir.cpp \
	monstermaker.cpp \
	monsters.cpp \
	monsters.h \
	monsterstate.cpp \
	mortar.cpp \
	mp5.cpp \
	multiplay_gamerules.cpp \
	music.cpp \
	nihilanth.cpp \
	nodes.cpp \
	osprey.cpp \
	pathcorner.cpp \
	plane.cpp \
	plats.cpp \
	player.cpp \
	../pm_shared/pm_debug.c \
	../pm_shared/pm_math.c \
	../pm_shared/pm_shared.c \
	python.cpp \
	radiomsg.cpp \
	rat.cpp \
	roach.cpp \
	rpg.cpp \
	rpggrunt.cpp \
	satchel.cpp \
	schedule.cpp \
	scientist.cpp \
	scripted.cpp \
	shotgun.cpp \
	singleplay_gamerules.cpp \
	skill.cpp \
	sniper.cpp \
	sound.cpp \
	soundent.cpp \
	spectator.cpp \
	squadmonster.cpp \
	squeakgrenade.cpp \
	subs.cpp \
	supergun.cpp \
	t_sub.cpp \
	talkmonster.cpp \
	tank.cpp \
	teamplay_gamerules.cpp \
	tempmonster.cpp \
	tentacle.cpp \
	triggers.cpp \
	tripmine.cpp \
	turret.cpp \
	util.cpp \
	util.h \
	weapons.cpp \
	world.cpp \
	xen.cpp \
	zombie.cpp

LOCAL_LDLIBS := -llog

include $(BUILD_SHARED_LIBRARY)
