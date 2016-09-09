# Makefile for server library
# Copyright © THE-Swank 2016

CC ?= gcc
CXX ?= g++
ARCH ?= $(shell uname -m)

LIBNAME := hl

CFLAGS := -pipe
CFLAGS += \
    -D_LINUX -DCLIENT_WEAPONS -DNO_VOICEGAMEMGR \
    -Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf

CFLAGS_RELEASE := -Ofast -w
CFLAGS_DEBUG := -Wall -Wno-invalid-offsetof -g -Og

CXX_ONLY_FLAGS := 
C_ONLY_FLAGS := 

CFLAGS_OPT_X86 := -march=native -mtune=native
CFLAGS_OPT_ARM := -mthumb -mfloat-abi=hard -mfpu=neon -mvectorize-with-neon-quad -mcpu=cortex-a9

LDLIBS := -lstdc++ -lm -lc
LDFLAGS := -fvisibility=hidden -Wl,--no-warn-mismatch

SRC_DIR := .
ENGINE_SRC_DIR := ../engine
COMMON_SRC_DIR := ../common
PM_SHARED_SRC_DIR := ../pm_shared
GAME_SHARED_SRC_DIR := ../game_shared

OBJ_DIR := $(SRC_DIR)/obj
PM_SHARED_OBJ_DIR := $(PM_SHARED_SRC_DIR)/obj

INCLUDES := \
    -I . \
    -I $(ENGINE_SRC_DIR) \
    -I $(COMMON_SRC_DIR) \
    -I $(PM_SHARED_SRC_DIR) \
    -I $(GAME_SHARED_SRC_DIR)

OBJECTS := \
    $(OBJ_DIR)/aflock.o \
    $(OBJ_DIR)/agrunt.o \
    $(OBJ_DIR)/airtank.o \
    $(OBJ_DIR)/animating.o \
    $(OBJ_DIR)/animation.o \
    $(OBJ_DIR)/apache.o \
    $(OBJ_DIR)/barnacle.o \
    $(OBJ_DIR)/barney.o \
    $(OBJ_DIR)/bigmomma.o \
    $(OBJ_DIR)/bloater.o \
    $(OBJ_DIR)/bmodels.o \
    $(OBJ_DIR)/bullsquid.o \
    $(OBJ_DIR)/buttons.o \
    $(OBJ_DIR)/cbase.o \
    $(OBJ_DIR)/client.o \
    $(OBJ_DIR)/combat.o \
    $(OBJ_DIR)/controller.o \
    $(OBJ_DIR)/crossbow.o \
    $(OBJ_DIR)/crowbar.o \
    $(OBJ_DIR)/defaultai.o \
    $(OBJ_DIR)/doors.o \
    $(OBJ_DIR)/effects.o \
    $(OBJ_DIR)/egon.o \
    $(OBJ_DIR)/explode.o \
    $(OBJ_DIR)/flyingmonster.o \
    $(OBJ_DIR)/func_break.o \
    $(OBJ_DIR)/func_tank.o \
    $(OBJ_DIR)/game.o \
    $(OBJ_DIR)/gamerules.o \
    $(OBJ_DIR)/gargantua.o \
    $(OBJ_DIR)/gauss.o \
    $(OBJ_DIR)/genericmonster.o \
    $(OBJ_DIR)/ggrenade.o \
    $(OBJ_DIR)/globals.o \
    $(OBJ_DIR)/glock.o \
    $(OBJ_DIR)/gman.o \
    $(OBJ_DIR)/h_ai.o \
    $(OBJ_DIR)/h_battery.o \
    $(OBJ_DIR)/h_cine.o \
    $(OBJ_DIR)/h_cycler.o \
    $(OBJ_DIR)/h_export.o \
    $(OBJ_DIR)/handgrenade.o \
    $(OBJ_DIR)/hassassin.o \
    $(OBJ_DIR)/headcrab.o \
    $(OBJ_DIR)/healthkit.o \
    $(OBJ_DIR)/hgrunt.o \
    $(OBJ_DIR)/hornet.o \
    $(OBJ_DIR)/hornetgun.o \
    $(OBJ_DIR)/houndeye.o \
    $(OBJ_DIR)/ichthyosaur.o \
    $(OBJ_DIR)/islave.o \
    $(OBJ_DIR)/items.o \
    $(OBJ_DIR)/leech.o \
    $(OBJ_DIR)/lights.o \
    $(OBJ_DIR)/maprules.o \
    $(OBJ_DIR)/monstermaker.o \
    $(OBJ_DIR)/monsters.o \
    $(OBJ_DIR)/monsterstate.o \
    $(OBJ_DIR)/mortar.o \
    $(OBJ_DIR)/mp5.o \
    $(OBJ_DIR)/multiplay_gamerules.o \
    $(OBJ_DIR)/nihilanth.o \
    $(OBJ_DIR)/nodes.o \
    $(OBJ_DIR)/osprey.o \
    $(OBJ_DIR)/pathcorner.o \
    $(OBJ_DIR)/plane.o \
    $(OBJ_DIR)/plats.o \
    $(OBJ_DIR)/player.o \
    $(OBJ_DIR)/python.o \
    $(OBJ_DIR)/rat.o \
    $(OBJ_DIR)/roach.o \
    $(OBJ_DIR)/rpg.o \
    $(OBJ_DIR)/satchel.o \
    $(OBJ_DIR)/schedule.o \
    $(OBJ_DIR)/scientist.o \
    $(OBJ_DIR)/scripted.o \
    $(OBJ_DIR)/shotgun.o \
    $(OBJ_DIR)/singleplay_gamerules.o \
    $(OBJ_DIR)/skill.o \
    $(OBJ_DIR)/sound.o \
    $(OBJ_DIR)/soundent.o \
    $(OBJ_DIR)/spectator.o \
    $(OBJ_DIR)/squadmonster.o \
    $(OBJ_DIR)/squeakgrenade.o \
    $(OBJ_DIR)/subs.o \
    $(OBJ_DIR)/talkmonster.o \
    $(OBJ_DIR)/teamplay_gamerules.o \
    $(OBJ_DIR)/tempmonster.o \
    $(OBJ_DIR)/tentacle.o \
    $(OBJ_DIR)/triggers.o \
    $(OBJ_DIR)/tripmine.o \
    $(OBJ_DIR)/turret.o \
    $(OBJ_DIR)/util.o \
    $(OBJ_DIR)/weapons.o \
    $(OBJ_DIR)/world.o \
    $(OBJ_DIR)/xen.o \
    $(PM_SHARED_OBJ_DIR)/pm_debug.o \
    $(PM_SHARED_OBJ_DIR)/pm_math.o \
    $(PM_SHARED_OBJ_DIR)/pm_shared.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(DO_CXX)

