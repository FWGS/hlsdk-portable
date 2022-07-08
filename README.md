# Half-life: Invasion
## A French single-player mod by JujU, Yag, D.X.M., Fregman, Manu and the company.

This is a humble attempt by Uncle Roy to update the mod's source code
to be compileable for modern Xash3d FWGS builds for GNU/Linux systems.

The original source code can be obtained on the mod's ModDB page,
and was released by JujU in 2015:

https://www.moddb.com/mods/half-life-invasion

https://www.moddb.com/mods/half-life-invasion/downloads/invasion-source-code

Obviously, this attempt didn't go as smoothly as originally planned,
and a great lot of things needed to be updated, fixed or worked around,
as the SDK has evolved and changed greatly since the mod's release
circa 2001.

While this version of the code compiles properly on Debian Linux 11 and
allows one to play the mod from start to finish,
and most of the things seem to work as they should, it is by no means
ideal or polished. Many of my fixes may seem haphazard and some are perhaps
not entirely necessary (although all of them work around specific issues
I have encountered), so I've tried to use #define's where possible,
to make it easy to undo my fixes and try different solutions. See cl_dll/crutches.h.

Also, many of JujU's original comments used non-UTF8 French enconding and were sadly
partially lost in file processing. Luckily, referring to the original source code is
always possible.

STILL NEEDS CODE CLEAN-UP!!!

## Known bugs

Known bugs not fixed while upgrading \ recovering code:
-Sniper rifle still has no scope (black box with a transparent circle).
-Water Wave still doesn't work properly.
Admittedly, neither seems to work with Xash3d (at least FWGS) even with original DLLs when run with Proton.

-Also, due to the nature of the tank sound fix, it's weapons produce no visible muzzle effect
when the player's selected weapon is glock (see below).

### Bugs fixed

Things fixed while transferring code:
- Reload animations, sounds, e.t.c on certain weapons (Beretta, Shotgun, Handgrenade) didn't work. Julien's original code for non-client based weapons was used with some necessary edits.
- Sniper rifle zoom states didn't work, m_pPlayer->pev->fov needed to be equalized to m_pPlayer->m_iFOV.
- VGUI used to hard-crash, partially fixed: CImageLables in vgui_OrdiControl that caused the crash were replaced with CommandButtons, skipTime added to keypad, OrdiMenu and OrdiControl in order to fix double mouse key presses, +1 added to radiomsg.cpp to fix the butts of the text messages, HUD health display didn't work due to Health Msg system receieving Battery-related data, fixed by manually re-coping a part of old code and paritioning it properly.
- Also fixed .txt files and fonts not loading due to backslashes, VGUI folder name being written in capitals and such.
- Music not playing fixed by creating a GStreamer implementation.
- l2m3 has a green exploding tank, which has func_door's named tremble_1 and tremble_2, when activated they cause SegFault on any non GoldSource-compatible build. Don't know why yet. Fixed by replacing a multi-manager target with garbage names if the map name and targetname match, see triggers.cpp.
- Tank used to have a weird sound (glock event playback) when primary attack is activated, fixed with m_iPlayerInTankExternal see hud_tank.cpp and hl_weapons.cpp.
- IR gun didn't actually Infra-Red anything, fixed by transferring changes to StudioModelRenderer.cpp.
- It was possible to fire while entering codes, e.t.c, fixed (see hlinv_isAttackSuspended).
- Cameras on l3m3 (monster_camera) should not have Thinks and shouldn't have pSprite in m_SaveData, otherwise it crashes on save, on touch (trigger), e.t.c., fixed.
- Trigger_gaz and IsInGaz have also been fixed by making them non-virtual, and moving IsInGaz function wholly to CBaseEntity, but making it return FALSE unless IsPlayer returns TRUE (which it never does, until it gets called from CBasePlayer instance).

The mod is playable from start to finish, with the main obvious drawbacks being:
- weapons may be 'clogged' on loading game (fixed by playing with whichever ones aren't clogged until they get unclogged, probably caused by DLL checksum failing after a recompile),
- Water Waves are not drawn, so aren't sniper rifle scope sprites.
- and the obvious < and > symbols instead of graphical arrows at the conveyor puzzle.

## A word of warning about build systems besides cmake

WARNING! While I've tried editing the make files for other build systems,
only cmake build was actually tested.
Also, the mod won't work correctly with Android (due to lack of true VGUI),
MSVC6 (haven't edited true VGUI compilation in, as IDK how to do that properly)
and wscript (same reason).
The premake5.lua was also edited and should, in theory, compile (as it contains
VGUI compilation directives), but gstreamer wasn't edited in.
If you can edit the VGUI in or fix any of those other build
systems - you're more than welcome to do so!

## How to build

### CMake as the only tested way

	mkdir build
	cd build
	cmake ../ -DUSE_VGUI=1 -DUSE_GSTREAMER=1
	make

On Debian, the following is necessary to:

	sudo apt install libgstreamer1.0:i386 #to play with music (also install plugins)
	sudo apt install libgstreamer1.0-dev:i386 #to compile with music (-DUSE_GSTREAMER=1)

Also added -DUSE_FMOD option for cmake and tried my best to prevent it being used in Linux (uses windows.h include).

The following will likely be necessary to compile a gold-source compatible (old xash, e.t.c) binaries:
	
	sudo apt install libsdl2-dev:i386

(but this wants to install a ton of i386 dev packages)

Must be built with -DUSE_VGUI=1 at least to work properly (thus, update the modules when clonning).
Must be built with either -DUSE_FMOD=1 (untested, on Windows and will require fmod headers v 3.6.1 if you can still get them somewhere),
or -DUSE_GSTREAMER=1 (and have appropriate architechture (i386) gstreamer-1.0 and dev packages installed) to be able to play with music.
So far GStreamer should only compile on Linux, as I haven't tested it (or linking it) on any other system.
Also, dlls/CMakeLists.txt must have a system-appropriate path to i386 version of GStreamer headers.
See set for ```ENV{PKG_CONFIG_PATH}```.