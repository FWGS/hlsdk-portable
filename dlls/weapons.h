/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#pragma once
#if !defined(WEAPONS_H)
#define WEAPONS_H

#include "effects.h"

class CBasePlayer;
extern int gmsgWeapPickup;
extern int gmsgExplosion;
extern int gmsgWorldExp;
extern int gmsgImpBullet;
extern int gmsgImpRocket;
extern int gmsgImpBeam;
extern int gmsgFireGun;
extern int gmsgFireBeam;
extern int gmsgBrassClip;
extern int gmsgPlrGib;
extern int gmsgBreakGib;
extern int gmsgTrail;
extern int gmsgSpecTank;

//Weapon fire effects
#define FIREGUN_REMOVE		0
#define FIREGUN_GLOCK		1
#define FIREGUN_GLOCKAKIMBO	2
#define FIREGUN_USP		3
#define FIREGUN_DEAGLE		4
#define FIREGUN_PYTHON		5
#define FIREGUN_UZI		6
#define FIREGUN_UZIAKIMBO	7
#define FIREGUN_SHIELDGUN	8
#define FIREGUN_SHOTGUN		9
#define FIREGUN_SHOTGUN_BR_SHELL 10
#define FIREGUN_AUTOSHOTGUN	11
#define FIREGUN_30MMSG		12
#define FIREGUN_MP5		13
#define FIREGUN_M16		14
#define FIREGUN_AKIMBOGUN_LEFT	15
#define FIREGUN_AKIMBOGUN_RIGHT	16
#define FIREGUN_AKIMBOGUN_BOTH	17
#define FIREGUN_AK74		18
#define FIREGUN_G11		19
#define FIREGUN_U2		20
#define FIREGUN_SVD		21
#define FIREGUN_AWP		22
#define FIREGUN_TORCH		23
#define FIREGUN_M249		24
#define FIREGUN_MINIGUN		25
#define FIREGUN_BFG		26
#define FIREGUN_BARETT		27
#define FIREGUN_MACHINEGUN	28
#define FIREGUN_FLAKCANNON	29
#define FIREGUN_PLASMARIFLE	30
#define FIREGUN_FROSTER		31
#define FIREGUN_EPISTOL		32
#define FIREGUN_DEVASTATOR	33
#define FIREGUN_RPG		34
#define FIREGUN_GLUONGUN	35
#define FIREGUN_DISPLACER	36
#define FIREGUN_SMARTGUN	37
#define FIREGUN_INCENDIARY	38
#define FIREGUN_IONTURRET	39
#define FIREGUN_M72		40
#define FIREGUN_BANDSAW		41
#define FIREGUN_CROSSBOW	42
#define FIREGUN_NAILGUN		43
#define FIREGUN_TESLAGUN	44
#define FIREGUN_BLASTER		45
#define FIREGUN_GAUSS		46
#define FIREGUN_PHOTONGUN	47
#define FIREGUN_PHOTONGUN_EXP	48
#define FIREGUN_PULSERIFLE	49
#define FIREGUN_TAUCANNON	50
#define FIREGUN_TURRETKIT	51
#define FIREGUN_CHRONOSCEPTOR	52
#define FIREGUN_LIGHTSABER	53
#define FIREGUN_FTHROWER	54
#define FIREGUN_BFG_SEC		55
#define FIREGUN_EGON		56

//Beam impact effects
#define IMPBEAM_BLASTER		1
#define IMPBEAM_GAUSS		2
#define IMPBEAM_GAUSSCHARGED	3
#define IMPBEAM_PHOTONGUN	4
#define IMPBEAM_TAUCANNON	5
#define IMPBEAM_IONTURRET	6
#define IMPBEAM_BLASTERBEAM	7
#define IMPBEAM_EGON		8
#define IMPBEAM_PLASMABALL	9
#define IMPBEAM_PBOLT		10
#define IMPBEAM_EGON2		11
#define IMPBEAM_TAUCANNON2	12
#define IMPBEAM_HELLVORTB	13

//Impact surfaces
#define SURFACE_NONE		0
#define SURFACE_WORLDBRUSH	1
#define SURFACE_BREAKABLE	2
#define SURFACE_ARMOR		3
#define SURFACE_ENERGYARMOR	4
#define SURFACE_FLESH		5
#define SURFACE_FLESH_YELLOW		6

//Weapon beam effects
#define BEAM_BLASTER		1
#define BEAM_BLASTER_EXIT	2
#define BEAM_GAUSS		3
#define BEAM_GAUSS_EXIT		4
#define BEAM_GAUSSCHARGED	5
#define BEAM_GAUSSCHARGED_EXIT	6
#define BEAM_PHOTONGUN		7
#define BEAM_PHOTONGUN_EXIT	8
#define BEAM_TAUCANNON		9
#define BEAM_TAUCANNON_EXIT	10
#define BEAM_IONTURRET		11
#define BEAM_IONTURRET_EXIT	12
#define BEAM_IONTURRET_EXIT	12
#define BEAM_PHOTONGUN_EXP	13
#define BEAM_TESLAGUN		14
#define BEAM_PULSERIFLE		15
#define BEAM_PSP		16
#define BEAM_LGTNGBALL		17
#define BEAM_PLASMABALL		18
#define BEAM_BLOODSTREAM	19
#define BEAM_STONEBEAM		20
#define BEAM_NOBITABEAM		21
#define BEAM_TAUCANNON2		22

