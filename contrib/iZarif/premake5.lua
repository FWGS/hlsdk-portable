workspace("hlsdk-xash3d")
configurations{"Debug", "Release"}
language("C++")
kind("SharedLib")
rtti("Off")
exceptionhandling("Off")
pic("On")
targetprefix("")

files{"pm_shared/pm_debug.c",
"pm_shared/pm_math.c",
"pm_shared/pm_shared.c",
"dlls/crossbow.cpp",
"dlls/crowbar.cpp",
"dlls/egon.cpp",
"dlls/gauss.cpp",
"dlls/glock.cpp",
"dlls/handgrenade.cpp",
"dlls/hornetgun.cpp",
"dlls/mp5.cpp",
"dlls/python.cpp",
"dlls/rpg.cpp",
"dlls/satchel.cpp",
"dlls/shotgun.cpp",
"dlls/squeakgrenade.cpp",
"dlls/tripmine.cpp"}

includedirs{"dlls",
"common",
"engine",
"pm_shared",
"game_shared"}

defines{"BARNACLE_FIX_VISIBILITY=0",
"CLIENT_WEAPONS=1",
"CROWBAR_IDLE_ANIM=0",
"CROWBAR_DELAY_FIX=0",
"CROWBAR_FIX_RAPID_CROWBAR=0",
"GAUSS_OVERCHARGE_FIX=0",
"OEM_BUILD=0",
"HLDEMO_BUILD=0"}

newoption{trigger = "64bits", description = "Allow targetting 64-bit engine", default = false}
newoption{trigger = "voicemgr", description = "Eable voice manager", default = false}
newoption{trigger = "goldsrc", description = "Enable GoldSource engine support", default = false}
newoption{trigger = "lto", description = "Enable Link Time Optimization", default = false}

if _OPTIONS["64bits"] then
  architecture("x86_64")
else
  architecture("x86")
end

if _OPTIONS["lto"] then
  flags{"LinkTimeOptimization"}
end

if os.findheader("tgmath.h") then
  defines{"HAVE_TGMATH_H"}
end

if os.findheader("cmath") then
  defines{"HAVE_CMATH"}
end

filter("configurations:Release")
optimize("Full")
symbols("Off")
omitframepointer("On")

filter("configurations:Debug")
optimize("Off")
symbols("On")
omitframepointer("Off")

filter("toolset:msc")
buildoptions{"/D_USING_V110_SDK71_", "/Zc:threadSafeInit-"}
defines{"_CRT_SECURE_NO_WARNINGS=1", "_CRT_NONSTDC_NO_DEPRECATE=1"}

filter("toolset:gcc or clang")
visibility("Hidden")
defines{"stricmp=strcasecmp",
"strnicmp=strncasecmp",
"_snprintf=snprintf",
"_vsnprintf=vsnprintf",
"_LINUX",
"LINUX"}

project("dlls")
targetname("hl")

