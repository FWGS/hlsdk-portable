#
# Half-Life Full SDK 2.3 hl_i***.so Makefile for Linux
#
# August 2001 by Leon Hartwig (hartwig@valvesoftware.com)
#
# Modified October 2001 by Jeff "DarthBobo" Fearn (DarthBobo@swarm.edgegaming.com)
# Added:
#	copying dll to mod folder
#	smart source file recognition
#	Cleaner deletion of intermediate files
#
# Modified October 2001 by Jeff "DarthBobo" Fearn (DarthBobo@swarm.edgegaming.com)
# Added:
#	Moved copy cody to install label (Motivation by Darius ;)
#
# Modified Decenber 2002 by Jeff "Codiac" Fearn (Codiac@swarm.edgegaming.com)
# I got bored of darthbobo :}
#	Changed for Spirit Of Halflife, moved extra files to Extras dir.
#	Fixed CFLAGS to use ARCH

DLLNAME=spirit

# used for so name and compile architecture type
ARCH=i686

# Make sure this is the correct compiler for your system
# Can only use gcc 2.96+ compilers on newer Linux servers
CC=gcc -m32
#CC=gcc296

# For older Linux servers or FBSD servers use compat version
#CC=kgcc

# Pinched this from Botmans HPB_Bot Makefile
# the following specifies the path to your MOD...
# I created a HL user, just a half baked security thing :}
MOD_DIR = /home/hl/hlds_l/spirit

DLL_SRCDIR=.
ENGINE_SRCDIR=../engine
COMMON_SRCDIR=../common
WPN_SHARED_SRCDIR=./wpn_shared
PM_SHARED_SRCDIR=../pm_shared
GAME_SHARED_SRCDIR=../game_shared

DLL_OBJDIR=$(DLL_SRCDIR)/linux_obj
WPN_SHARED_OBJDIR=$(WPN_SHARED_SRCDIR)/linux_obj
PM_SHARED_OBJDIR=$(PM_SHARED_SRCDIR)/linux_obj
GAME_SHARED_OBJDIR=$(GAME_SHARED_SRCDIR)/linux_obj

BASE_CFLAGS=	-Dstricmp=strcasecmp -D_strnicmp=strncasecmp -Dstrnicmp=strncasecmp \
		-DCLIENT_WEAPONS -fpermissive

#safe optimization
#CFLAGS=$(BASE_CFLAGS)  -Wall -Wno-conversion -march=$(ARCH) -O1
#CFLAGS=$(BASE_CFLAGS)  -Wall -march=$(ARCH) -O1
CFLAGS=$(BASE_CFLAGS) -w -march=$(ARCH) -O1

#full optimization
#CFLAGS=$(BASE_CFLAGS) -Wall -O1 -march=$(ARCH) -ffast-math -funroll-loops \
#	-fomit-frame-pointer -fexpensive-optimizations \
#	-malign-loops=2 -malign-jumps=2 -malign-functions=2

#use these when debugging
#CFLAGS=$(BASE_CFLAGS) -Wall -g
#CFLAGS=$(BASE_CFLAGS) -w -g

INCLUDEDIRS=-I. -I$(ENGINE_SRCDIR) -I$(COMMON_SRCDIR) -I$(PM_SHARED_SRCDIR) -I$(GAME_SHARED_SRCDIR) -I..

# Need to specifically link Maths and C++ libraries in latest gcc
#LDFLAGS=-lm -lstdc++

# debugging
LDFLAGS=-lm -lstdc++

SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

DO_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) $(INCLUDEDIRS) -o $@ -c $<

#############################################################################
# SETUP AND BUILD
# GAME
#############################################################################

$(DLL_OBJDIR)/%.o: $(DLL_SRCDIR)/%.cpp
	$(DO_CC)

$(WPN_SHARED_OBJDIR)/%.o: $(WPN_SHARED_SRCDIR)/%.cpp
	$(DO_CC)

$(GAME_SHARED_OBJDIR)/%.o: $(GAME_SHARED_SRCDIR)/%.cpp
	$(DO_CC)

$(PM_SHARED_OBJDIR)/%.o: $(PM_SHARED_SRCDIR)/%.c
	$(DO_CC)

# Here is the magic, I swear info pages are even harder to read than man pages!
# Just get every cpp file in dll src dir
# Then substitute the src directory and .cpp with the object dir and .o
#NOTE:
#	I found some "extra" files in the dll src dir!
#	I removed them without any problem:
#		wxdebug.cpp & .h, AI_BaseNPC_Schedule.cpp & mpstubb.cpp
#	I think they are source from the HLDS program ...

OBJ := $(patsubst $(DLL_SRCDIR)/%.cpp,$(DLL_OBJDIR)/%.o,$(wildcard $(DLL_SRCDIR)/*.cpp))
OBJ += $(patsubst $(WPN_SHARED_SRCDIR)/%.cpp,$(WPN_SHARED_OBJDIR)/%.o,$(wildcard  $(WPN_SHARED_SRCDIR)/*.cpp))
OBJ += $(GAME_SHARED_OBJDIR)/voice_gamemgr.o
OBJ += $(patsubst $(PM_SHARED_SRCDIR)/%.c,$(PM_SHARED_OBJDIR)/%.o,$(wildcard $(PM_SHARED_SRCDIR)/*.c))

$(DLLNAME)_$(ARCH).$(SHLIBEXT) : neat $(OBJ)
	$(CC) $(CFLAGS) $(SHLIBLDFLAGS) $(LDFLAGS) -o $@ $(OBJ)

# I'm to lazy to keep manually copying the *&%(&% so file over :}
# Thanks to Darius for kicking my arse about where this should go ;)
install:
	cp -f $(DLLNAME)_$(ARCH).$(SHLIBEXT) ${MOD_DIR}/dlls/.

neat:
	-mkdir $(DLL_OBJDIR)
	-mkdir $(WPN_SHARED_OBJDIR)
	-mkdir $(GAME_SHARED_OBJDIR)
	-mkdir $(PM_SHARED_OBJDIR)

clean:
# Doing it this way means if we remove a cpp file the .o file will still get removed, the old way would not.
	-rm -f $(DLL_OBJDIR)/*
	-rm -f $(WPN_SHARED_OBJDIR)/*
	-rm -f $(GAME_SHARED_OBJDIR)/*
	-rm -f $(PM_SHARED_OBJDIR)/*
	-rm -f $(DLLNAME)_$(ARCH).$(SHLIBEXT)

spotless: clean
	-rm -r $(DLL_OBJDIR)
	-rm -r $(WPN_SHARED_OBJDIR)
	-rm -r $(GAME_SHARED_OBJDIR)
	-rm -r $(PM_SHARED_OBJDIR)