//Projectile trail and explosion effects
#define PROJ_REMOVE			0
#define PROJ_FLAME			1
#define PROJ_FLAME_DETONATE 		2
#define PROJ_FLAME_DETONATE_WATER	3
#define PROJ_ICE			4
#define PROJ_ICE_DETONATE 		5
#define PROJ_ICE_DETONATE_WATER		6
#define PROJ_NERVEGREN			7
#define PROJ_NERVEGREN_DETONATE 	8
#define PROJ_INCENDIARY2		9
#define PROJ_30MMGREN			10
#define PROJ_30MMGREN_DETONATE	 	11
#define PROJ_30MMGREN_DETONATE_WATER	12
#define PROJ_AK74			13
#define PROJ_AK74_DETONATE 		14
#define PROJ_AK74_DETONATE_WATER	15
#define PROJ_M203			16
#define PROJ_M203_DETONATE 		17
#define PROJ_M203_DETONATE_WATER	18
#define PROJ_MMISSILE			19
#define PROJ_MMISSILE_DETONATE 		20
#define PROJ_MMISSILE_DETONATE_WATER	21
#define PROJ_WARHEAD			22
#define PROJ_WARHEAD_DETONATE 		23
#define PROJ_WARHEAD_DETONATE_WATER	24
#define PROJ_DUMBFIRE			25
#define PROJ_DUMBFIRE_DETONATE 		26
#define PROJ_DUMBFIRE_DETONATE_WATER	27
#define PROJ_NUKE			28
#define PROJ_NUKE_DETONATE	 	29
#define PROJ_NUKE_DETONATE_WATER	30
#define PROJ_U2				31
#define PROJ_U2_DETONATE 		32
#define PROJ_U2_DETONATE_WATER		33
#define PROJ_U2_DETONATE_SHARDS		34
#define PROJ_U2_SHARD			35
#define PROJ_U2_SHARD_DETONATE 		36
#define PROJ_U2_SHARD_DETONATE_WATER	37
#define PROJ_FLAKBOMB			38
#define PROJ_FLAKBOMB_DETONATE 		39
#define PROJ_FLAKBOMB_DETONATE_WATER	40
#define PROJ_RPGROCKET			41
#define PROJ_RPGROCKET_DETONATE 	42
#define PROJ_RPGROCKET_DETONATE_WATER	43
#define PROJ_INCENDIARY			44
#define PROJ_INCENDIARY_DETONATE 	45
#define PROJ_INCENDIARY_DETONATE_WATER	46
#define PROJ_TESLAGREN			47
#define PROJ_TESLAGREN_DETONATE 	48
#define PROJ_TESLAGREN_DETONATE_WATER	49
#define PROJ_CLUSTERBOMB		50
#define PROJ_CLUSTERBOMB_DETONATE 	51
#define PROJ_CLUSTERBOMB_DETONATE_WATER	52
#define PROJ_CLUSTERBABY_DETONATE 	53
#define PROJ_CLUSTERBABY_DETONATE_WATER	54
#define PROJ_GLUON			55
#define PROJ_GLUON_DETONATE 		56
#define PROJ_GLUON_DETONATE_WATER	57
#define PROJ_GLUON2			58
#define PROJ_GLUON2_DETONATE 		59
#define PROJ_PLASMA	 		60
#define PROJ_DISPLACER			61
#define PROJ_DISPLACER_DETONATE 	62
#define PROJ_DISPLACER_DETONATE_WATER	63
#define PROJ_SHOCK			64
#define PROJ_SHOCK_DETONATE 		65
#define PROJ_SHOCK_DETONATE_WATER	66
#define PROJ_SUNOFGOD			67
#define PROJ_SUNOFGOD_DETONATE		68
#define PROJ_SUNOFGOD_DETONATE_WATER	69
#define PROJ_BLACKHOLE			70
#define PROJ_BLACKHOLE_DETONATE		71
#define PROJ_TELEENTER			72
#define PROJ_TELEENTER_DETONATE 	73
#define PROJ_TELEENTER_DETONATE_WATER	74
#define PROJ_DISPPOWER_DETONATE 	75
#define PROJ_SHOCKPOWER_DETONATE 	76
#define PROJ_GUTS			77
#define PROJ_GUTS_DETONATE		78
#define PROJ_ENERGYCHARGE_DETONATE 	79
#define PROJ_ENERGYCHARGE_DETONATE_WATER	80
#define EFFECT_BURN			81
#define EFFECT_FREEZE			82
#define PROJ_SHOCK2				83
#define PROJ_SUNOFGOD2			134

