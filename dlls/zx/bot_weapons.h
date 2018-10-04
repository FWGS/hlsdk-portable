//
// HPB_bot - botman's High Ping Bastard bot
//
// (http://planethalflife.com/botman/)
//
// bot_weapons.h
//

#ifndef BOT_WEAPONS_H
#define BOT_WEAPONS_H

// weapon ID values for Valve's Half-Life Deathmatch
#define VALVE_WEAPON_CROWBAR       1
#define VALVE_WEAPON_GLOCK         2
#define VALVE_WEAPON_PYTHON        3
#define VALVE_WEAPON_MP5           4
#define VALVE_WEAPON_CHAINGUN      5
#define VALVE_WEAPON_CROSSBOW      6
#define VALVE_WEAPON_SHOTGUN       7
#define VALVE_WEAPON_RPG           8
#define VALVE_WEAPON_GAUSS         9
#define VALVE_WEAPON_EGON         10
#define VALVE_WEAPON_HORNETGUN    11
#define VALVE_WEAPON_HANDGRENADE  12
#define VALVE_WEAPON_TRIPMINE     13
#define VALVE_WEAPON_SATCHEL      14
#define VALVE_WEAPON_SNARK        15

#define VALVE_MAX_NORMAL_BATTERY   100
#define VALVE_HORNET_MAX_CARRY      8


// weapon ID values for Valve's Team Fortress Classic & 1.5
#define TF_WEAPON_UNKNOWN1       1
#define TF_WEAPON_UNKNOWN2       2
#define TF_WEAPON_MEDIKIT        3
#define TF_WEAPON_SPANNER        4
#define TF_WEAPON_AXE            5
#define TF_WEAPON_SNIPERRIFLE    6
#define TF_WEAPON_AUTORIFLE      7
#define TF_WEAPON_SHOTGUN        8
#define TF_WEAPON_SUPERSHOTGUN   9
#define TF_WEAPON_NAILGUN       10
#define TF_WEAPON_SUPERNAILGUN  11
#define TF_WEAPON_GL            12
#define TF_WEAPON_FLAMETHROWER  13
#define TF_WEAPON_RPG           14
#define TF_WEAPON_IC            15
#define TF_WEAPON_UNKNOWN16     16
#define TF_WEAPON_AC            17
#define TF_WEAPON_UNKNOWN18     18
#define TF_WEAPON_UNKNOWN19     19
#define TF_WEAPON_TRANQ         20
#define TF_WEAPON_RAILGUN       21
#define TF_WEAPON_PL            22
#define TF_WEAPON_KNIFE         23


// weapon ID values for Counter-Strike
#define CS_WEAPON_P228           1
#define CS_WEAPON_UNKNOWN2       2
#define CS_WEAPON_SCOUT          3
#define CS_WEAPON_HEGRENADE      4
#define CS_WEAPON_XM1014         5
#define CS_WEAPON_C4             6
#define CS_WEAPON_MAC10          7
#define CS_WEAPON_AUG            8
#define CS_WEAPON_SMOKEGRENADE   9
#define CS_WEAPON_ELITE         10
#define CS_WEAPON_FIVESEVEN     11
#define CS_WEAPON_UMP45         12
#define CS_WEAPON_SG550         13
#define CS_WEAPON_UNKNOWN14     14
#define CS_WEAPON_UNKNOWN15     15
#define CS_WEAPON_USP           16
#define CS_WEAPON_GLOCK18       17
#define CS_WEAPON_AWP           18
#define CS_WEAPON_MP5NAVY       19
#define CS_WEAPON_M249          20
#define CS_WEAPON_M3            21
#define CS_WEAPON_M4A1          22
#define CS_WEAPON_TMP           23
#define CS_WEAPON_G3SG1         24
#define CS_WEAPON_FLASHBANG     25
#define CS_WEAPON_DEAGLE        26
#define CS_WEAPON_SG552         27
#define CS_WEAPON_AK47          28
#define CS_WEAPON_KNIFE         29
#define CS_WEAPON_P90           30


