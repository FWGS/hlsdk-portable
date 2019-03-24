CC ?= gcc
CXX ?= g++

SRCS+=../dlls/tfc/tf_wpn_ac.cpp
SRCS+=../dlls/tfc/tf_wpn_ar.cpp
SRCS+=../dlls/tfc/tf_wpn_axe.cpp
SRCS+=../dlls/tfc/tf_wpn_flame.cpp
SRCS+=../dlls/tfc/tf_wpn_gl.cpp
SRCS+=../dlls/tfc/tf_wpn_ic.cpp
SRCS+=../dlls/tfc/tf_wpn_ng.cpp
SRCS+=../dlls/tfc/tf_wpn_railgun.cpp
SRCS+=../dlls/tfc/tf_wpn_rpg.cpp
SRCS+=../dlls/tfc/tf_wpn_sg.cpp
SRCS+=../dlls/tfc/tf_wpn_srifle.cpp
SRCS+=../dlls/tfc/tf_wpn_tranq.cpp
SRCS+=./tfc/ev_tfc.cpp
SRCS+=./tfc/tf_baseentity.cpp
SRCS+=./tfc/tf_events.cpp
SRCS+=./tfc/tf_objects.cpp
SRCS+=./tfc/tf_weapons.cpp
SRCS+=./tfc/touch.cpp
SRCS+=../dlls/crowbar.cpp
SRCS+=../common/interface.cpp
SRCS+=./ammo.cpp
SRCS+=./ammo_secondary.cpp
SRCS+=./ammohistory.cpp
SRCS+=./battery.cpp
SRCS+=./cdll_int.cpp
SRCS+=./com_weapons.cpp
SRCS+=./death.cpp
SRCS+=./demo.cpp
SRCS+=./entity.cpp
SRCS+=./ev_common.cpp
SRCS+=./events.cpp
SRCS+=./flashlight.cpp
SRCS+=./GameStudioModelRenderer.cpp
SRCS+=./geiger.cpp
SRCS+=./health.cpp
SRCS+=./hud.cpp
SRCS+=./hud_msg.cpp
SRCS+=./hud_redraw.cpp
SRCS+=./hud_spectator.cpp
SRCS+=./hud_update.cpp
SRCS+=./in_camera.cpp
SRCS+=./input.cpp
SRCS+=./input_goldsource.cpp
SRCS+=./input_mouse.cpp
SRCS+=./menu.cpp
SRCS+=./message.cpp
SRCS+=./overview.cpp
SRCS+=./parsemsg.cpp
SRCS_C+=../pm_shared/pm_debug.c
SRCS_C+=../pm_shared/pm_math.c
SRCS_C+=../pm_shared/pm_shared.c
SRCS+=./saytext.cpp
SRCS+=./status_icons.cpp
SRCS+=./statusbar.cpp
SRCS+=./studio_util.cpp
SRCS+=./StudioModelRenderer.cpp
SRCS+=./text_message.cpp
SRCS+=./train.cpp
SRCS+=./tri.cpp
SRCS+=./util.cpp
SRCS+=./view.cpp
SRCS+=./input_xash3d.cpp
SRCS+=./scoreboard.cpp
SRCS+=./MOTD.cpp

INCLUDES = -I../common -I. -I../game_shared -I../pm_shared -I../engine -I../dlls -Itfc -I../dlls/tfc -I../utils/false_vgui/include
DEFINES = -Wno-write-strings -DLINUX -D_LINUX -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DCLIENT_WEAPONS -DCLIENT_DLL
CFLAGS = -m32 -g
OBJS = $(SRCS:.cpp=.o) $(SRCS_C:.c=.o)
LIBS = -lm

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) $(DEFINES) -c $< -o $@

all : $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -g -o client.dll -shared -Wl,--no-undefined $(LIBS)

clean:
	-cmd /c del $(subst /,\,$(OBJS))