// explosions
#define EXPLOSION_C4		1
#define EXPLOSION_BIOMASS	2 
#define EXPLOSION_TURRET	3
#define EXPLOSION_PSHIELD	4 
#define EXPLOSION_TRIPMINE	5 
#define EXPLOSION_GRENADE	6 
#define EXPLOSION_FLASHBANG	7 
#define EXPLOSION_CHRONOCLIP	8 
#define EXPLOSION_BOLT		9
#define EXPLOSION_SATCHEL	10
#define EXPLOSION_TANKPROJ	11
#define EXPLOSION_SPARKSHOWER	12
#define EXPLOSION_SHIELDIMPACT	13
#define EXPLOSION_ARMORIMPACT	14
#define EXPLOSION_BUBBLES	15
#define EXPLOSION_WHITESMOKE	16
#define EXPLOSION_BIOMASSIMPACT	17 
#define EXPLOSION_TANK		18
#define EXPLOSION_BLOODDRIPS	19 
#define EXPLOSION_SATELLITE	20
#define EXPLOSION_MORTAR	21
#define EXPLOSION_TORCH		22
#define EXPLOSION_PTELEPORT	23
#define EXPLOSION_DISPTELEPORT	24
#define EXPLOSION_PLASMABALL2	25
#define EXPLOSION_PLASMABALL2_SPARKS	26
#define EXPLOSION_ENERGY_INWATER_S	27
#define EXPLOSION_ENERGY_INWATER_M	28
#define EXPLOSION_ENERGY_INWATER_L	29
#define EXPLOSION_PBOLT			30
#define EXPLOSION_WHL_SHARD		31
#define EXPLOSION_SPARKSHOWER_SND	32
#define EXPLOSION_LIGHTSABER		33
#define EXPLOSION_LAVA			34
#define EXPLOSION_LAVA_FLAME		35
#define EXPLOSION_CLUSTERBABY		36
#define EXPLOSION_MEDKIT		37
#define EXPLOSION_HEVCHARGER		38
#define EXPLOSION_BEAM_FLESHIMPACT	39
#define EXPLOSION_TURRET_SPAWN		40
#define EXPLOSION_MACHINEGUN		41

