
# GravGunMod

Extended Half-Life mod

### Main features:

* New weapons (GravGun, AR2, BIG_COCK, Gate of Babylon)
* Pseudo-physics for weapons, ammo, items, props
* Co-op mode
* Advanced game-side enitity tools
* Advanced menu, motd and help system
* Server-side visibility limit
* error.mdl support (like in Source Engine)
* Other tools

## Gravity gun and pseudo-physics

Gravity Gun may drag props, weapons, ammos, items, monsters and pushables
`mp_gravgun_players` option allows dragging players

prop is special entity for pseudo-physical objects (alternative to prop_physics from Source)
While it still use hull for collision, it may change rotation angle and size for fake shape.

### Supported keyvalues:

`"shape"` - fake shape number

Supported shapes:

 * cylinder horizontal (0)
 * cylinder vertical (1)
 * generic (3)
 * sphere (4)

`"respawntime"` : idle time before respawn (for multiplayer)

`"customanglesx"`, `"customanglesz"` : custom angles for generic shape

`"hmin"`, `"hmax"`, `"vmin"`, `"vmax"` : size vectors for horizontal and vertical mode.
All shapes except vertical cylinder uses horizontal size. Horizontal and vertical cylinders changes dynamically

Breakable options: `"material"`,  `"explosion"`, `"gibmodel"`, `"explodemagnitude"`

Use `dumpprops` command to dump all spawned props to `props.ent` file (need `sv_cheats`)

## Cooperative mode

Cooperative mode still does not work correctly in half-life, you need to remove all blockers on `c3a1` map with entpatch

### Sample cooperative config:

```
set mp_coop 1 // enable cooperative mode. This enables frags from some monsters and unlocks other coop features like menu
set mp_spectator 1 // make players spawned in spectator mode
set mp_coop_changelevel 1 // enable changelevel with hacks
set mp_coop_nofriendlyfire 1 // disallow damage players
set mp_skipdefaults 1 // do not give glock/crowbar on player spawn
set mp_allowmonsters 1 // allow monsters in multiplayer
set mp_flashlight 1 // allow flashlight
set mp_unduck 1 // allow duck hack. need to prevent blocking players on changelevel
set mp_semclip 1 // semclip mode. prevent players blocking narrow places
set mp_coop_checkpoints 1 // allow checkpoints. every player may create checkpoint to continue after respawn
set mp_coop_strongcheckpoints 1 // do not allow changelevel back if map has checkpoints. This preventing some STUPID players from breaking the game
set mp_coop_noangry 1 // prevent ally npc from angrying on bad players
```

## Entity tools

Mod has server-side entity tools

This allows making sandbox servers

To enable it, ensure `sv_enttools_enable` is set to `0` and `mp_enttools_enable` set to` 1`

### Availiable options and commands

```
mp_enttools_enable (1/0) - enable entity tools
mp_enttools_maxfire - limit entity count for ent_fire
mp_enttools_lockmapentities - lock map's own entities from ent_fire
mp_enttools_checkowner owner checking
1: allow ent_fire on entity only if player owns it or disconnected from server
2: allow ent_fire on entity only if player owns it
mp_enttools_players - allow ent_fire on players (dangerous)
mp_enttools_addblacklist <pattern> <per minute limit> <behaviour (0 - block, 1 - kick, 2 - ban)> [<clear>]
rate-limiting and blacklist for ent_create
if per-minute-limit set to 0, refuses to create entity
if per-minute-limit reached, does action set in behaviour
if clear is set, clears all entities created by this player
mp_enttools_clearblacklist - clear blacklist.
Add before addblacklist commands to config to prevent duplicates in blacklist
mp_enttools_checkmodels - check model name before setting model
Useful if you do not want to use error.mdl
```

## Menu command system

GravGumMod has advanced command processing

Command may be entered in console, in chat with `'/'` prefix and from menu

Every command may be associated to menu, custom motd message and short help message
menus placed in `ggm/menus` folder in gamedir

Each menu is plain text file without extension and formmated this way:
```
"Menu title"
"Item name1" "command1"
"Item name2" "command2"
```
Up to 5 items allowed

Each custom motd placed in `ggm/motd` as `plain text`

Each help message placed in `ggm/help` as `plain text`

You may send commands to client with `"client"` command

When client connects to server, it executes` "init" `command

### Touch settings:

```
mp_touchname, mp_touchcommand - add button with specified name
and command to touch controls on connect
mp_touchmenu - upload touch menu to client.
Will not be saved after exit. Will replace all text menus
```


## Other options


server visibility distance limit:
```
mp_serverdistclip - general switch
mp_maxbmodeldist - max distance for bmodels
mp_maxtrashdist - max distance for small monsters and gibs
mp_maxwaterdist - max distance for water
mp_maxmonsterdist - max distance for monsters
mp_maxotherdist - max distance for other entities
mp_servercliptents - switch for tempentity clipping
mp_maxtentdist - tempentity distance Cuts shoots, beams, ligths
```
weapon switches:

```
mp_allow_gravgun
mp_allow_ar2
mp_allow_bigcock
mp_allow_gateofbabylon
0 - disable, 1 - enable (precache), 2 - give to player
```
tweaks:
```
mp_ar2_mp5 - make ar2 use mp5 ammo
mp_ar2_bullets - default ar2 bullets count
mp_ar2_balls - default ar2 balls count
mp_wresptime - weapon respawn time
mp_iresptime - item respawn time
mp_hgibcount - human gibs count
mp_agibcount - alien gibs count
mp_gibtime - set gib stay time
mp_fixhornetbug - enable workaround for hornetgun bug
mp_fixsavetime - cut down time when loading
save (useful for very LOOOOONG game and sandbox
servers with save)
mp_checkentities - check for entity overflow. preventing crash on edicts overflow.
This enables garbage collector
mp_maxdecals - set r_decals from server
mp_errormdl, mp_errormdlpath - replace bad models with error.mdl
mp_lightstyle <number> <pattern> - override lightstyle from server.
For example, mp_lightstyle 0 k makes static light darker.
ent_rungc <mode> [<pattern>] - execute garbage collector manually
0 - default garbage collector
1 - remove all entities created by enttools (freshen map)
2 - remove all entities by given pattern
```