// weapon ID values for Gearbox's OpFor Deathmatch
#define GEARBOX_WEAPON_CROWBAR       1
#define GEARBOX_WEAPON_GLOCK         2
#define GEARBOX_WEAPON_PYTHON        3
#define GEARBOX_WEAPON_MP5           4
#define GEARBOX_WEAPON_CHAINGUN      5
#define GEARBOX_WEAPON_CROSSBOW      6
#define GEARBOX_WEAPON_SHOTGUN       7
#define GEARBOX_WEAPON_RPG           8
#define GEARBOX_WEAPON_GAUSS         9
#define GEARBOX_WEAPON_EGON         10
#define GEARBOX_WEAPON_HORNETGUN    11
#define GEARBOX_WEAPON_HANDGRENADE  12
#define GEARBOX_WEAPON_TRIPMINE     13
#define GEARBOX_WEAPON_SATCHEL      14
#define GEARBOX_WEAPON_SNARK        15
#define GEARBOX_WEAPON_GRAPPLE      16
#define GEARBOX_WEAPON_EAGLE        17
#define GEARBOX_WEAPON_PIPEWRENCH   18
#define GEARBOX_WEAPON_M249         19
#define GEARBOX_WEAPON_DISPLACER    20
#define GEARBOX_WEAPON_UNKNOWN21    21
#define GEARBOX_WEAPON_SHOCKRIFLE   22
#define GEARBOX_WEAPON_SPORELAUNCHER 23
#define GEARBOX_WEAPON_SNIPERRIFLE  24
#define GEARBOX_WEAPON_KNIFE        25


// weapon ID values for FrontLineForce
#define FLF_WEAPON_SAKO           9
#define FLF_WEAPON_AK5           10
#define FLF_WEAPON_RS202M2       11
#define FLF_WEAPON_UNKNOWN12     12
#define FLF_WEAPON_UNKNOWN13     13
#define FLF_WEAPON_UNKNOWN14     14
#define FLF_WEAPON_UNKNOWN15     15
#define FLF_WEAPON_MP5SD         16
#define FLF_WEAPON_M4            17
#define FLF_WEAPON_FLASHBANG     18
#define FLF_WEAPON_HEGRENADE     19
#define FLF_WEAPON_MP5A2         20
#define FLF_WEAPON_UMP45         21
#define FLF_WEAPON_SPAS12        22
#define FLF_WEAPON_BERETTA       23
#define FLF_WEAPON_KNIFE         24
#define FLF_WEAPON_MAC10         25
#define FLF_WEAPON_MK23          26
#define FLF_WEAPON_MSG90         27
#define FLF_WEAPON_FAMAS         28
#define FLF_WEAPON_HK21          29

// weapon ID values for HolyWars
#define HW_WEAPON_JACKHAMMER     15
#define HW_WEAPON_DOUBLESHOTGUN  16
#define HW_WEAPON_MACHINEGUN     17
#define HW_WEAPON_ROCKETLAUNCHER 18
#define HW_WEAPON_UNKNOWN19      19
#define HW_WEAPON_RAILGUN        20

// weapon ID values for Valve's DMC
#define DMC_WEAPON_AXE            1
#define DMC_WEAPON_SHOTGUN        2
#define DMC_WEAPON_DOUBLESHOTGUN  4
#define DMC_WEAPON_NAILGUN        8
#define DMC_WEAPON_SUPERNAIL     16
#define DMC_WEAPON_GRENADE1      32
#define DMC_WEAPON_ROCKET1       64
#define DMC_WEAPON_LIGHTNING    128


typedef struct
{
   char szClassname[64];
   int  iAmmo1;     // ammo index for primary ammo
   int  iAmmo1Max;  // max primary ammo
   int  iAmmo2;     // ammo index for secondary ammo
   int  iAmmo2Max;  // max secondary ammo
   int  iSlot;      // HUD slot (0 based)
   int  iPosition;  // slot position
   int  iId;        // weapon ID
   int  iFlags;     // flags???
} bot_weapon_t;


#endif // BOT_WEAPONS_H