void DeactivateSatchels( CBasePlayer *pOwner );

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	void Spawn( void );

	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;

	static CGrenade *Shoot_Flashbang( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade *ShootTimed( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade *ShootContact( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade *ShootSatchelCharge( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	static void UseSatchelCharges( entvars_t *pevOwner, SATCHELCODE code );

	static CGrenade *ShootTimed_darkhole( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );

	static CGrenade *ShootTimed_low( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade *ShootContact_low( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );

	static CGrenade *ShootContact_height( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );


	void Explode( Vector vecSrc, Vector vecAim );
	virtual void Explode( TraceResult *pTrace, int bitsDamageType );
	void Explode_Flashbang( TraceResult *pTrace, int bitsDamageType );
	void EXPORT Smoke( void );

	void EXPORT BounceTouch( CBaseEntity *pOther );
	void EXPORT SlideTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );
	void EXPORT ExplodeTouch2( CBaseEntity *pOther );
	void EXPORT DangerSoundThink( void );
	void EXPORT PreDetonate( void );
	void EXPORT Detonate( void );
	void EXPORT DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink( void );

	virtual void BounceSound( void );
	virtual int	BloodColor( void ) { return DONT_BLEED; }
	virtual void Killed( entvars_t *pevAttacker, int iGib );

	BOOL m_fRegisteredSound;// whether or not this grenade has issued its DANGER sound to the world sound list yet.
};

// constant items
#define ITEM_HEALTHKIT		1
#define ITEM_ANTIDOTE		2
#define ITEM_SECURITY		3
#define ITEM_BATTERY		4

#define WEAPON_NONE				0
#define WEAPON_CROWBAR			1
#define	WEAPON_GLOCK			2
#define WEAPON_PYTHON			3
#define WEAPON_MP5				4
#define WEAPON_DEAGLE			5
#define WEAPON_CROSSBOW			6
#define WEAPON_SHOTGUN			7
#define WEAPON_RPG				8
#define WEAPON_GAUSS			9
#define WEAPON_EGON				10
#define WEAPON_HORNETGUN		11
#define WEAPON_HANDGRENADE		12
#define WEAPON_TRIPMINE			13
#define	WEAPON_SATCHEL			14
#define	WEAPON_SNARK			15

#define WEAPON_M40A1			16
#define WEAPON_DISPLACER		17
#define WEAPON_FIREAXE			18
#define WEAPON_AK47				19
#define WEAPON_FIST				20
#define WEAPON_HAMMER			21
#define WEAPON_DARKGRENADE		22
#define WEAPON_DUELUZI			23
#define WEAPON_KMEDKIT			24
#define WEAPON_SMG				25
#define WEAPON_SG550			26
#define WEAPON_M134				27
#define WEAPON_AIRGUN			28
#define WEAPON_DUALDBARREL		29
#define WEAPON_REDEEMER			30

#define WEAPON_ALLWEAPONS		(~(1<<WEAPON_SUIT))

#define WEAPON_SUIT				31	// ?????

#define MAX_NORMAL_BATTERY	100

// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT		0
#define GLOCK_WEIGHT		10
#define PYTHON_WEIGHT		15
#define MP5_WEIGHT			15
#define SHOTGUN_WEIGHT		15
#define CROSSBOW_WEIGHT		10
#define RPG_WEIGHT			20
#define GAUSS_WEIGHT		20
#define EGON_WEIGHT			20
#define HORNETGUN_WEIGHT	15
#define HANDGRENADE_WEIGHT	5
#define SNARK_WEIGHT		5
#define SATCHEL_WEIGHT		-10
#define TRIPMINE_WEIGHT		-10

#define KNIFE_WEIGHT		1
#define SMG_WEIGHT			10
#define AK47_WEIGHT			15
#define SG550_WEIGHT		11
#define M134_WEIGHT			20

// weapon clip/carry ammo capacities
#define URANIUM_MAX_CARRY		120
#define	_9MM_MAX_CARRY			240
#define _357_MAX_CARRY			42
#define BUCKSHOT_MAX_CARRY		120
#define BOLT_MAX_CARRY			50
#define ROCKET_MAX_CARRY		9
#define HANDGRENADE_MAX_CARRY	10
#define SATCHEL_MAX_CARRY		5
#define TRIPMINE_MAX_CARRY		5
#define SNARK_MAX_CARRY			15
#define HORNET_MAX_CARRY		8
#define M203_GRENADE_MAX_CARRY	10

#define AK_MAX_CARRY				180
#define M16_MAX_CARRY				180
#define MAC_MAX_CARRY				300
#define MINIGUN_MAX_CARRY			300
#define SNIPER_MAX_CARRY			30
#define MEDKIT_MAX_CARRY			5
#define DARKGRENADE_MAX_CARRY		3

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1

//#define CROWBAR_MAX_CLIP		WEAPON_NOCLIP
#define GLOCK_MAX_CLIP			17
#define PYTHON_MAX_CLIP			6
#define MP5_MAX_CLIP			50
#define SHOTGUN_MAX_CLIP		8
#define CROSSBOW_MAX_CLIP		5
#define RPG_MAX_CLIP			1
#define GAUSS_MAX_CLIP			WEAPON_NOCLIP
#define EGON_MAX_CLIP			WEAPON_NOCLIP
#define HORNETGUN_MAX_CLIP		WEAPON_NOCLIP
#define HANDGRENADE_MAX_CLIP	WEAPON_NOCLIP
#define SATCHEL_MAX_CLIP		WEAPON_NOCLIP
#define TRIPMINE_MAX_CLIP		WEAPON_NOCLIP
#define SNARK_MAX_CLIP			WEAPON_NOCLIP

// the default amount of ammo that comes with each gun when it spawns
#define GLOCK_DEFAULT_GIVE			17
#define PYTHON_DEFAULT_GIVE			6
#define MP5_DEFAULT_GIVE			25
#define MP5_DEFAULT_GIVE_MP			MP5_MAX_CLIP
#define MP5_M203_DEFAULT_GIVE		2
#define SHOTGUN_DEFAULT_GIVE		12
#define CROSSBOW_DEFAULT_GIVE		5
#define RPG_DEFAULT_GIVE			1
#define GAUSS_DEFAULT_GIVE			30
#define EGON_DEFAULT_GIVE			30
#define HANDGRENADE_DEFAULT_GIVE	2
#define SATCHEL_DEFAULT_GIVE		1
#define TRIPMINE_DEFAULT_GIVE		1
#define SNARK_DEFAULT_GIVE			5
#define HIVEHAND_DEFAULT_GIVE		10

// The amount of ammo given to a player by an ammo item.
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_GLOCKCLIP_GIVE		GLOCK_MAX_CLIP
#define AMMO_357BOX_GIVE		PYTHON_MAX_CLIP
#define AMMO_MP5CLIP_GIVE		MP5_MAX_CLIP
#define AMMO_CHAINBOX_GIVE		200
#define AMMO_M203BOX_GIVE		2
#define AMMO_BUCKSHOTBOX_GIVE	12
#define AMMO_CROSSBOWCLIP_GIVE	CROSSBOW_MAX_CLIP
#define AMMO_RPGCLIP_GIVE		RPG_MAX_CLIP
#define AMMO_URANIUMBOX_GIVE	20
#define AMMO_SNARKBOX_GIVE		5

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,

	BULLET_CROWBAR,
	BULLET_338Magnum,
	BULLET_357,
	BULLET_762Nato,
	BULLET_556Nato,
	BULLET_556,
	BULLET_762,
	BULLET_50AE,
	BULLET_BUCKSHOT,
	BULLET_MP5,
	BULLET_30mm,
	BULLET_8mm,
	BULLET_57mm,
	BULLET_9MM,
	BULLET_12MM,
	BULLET_14MM,
	BULLET_GAUSS_HEV,
	BULLET_IONTURRET,
	BULLET_DOMA_LASER,
	BULLET_GMAN_LASER,
} Bullet;

#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_NOAUTOSWITCHTO	32

#define WEAPON_IS_ONTARGET 0x40

typedef struct
{
	int		iSlot;
	int		iPosition;
	const char	*pszAmmo1;	// ammo 1 type
	int		iMaxAmmo1;		// max ammo 1
	const char	*pszAmmo2;	// ammo 2 type
	int		iMaxAmmo2;		// max ammo 2
	const char	*pszName;
	int		iMaxClip;
	int		iId;
	int		iFlags;
	int		iWeight;// this value used to determine this weapon's importance in autoselection.
} ItemInfo;

typedef struct
{
	const char *pszName;
	int iId;
} AmmoInfo;

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
public:
	virtual void SetObjectCollisionBox( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	virtual int AddToPlayer( CBasePlayer *pPlayer );	// return TRUE if the item you want the item added to the player inventory
	virtual int AddDuplicate( CBasePlayerItem *pItem ) { return FALSE; }	// return TRUE if you want your duplicate removed from world
	void EXPORT DestroyItem( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther );	// default weapon touch
	void EXPORT FallThink ( void );// when an item is first spawned, this think is run to determine when the object has hit the ground.
	void EXPORT Materialize( void );// make a weapon visible and tangible
	void EXPORT AttemptToMaterialize( void );  // the weapon desires to become visible and tangible, if the game rules allow for it
	CBaseEntity* Respawn ( void );// copy a weapon
	void FallInit( void );
	void CheckRespawn( void );
	virtual int GetItemInfo(ItemInfo *p) { return 0; };	// returns 0 if struct not filled out
	virtual BOOL CanDeploy( void ) { return TRUE; };
	virtual BOOL Deploy( )								// returns is deploy was successful
		 { return TRUE; };

	virtual BOOL CanHolster( void ) { return TRUE; };// can this weapon be put away right now?
	virtual void Holster( int skiplocal = 0 );
	virtual void UpdateItemInfo( void ) { return; };

	virtual void ItemPreFrame( void )	{ return; }		// called each frame by the player PreThink
	virtual void ItemPostFrame( void ) { return; }		// called each frame by the player PostThink

	virtual void Drop( void );
	virtual void Kill( void );
	virtual void AttachToPlayer ( CBasePlayer *pPlayer );

	virtual int PrimaryAmmoIndex() { return -1; };
	virtual int SecondaryAmmoIndex() { return -1; };

	virtual int UpdateClientData( CBasePlayer *pPlayer ) { return 0; }

	virtual CBasePlayerItem *GetWeaponPtr( void ) { return NULL; };

	static ItemInfo ItemInfoArray[ MAX_WEAPONS ];
	static AmmoInfo AmmoInfoArray[ MAX_AMMO_SLOTS ];

	CBasePlayer	*m_pPlayer;
	CBasePlayerItem *m_pNext;
	int		m_iId;												// WEAPON_???

	virtual int iItemSlot( void ) { return 0; }			// return 0 to MAX_ITEMS_SLOTS, used in hud

	int			iItemPosition( void ) { return ItemInfoArray[ m_iId ].iPosition; }
	const char	*pszAmmo1( void )	{ return ItemInfoArray[ m_iId ].pszAmmo1; }
	int			iMaxAmmo1( void )	{ return ItemInfoArray[ m_iId ].iMaxAmmo1; }
	const char	*pszAmmo2( void )	{ return ItemInfoArray[ m_iId ].pszAmmo2; }
	int			iMaxAmmo2( void )	{ return ItemInfoArray[ m_iId ].iMaxAmmo2; }
	const char	*pszName( void )	{ return ItemInfoArray[ m_iId ].pszName; }
	int			iMaxClip( void )	{ return ItemInfoArray[ m_iId ].iMaxClip; }
	int			iWeight( void )		{ return ItemInfoArray[ m_iId ].iWeight; }
	int			iFlags( void )		{ return ItemInfoArray[ m_iId ].iFlags; }

	// int		m_iIdPrimary;										// Unique Id for primary ammo
	// int		m_iIdSecondary;										// Unique Id for secondary ammo
};

// inventory items that 
class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );

	static	TYPEDESCRIPTION m_SaveData[];

	// generic weapon versions of CBasePlayerItem calls
	virtual int AddToPlayer( CBasePlayer *pPlayer );
	virtual int AddDuplicate( CBasePlayerItem *pItem );

	virtual int ExtractAmmo( CBasePlayerWeapon *pWeapon ); //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo( CBasePlayerWeapon *pWeapon );// { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up

	virtual int AddWeapon( void ) { ExtractAmmo( this ); return TRUE; };	// Return TRUE if you want to add yourself to the player

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo( int iCount, char *szName, int iMaxClip, int iMaxCarry );
	BOOL AddSecondaryAmmo( int iCount, char *szName, int iMaxCarry );

	virtual void UpdateItemInfo( void ) {};	// updates HUD state

	int m_iPlayEmptySound;
	int m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the
							// attack key(s)
	virtual BOOL PlayEmptySound( void );
	virtual void ResetEmptySound( void );

	virtual void SendWeaponAnim( int iAnim, int skiplocal = 1, int body = 0 );  // skiplocal is 1 if client is predicting weapon animations

	virtual BOOL CanDeploy( void );
	virtual BOOL IsUseable( void );
	BOOL DefaultDeploy( const char *szViewModel, const char *szWeaponModel, int iAnim, const char *szAnimExt, int skiplocal = 0, int body = 0 );
	int DefaultReload( int iClipSize, int iAnim, float fDelay, int body = 0 );

	virtual void ItemPostFrame( void );	// called each frame by the player PostThink
	// called by CBasePlayerWeapons ItemPostFrame()
	virtual void PrimaryAttack( void ) { return; }				// do "+ATTACK"
	virtual void SecondaryAttack( void ) { return; }			// do "+ATTACK2"
	virtual void Reload( void ) { return; }						// do "+RELOAD"
	virtual void WeaponIdle( void ) { return; }					// called when no buttons pressed
	virtual int UpdateClientData( CBasePlayer *pPlayer );		// sends hud info to client dll, if things have changed
	virtual void RetireWeapon( void );
	virtual BOOL ShouldWeaponIdle( void ) {return FALSE; };
	virtual void Holster( int skiplocal = 0 );
	virtual BOOL UseDecrement( void ) { return FALSE; };

	int	PrimaryAmmoIndex(); 
	int	SecondaryAmmoIndex(); 

	void PrintState( void );

	virtual CBasePlayerItem *GetWeaponPtr( void ) { return (CBasePlayerItem *)this; };
	float GetNextAttackDelay( float delay );

	float m_flPumpTime;
	int		m_fInSpecialReload;									// Are we in the middle of a reload for the shotguns
	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_fInReload;										// Are we in the middle of a reload;

	int		m_iDefaultAmmo;// how much ammo you get when you pick up this weapon as placed by a level designer.

	// hle time creep vars
	float	m_flPrevPrimaryAttack;
	float	m_flLastFireTime;

	void KickBack(float up_base, float lateral_base, float up_modifier, float lateral_modifier, float up_max, float lateral_max, int direction_change);
	bool m_bDelayFire;
	int m_iShotsFired;

	int m_fldisplacer;

	float m_flAccuracy;
	float m_flDecreaseShotsFired;
	int m_iDirection;
	int m_fldiffaclut_crossize;	
};