$(PM_SHARED_OBJ_DIR)/%.o: $(PM_SHARED_SRC_DIR)/%.c
	$(DO_CC)

##############################

DO_CC = $(CC) $(CFLAGS) $(C_ONLY_FLAGS) $(INCLUDES) -fPIC -c $< -o $@
DO_CXX = $(CXX) $(CFLAGS) $(CXX_ONLY_FLAGS) $(INCLUDES) -fPIC -c $< -o $@

COMPILE_ARCH := $(ARCH)
COMPILE_ARCH := $(COMPILE_ARCH:i%86=x86)
COMPILE_ARCH := $(COMPILE_ARCH:arm%=arm)

ifeq ($(ARCH), i86pc)
    COMPILE_ARCH := x86
endif
ifeq ($(ARCH), x64)
    COMPILE_ARCH := x86_64
endif
ifeq ($(ARCH), amd64)
    COMPILE_ARCH := x86_64
endif

ifeq ($(COMPILE_ARCH), x86)
    CFLAGS += $(CFLAGS_OPT_X86)
endif
ifeq ($(COMPILE_ARCH), x86_64)
    CFLAGS += $(CFLAGS_OPT_X86)
endif
ifeq ($(COMPILE_ARCH), arm)
    CFLAGS += $(CFLAGS_OPT_ARM)
endif

release: CFLAGS += $(CFLAGS_RELEASE)
release: build printresult

debug: CFLAGS += $(CFLAGS_DEBUG)
debug: build printresult

build: createobjdir $(OBJECTS)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(LDLIBS) -shared -o $(LIBNAME).so $(OBJECTS)

printresult:
	@echo ""
	@echo " #########################"
	@echo " #   Build info          #"
	@echo " #########################"
	@echo "  target: $(LIBNAME).so"
	@echo "  size: `stat -c%s $(LIBNAME).so`"
	@echo ""
	@echo "  COMPILE_ARCH: $(COMPILE_ARCH)"
	@echo "  CC: $(CC)"
	@echo "  CXX: $(CXX)"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  C_ONLY_FLAGS: $(C_ONLY_FLAGS)"
	@echo "  CXX_ONLY_FLAGS: $(CXX_ONLY_FLAGS)"
	@echo "  LDFLAGS: $(LDLIBS) $(LDFLAGS)"
	@echo "  OBJECTS: $(OBJECTS)"
	@echo ""
	@echo " #########################"
	@if [ -s $(LIBNAME).so ]; then \
	    echo " #   Build succesfuly!  #"; \
	else \
	    echo " #   Build FAIL!        #"; \
	fi
	@echo " #########################"

createobjdir:
	-mkdir $(OBJ_DIR) $(PM_SHARED_OBJ_DIR)

clean:
	-rm -rf $(OBJ_DIR) $(PM_SHARED_OBJ_DIR) $(LIBNAME).so
