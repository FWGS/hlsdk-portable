@echo off
@rem "Edited by Roy for HL:Invasion"
@rem "Warning: Completely UNTESTED!"
@rem "Warning: Won't work properly as this mod requires TRUE VGUI! (If you can edit it in - you're more than welcome!)"
echo Setting environment for minimal Visual C++ 6
set INCLUDE=%MSVCDir%\VC98\Include
set LIB=%MSVCDir%\VC98\Lib
set PATH=%MSVCDir%\VC98\Bin;%MSVCDir%\Common\MSDev98\Bin\;%PATH%

echo -- Compiler is MSVC6

set XASH3DSRC=..\..\Xash3D_original
set INCLUDES=-I../common -I../engine -I../pm_shared -I../game_shared -I../public
set SOURCES=aflock.cpp ^
	agrunt.cpp ^
	airtank.cpp ^
	animating.cpp ^
	animation.cpp ^
	apache.cpp ^
	barnacle.cpp ^
	barney.cpp ^
	bigmomma.cpp ^
	bloater.cpp ^
	bmodels.cpp ^
	briquet.cpp ^
	bullsquid.cpp ^
	buttons.cpp ^
	cbase.cpp ^
	client.cpp ^
	combat.cpp ^
	controller.cpp ^
	crowbar.cpp ^
	defaultai.cpp ^
	diablo.cpp ^
	doors.cpp ^
	effects.cpp ^
	explode.cpp ^
	fgrenade.cpp ^
	flybee.cpp ^
	flyingmonster.cpp ^
	fog.cpp ^
	fsniper.cpp ^
	func_break.cpp ^
	func_tank.cpp ^
	func_vgui.cpp ^
	game.cpp ^
	gamerules.cpp ^
	gargantua.cpp ^
	gauss.cpp ^
	genericmonster.cpp ^
	ggrenade.cpp ^
	globals.cpp ^
	gman.cpp ^
	h_ai.cpp ^
	h_battery.cpp ^
	h_cine.cpp ^
	h_cycler.cpp ^
	h_export.cpp ^
	handgrenade.cpp ^
	hassassin.cpp ^
	headcrab.cpp ^
	healthkit.cpp ^
	hgrunt.cpp ^
	glock.cpp ^
	hornet.cpp ^
	houndeye.cpp ^
	ichthyosaur.cpp ^
	irgun.cpp ^
	islave.cpp ^
	items.cpp ^
	leech.cpp ^
	lflammes.cpp ^
	lights.cpp ^
	luciole.cpp ^
	m16.cpp ^
	maprules.cpp ^
	miroir.cpp ^
	monstermaker.cpp ^
	monsters.cpp ^
	monsters.h ^
	monsterstate.cpp ^
	mortar.cpp ^
	mp5.cpp ^
	multiplay_gamerules.cpp ^
	music.cpp ^
	nihilanth.cpp ^
	nodes.cpp ^
	osprey.cpp ^
	pathcorner.cpp ^
	plane.cpp ^
	plats.cpp ^
	player.cpp ^
	python.cpp ^
	radiomsg.cpp ^
	rat.cpp ^
	roach.cpp ^
	rpg.cpp ^
	rpggrunt.cpp ^
	satchel.cpp ^
	schedule.cpp ^
	scientist.cpp ^
	scripted.cpp ^
	shotgun.cpp ^
	singleplay_gamerules.cpp ^
	skill.cpp ^
	sniper.cpp ^
	sound.cpp ^
	soundent.cpp ^
	spectator.cpp ^
	squadmonster.cpp ^
	squeakgrenade.cpp ^
	subs.cpp ^
	supergun.cpp ^
	t_sub.cpp ^
	talkmonster.cpp ^
	tank.cpp ^
	teamplay_gamerules.cpp ^
	tempmonster.cpp ^
	tentacle.cpp ^
	triggers.cpp ^
	tripmine.cpp ^
	turret.cpp ^
	util.cpp ^
	util.h ^
	weapons.cpp ^
	world.cpp ^
	xen.cpp ^
	zombie.cpp ^
	../pm_shared/pm_debug.c ../pm_shared/pm_math.c ../pm_shared/pm_shared.c
set DEFINES=/DCLIENT_WEAPONS /Dsnprintf=_snprintf /DNO_VOICEGAMEMGR
set LIBS=user32.lib
set OUTNAME=hl.dll
set DEBUG=/debug

cl %DEFINES% %LIBS% %SOURCES% %INCLUDES% -o %OUTNAME% /link /dll /out:%OUTNAME% %DEBUG% /def:".\hl.def"

echo -- Compile done. Cleaning...

del *.obj *.exp *.lib *.ilk
echo -- Done.