class CBasePlayerAmmo : public CBaseEntity
{
public:
	virtual void Spawn( void );
	void EXPORT DefaultTouch( CBaseEntity *pOther ); // default weapon touch
	virtual BOOL AddAmmo( CBaseEntity *pOther ) { return TRUE; };

	void SetObjectCollisionBox( void )
	{
		//!!!BUGBUG - fix the model!
		pev->absmin = pev->origin + Vector(-16, -16, 0);
		pev->absmax = pev->origin + Vector(16, 16, 16); 
	}

	CBaseEntity* Respawn( void );
	void EXPORT Materialize( void );
	void EXPORT FallThink ( void );
};

extern DLL_GLOBAL	short	g_sModelIndexLaser;// holds the index for the laser beam
extern DLL_GLOBAL	const char *g_pModelNameLaser;

extern DLL_GLOBAL	short	g_sModelIndexLaserDot;// holds the index for the laser beam dot
extern DLL_GLOBAL	short	g_sModelIndexFireball;// holds the index for the fireball
extern DLL_GLOBAL	short	g_sModelIndexSmoke;// holds the index for the smoke cloud
extern DLL_GLOBAL	short	g_sModelIndexWExplosion;// holds the index for the underwater explosion
extern DLL_GLOBAL	short	g_sModelIndexBubbles;// holds the index for the bubbles model
extern DLL_GLOBAL	short	g_sModelIndexBloodDrop;// holds the sprite index for blood drops
extern DLL_GLOBAL	short	g_sModelIndexBloodSpray;// holds the sprite index for blood spray (bigger)
extern DLL_GLOBAL	short	g_sModelIndexCteleport;// teleport spr
extern DLL_GLOBAL	short	g_sModelIndexFlareGlow;
extern DLL_GLOBAL	short	g_sModelIndexTrail;