files{"dlls/agrunt.cpp",
"dlls/airtank.cpp",
"dlls/aflock.cpp",
"dlls/animating.cpp",
"dlls/animation.cpp",
"dlls/apache.cpp",
"dlls/barnacle.cpp",
"dlls/barney.cpp",
"dlls/bigmomma.cpp",
"dlls/bloater.cpp",
"dlls/bmodels.cpp",
"dlls/bullsquid.cpp",
"dlls/buttons.cpp",
"dlls/cbase.cpp",
"dlls/client.cpp",
"dlls/combat.cpp",
"dlls/controller.cpp",
"dlls/defaultai.cpp",
"dlls/doors.cpp",
"dlls/effects.cpp",
"dlls/explode.cpp",
"dlls/flyingmonster.cpp",
"dlls/func_break.cpp",
"dlls/func_tank.cpp",
"dlls/game.cpp",
"dlls/gamerules.cpp",
"dlls/gargantua.cpp",
"dlls/genericmonster.cpp",
"dlls/ggrenade.cpp",
"dlls/globals.cpp",
"dlls/gman.cpp",
"dlls/h_ai.cpp",
"dlls/h_battery.cpp",
"dlls/h_cine.cpp",
"dlls/h_cycler.cpp",
"dlls/h_export.cpp",
"dlls/hassassin.cpp",
"dlls/headcrab.cpp",
"dlls/healthkit.cpp",
"dlls/hgrunt.cpp",
"dlls/hornet.cpp",
"dlls/houndeye.cpp",
"dlls/ichthyosaur.cpp",
"dlls/islave.cpp",
"dlls/items.cpp",
"dlls/leech.cpp",
"dlls/lights.cpp",
"dlls/maprules.cpp",
"dlls/monstermaker.cpp",
"dlls/monsters.cpp",
"dlls/monsterstate.cpp",
"dlls/mortar.cpp",
"dlls/multiplay_gamerules.cpp",
"dlls/nihilanth.cpp",
"dlls/nodes.cpp",
"dlls/observer.cpp",
"dlls/osprey.cpp",
"dlls/pathcorner.cpp",
"dlls/plane.cpp",
"dlls/plats.cpp",
"dlls/player.cpp",
"dlls/playermonster.cpp",
"dlls/rat.cpp",
"dlls/roach.cpp",
"dlls/schedule.cpp",
"dlls/scientist.cpp",
"dlls/scripted.cpp",
"dlls/singleplay_gamerules.cpp",
"dlls/skill.cpp",
"dlls/sound.cpp",
"dlls/soundent.cpp",
"dlls/spectator.cpp",
"dlls/squadmonster.cpp",
"dlls/subs.cpp",
"dlls/talkmonster.cpp",
"dlls/teamplay_gamerules.cpp",
"dlls/tempmonster.cpp",
"dlls/tentacle.cpp",
"dlls/triggers.cpp",
"dlls/turret.cpp",
"dlls/util.cpp",
"dlls/weapons.cpp",
"dlls/world.cpp",
"dlls/xen.cpp",
"dlls/zombie.cpp"}

includedirs{"public"}

if _OPTIONS["voicemgr"] then
  files{"game_shared/voice_gamemgr.cpp"}
else
  defines{"NO_VOICEGAMEMGR"}
end

filter("toolset:msc")
buildoptions{"/def:" .. path.getabsolute("dlls/hl.def")}

project("cl_dll")
targetname("client")

files{"cl_dll/hl/hl_baseentity.cpp",
"cl_dll/hl/hl_events.cpp",
"cl_dll/hl/hl_objects.cpp",
"cl_dll/hl/hl_weapons.cpp",
"cl_dll/GameStudioModelRenderer.cpp",
"cl_dll/MOTD.cpp",
"cl_dll/StudioModelRenderer.cpp",
"cl_dll/ammo.cpp",
"cl_dll/ammo_secondary.cpp",
"cl_dll/ammohistory.cpp",
"cl_dll/battery.cpp",
"cl_dll/cdll_int.cpp",
"cl_dll/com_weapons.cpp",
"cl_dll/death.cpp",
"cl_dll/demo.cpp",
"cl_dll/entity.cpp",
"cl_dll/ev_hldm.cpp",
"cl_dll/ev_common.cpp",
"cl_dll/events.cpp",
"cl_dll/flashlight.cpp",
"cl_dll/geiger.cpp",
"cl_dll/health.cpp",
"cl_dll/hud.cpp",
"cl_dll/hud_msg.cpp",
"cl_dll/hud_redraw.cpp",
"cl_dll/hud_spectator.cpp",
"cl_dll/hud_update.cpp",
"cl_dll/in_camera.cpp",
"cl_dll/input.cpp",
"cl_dll/input_goldsource.cpp",
"cl_dll/input_mouse.cpp",
"cl_dll/input_xash3d.cpp",
"cl_dll/menu.cpp",
"cl_dll/message.cpp",
"cl_dll/overview.cpp",
"cl_dll/parsemsg.cpp",
"cl_dll/saytext.cpp",
"cl_dll/scoreboard.cpp",
"cl_dll/status_icons.cpp",
"cl_dll/statusbar.cpp",
"cl_dll/studio_util.cpp",
"cl_dll/text_message.cpp",
"cl_dll/train.cpp",
"cl_dll/tri.cpp",
"cl_dll/util.cpp",
"cl_dll/view.cpp"}

includedirs{"cl_dll",
"cl_dll/hl",
"utils/false_vgui/include"}

defines{"CLIENT_DLL"}

if _OPTIONS["goldsrc"] then
  defines{"GOLDSOURCE_SUPPORT"}
end

filter("system:Windows")
links{"user32", "winmm"}

filter("system:not windows")
links{"dl"}