extern void ClearMultiDamage(void);
extern void ApplyMultiDamage(entvars_t* pevInflictor, entvars_t* pevAttacker );
extern void AddMultiDamage( entvars_t *pevInflictor, CBaseEntity *pEntity, float flDamage, int bitsDamageType);

extern void DecalGunshot( TraceResult *pTrace, int iBulletType );
extern void SpawnBlood(Vector vecSpot, int bloodColor, float flDamage);
extern int DamageDecal( CBaseEntity *pEntity, int bitsDamageType );
extern void RadiusFlash(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage);
extern void RadiusDamage( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );
extern void RadiusDamage2( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );
extern void RadiusDamage3( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );
extern void RadiusDamage_limit( Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );

typedef struct 
{
	CBaseEntity		*pEntity;
	float			amount;
	int				type;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;

#define LOUD_GUN_VOLUME			1000
#define NORMAL_GUN_VOLUME		600
#define QUIET_GUN_VOLUME		200

#define	BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define	DIM_GUN_FLASH			128

#define BIG_EXPLOSION_VOLUME	2048
#define NORMAL_EXPLOSION_VOLUME	1024
#define SMALL_EXPLOSION_VOLUME	512

#define	WEAPON_ACTIVITY_VOLUME	64

#define VECTOR_CONE_1DEGREES	Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES	Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES	Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES	Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES	Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES	Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES	Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES	Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES	Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES	Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES	Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES	Vector( 0.17365, 0.17365, 0.17365 )

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo. 
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void Precache( void );
	void Spawn( void );
	void Touch( CBaseEntity *pOther );
	void KeyValue( KeyValueData *pkvd );
	BOOL IsEmpty( void );
	int  GiveAmmo( int iCount, const char *szName, int iMax, int *pIndex = NULL );
	void SetObjectCollisionBox( void );

public:
	void EXPORT Kill ( void );
	void EXPORT FallThink ( void );
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL HasWeapon( CBasePlayerItem *pCheckItem );
	BOOL PackWeapon( CBasePlayerItem *pWeapon );
	BOOL PackAmmo( int iszName, int iCount );

	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];// one slot for each 

	string_t m_rgiszAmmo[MAX_AMMO_SLOTS];// ammo names
	int	m_rgAmmo[MAX_AMMO_SLOTS];// ammo quantities

	int m_cAmmoTypes;// how many ammo types packed into this box (if packed by a level designer)
};

#if CLIENT_DLL
bool bIsMultiplayer ( void );
void LoadVModel ( const char *szViewModel, CBasePlayer *m_pPlayer );
#endif

class CGlock : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void GlockFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );

	BOOL TriggerReleased;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	int m_iShell;

	unsigned short m_usFireGlock1;
	unsigned short m_usFireGlock2;
};

class CHammer : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo(ItemInfo *p);

	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	int Swing( int fFirst );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	int m_iSwing;
	TraceResult m_trHit;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
private:
	unsigned short m_usHammer;
};

class CCrowbar : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo( ItemInfo *p );
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	int Swing( int fFirst );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
#if CROWBAR_IDLE_ANIM
	void WeaponIdle();
#endif
	int m_iSwing;
	TraceResult m_trHit;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
private:
	unsigned short m_usCrowbar;
};

class CFist : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo(ItemInfo *p);

	int AddToPlayer( CBasePlayer *pPlayer );

	int Swing2( int fFirst );
	void EXPORT SwingAgain2( void );
	void Reload( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int Swing( int fFirst );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	int m_iSwing;
	TraceResult m_trHit;

	bool m_bDelayFinger;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
private:
	unsigned short m_usCFist;
};


class CFireAxe : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	void EXPORT SwingAgain( void );
	void EXPORT Smack( void );
	int GetItemInfo(ItemInfo *p);
	void SecondaryAttack( void );
	void PrimaryAttack( void );
	int Swing( int fFirst );
	int Swing_big( int fFirst );
	void Reload( void );
	void WeaponIdle( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	int m_iSwing;
	TraceResult m_trHit;
	int AddToPlayer( CBasePlayer *pPlayer );
	virtual BOOL UseDecrement( void )
	{ 

#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
private:
	unsigned short m_usCFireAxe;
};



class CDisplacer : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);

	BOOL Deploy( void );

	void Holster( int skiplocal = 0 );

	void EXPORT Teleport( void );
	void EXPORT FireBall( void );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );
	void SwitchFireMode( void );

	int	m_iFireMode;
	int	m_iFireState;

	virtual int AddToPlayer( CBasePlayer *pPlayer );	// return TRUE if the item you want the item added to the player inventory

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

};

class CRedeemer : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	BOOL Deploy( void );

	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void WeaponIdle( void );

	virtual int AddToPlayer( CBasePlayer *pPlayer );	// return TRUE if the item you want the item added to the player inventory

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

};

class CPython : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	float m_flSoundDelay;

	BOOL TriggerReleased;

	BOOL m_fInZoom;// don't save this. 

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usFirePython;
};

class CSMG : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	void Holster( int skiplocal = 0 );
	float m_flNextAnimTime;
	int m_iShell;

	int m_deshoot;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usSMG;
};

class CMP5 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int SecondaryAmmoIndex( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

	int m_iChargeLevel;
	int m_deshoot;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usMP5;
	unsigned short m_usMP52;
};

class CDuelUzi : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 6; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;
	BOOL TriggerReleased;
	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usUZI;
};



class CM134 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 6; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void Holster( int skiplocal = 0 );
	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

	int m_minigunspin;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

};

class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );

	BOOL Deploy( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;
	int m_iShell;

	int m_deshoot;

	BOOL TriggerReleased;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usAK47;
};

class CSniper : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 6; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

	int m_fInZoom; // don't save this

	BOOL TriggerReleased;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usSniper;
};

class CCrossbow : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);

	void FireBolt( void );
	void FireSniperBolt( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

	int m_fInZoom; // don't save this

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usCrossbow;
	unsigned short m_usCrossbow2;
};

class CSg550 : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 6; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer( CBasePlayer *pPlayer );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );

	int m_fInZoom; // don't save this

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usSg550;
};

class CShotgun : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( );
	void Reload( void );
	void WeaponIdle( void );
	//void ItemPostFrame( void );
	int m_fInReload;
	float m_flNextReload;
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};

class CDualdbarrel : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 6; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	BOOL Deploy( );
	void Reload( void );
	void WeaponIdle( void );;
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usdualdbarrelFire;
};

class CLaserSpot : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );

	int	ObjectCaps( void ) { return FCAP_DONT_SAVE; }

public:
	void Suspend( float flSuspendTime );
	void EXPORT Revive( void );

	static CLaserSpot *CreateSpot( void );
};

class CRpg : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	void Reload( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void UpdateSpot( void );
	BOOL ShouldWeaponIdle( void ) { return TRUE; };

	CLaserSpot *m_pSpot;
	int m_fSpotActive;
	int m_cActiveRockets;// how many missiles in flight from this launcher right now?

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usRpg;
};

class CRpgRocket : public CGrenade
{
public:
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	void Spawn( void );
	void Precache( void );
	void EXPORT FollowThink( void );
	void EXPORT IgniteThink( void );
	void EXPORT RocketTouch( CBaseEntity *pOther );
	static CRpgRocket *CreateRpgRocket( Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, CRpg *pLauncher );

	int m_iTrail;
	float m_flIgniteTime;
	CRpg *m_pLauncher;// pointer back to the launcher that fired me. 
};


class CDeagle : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	float m_flSoundDelay;
    BOOL TriggerReleased;
	BOOL m_fInZoom;// don't save this. 

	CLaserSpot *m_pSpot;
	void UpdateSpot( void );
	int m_fSpotActive;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usFireDeagle;
	unsigned short m_usFireDeagle2;
};

class CGauss : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0  );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void StartFire( void );
	void Fire( Vector vecOrigSrc, Vector vecDirShooting, float flDamage );
	float GetFullChargeTime( void );
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;
	int m_iSoundState; // don't save this

	// was this weapon just fired primary or secondary?
	// we need to know so we can pick the right set of effects. 
	BOOL m_fPrimaryFire;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usGaussFire;
	unsigned short m_usGaussSpin;
};

class CEgon : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	// BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );

	void UpdateEffect( const Vector &startPoint, const Vector &endPoint, float timeBlend );

	void CreateEffect ( void );
	void DestroyEffect ( void );
	void EndAttack( void );
	//void Attack( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	float m_flAmmoUseTime;// since we use < 1 point of ammo per update, we subtract ammo on a timer.

	float GetPulseInterval( void );
	float GetDischargeInterval( void );

	void Fire( const Vector &vecOrigSrc, const Vector &vecDir );

	BOOL HasAmmo( void );

	void UseAmmo( int count );

	enum EGON_FIREMODE { FIRE_NARROW, FIRE_WIDE};

	CBeam				*m_pBeam;
	CBeam				*m_pNoise;
	CSprite				*m_pSprite;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

	// unsigned short m_usEgonStop;

private:
	float				m_shootTime;
	EGON_FIREMODE		m_fireMode;
	float				m_shakeTime;
	BOOL				m_deployed;

	unsigned short m_usEgonFire;
};

class CAirGun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;

	float m_flRechargeTime;
	
	int m_iFirePhase;// don't save me.

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
};


class CHgun : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	float m_flNextAnimTime;

	float m_flRechargeTime;

	int m_iFirePhase;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
private:
	unsigned short m_usHornetFire;
};

class Ckmedkit : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	float m_flNextAnimTime;

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
};

class CDarkGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	
	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
};

class CHandGrenade : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	BOOL Deploy( void );
	BOOL CanHolster( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
};

class CSatchel : public CBasePlayerWeapon
{
public:
#if !CLIENT_DLL
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
#endif
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddDuplicate( CBasePlayerItem *pOriginal );
	BOOL CanDeploy( void );
	BOOL Deploy( void );
	BOOL IsUseable( void );

	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	void Throw( void );

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}
};

class CTripmine : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);
	void SetObjectCollisionBox( void )
	{
		//!!!BUGBUG - fix the model!
		pev->absmin = pev->origin + Vector(-16, -16, -5);
		pev->absmax = pev->origin + Vector(16, 16, 28); 
	}

	void PrimaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );

	virtual BOOL UseDecrement( void )
	{ 
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usTripFire;
};

class CSqueak : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 5; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );
	void WeaponIdle( void );
	int m_fJustThrown;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usSnarkFire;
};
#endif // WEAPONS_H
