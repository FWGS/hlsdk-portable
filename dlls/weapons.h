/*** ****/

#ifndef WEAPONS_H
#define WEAPONS_H

class CBasePlayer;
class CBeam;
class CSprite;

void RadiusFlash( Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType );
void RadiusDamage( Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );

#ifndef MAX_WEAPONS
#define MAX_WEAPONS		64
#endif
#ifndef MAX_AMMO_SLOTS
#define MAX_AMMO_SLOTS	32
#endif
#ifndef MAX_ITEM_TYPES
#define MAX_ITEM_TYPES	7
#endif

#define WEAPON_NONE		0


// constant items
#define ITEM_HEALTHKIT	1
#define ITEM_ANTIDOTE		2
#define ITEM_SECURITY		3
#define ITEM_BATTERY		4

#define WEAPON_P228		1
#define WEAPON_SCOUT		2
#define WEAPON_XM1014		3
#define WEAPON_MAC10		4
#define WEAPON_AUG		5
#define WEAPON_ELITE		6
#define WEAPON_FIVESEVEN	7
#define WEAPON_UMP45		8
#define WEAPON_SG550		9
#define WEAPON_USP		10
#define WEAPON_GLOCK18	11
#define WEAPON_MP5N		12
#define WEAPON_AWP		13
#define WEAPON_M249		14
#define WEAPON_M3			15
#define WEAPON_M4A1		16
#define WEAPON_TMP		17
#define WEAPON_G3SG1		18
#define WEAPON_DEAGLE		19
#define WEAPON_SG552		20
#define WEAPON_AK47		21
#define WEAPON_P90		22
#define WEAPON_M60		23
#define WEAPON_FAMAS		24
#define WEAPON_GALIL		25
#define WEAPON_SHIELDGUN	26
#define WEAPON_SUIT		31	// ?????

#define ACCESSORY_FLASHBANG	4
#define ACCESSORY_HEGRENADE	5
#define ACCESSORY_C4			6
#define ACCESSORY_SMOKEGRENADE	7
#define ACCESSORY_KNIFE		9
#define ACCESSORY_RADIO		11
#define ACCESSORY_HOLSTER		17
#define ACCESSORY_CAMERA		18
#define ACCESSORY_RADIOCONTROLLEDBOMB	20
#define ACCESSORY_LAWS		21
#define ACCESSORY_BRIEFCASE	22
#define ACCESSORY_MEDKIT		23
#define ACCESSORY_SYRINGE		24
#define ACCESSORY_ZIPLINE		25
#define ACCESSORY_FIBEROPTICCAMERA	26
#define ACCESSORY_BLOWTORCH	27

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1
#define WEAPON_IS_ONTARGET	0x40
#define WPNSTATE_USP_SILENCED		1
#define WPNSTATE_GLOCK18_BURST_MODE	2
#define WPNSTATE_M4A1_SILENCED		4
#define WPNSTATE_ELITE_LEFT		8
#define WPNSTATE_FAMAS_BURST_MODE	16
#define WPNSTATE_SHIELD_DRAWN		32
#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_NOAUTOSWITCHTO		32

#define LOUD_GUN_VOLUME		1500
#define NORMAL_GUN_VOLUME	600
#define QUIET_GUN_VOLUME		200
#define BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define DIM_GUN_FLASH		128
#define BIG_EXPLOSION_VOLUME		2048
#define NORMAL_EXPLOSION_VOLUME	1024
#define SMALL_EXPLOSION_VOLUME	512
#define WEAPON_ACTIVITY_VOLUME	64

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

// bullet types
typedef enum
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
	BULLET_PLAYER_45ACP,
	BULLET_PLAYER_338MAG,
	BULLET_PLAYER_762MM,
	BULLET_PLAYER_556MM,
	BULLET_PLAYER_50AE,
	BULLET_PLAYER_57MM,
	BULLET_PLAYER_357SIG,
	BULLET_PLAYER_BLOWTORCH,
} Bullet;

typedef struct
{
	int			iSlot;
	int			iPosition;
	const char*	pszAmmo1;	// ammo 1 type
	int			iMaxAmmo1;		// max ammo 1
	const char*	pszAmmo2;	// ammo 2 type
	int			iMaxAmmo2;		// max ammo 2
	const char*	pszName;
	int			iMaxClip;
	int			iId;
	int			iFlags;
	int			iWeight; // this value used to determine this weapon's importance in autoselection.
	BOOL		bIsAccessory;
} ItemInfo;

typedef struct
{
	const char*	pszName;
	int			iId;
} AmmoInfo;

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	void Spawn() override;
	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;
	static CGrenade* ShootTimed( entvars_t* owner, Vector start, Vector velocity, float time,
		float noise, int classtype );
	static CGrenade* ShootTimed2( entvars_t* owner, Vector start, Vector velocity, float time,
		int team, unsigned short event );
	static CGrenade* ShootContact( entvars_t* owner, Vector start, Vector velocity );
	static CGrenade* ShootSmokeGrenade( entvars_t* owner, Vector start, Vector velocity, float time,
		unsigned short event );
	static CGrenade* ShootSatchelCharge( entvars_t* owner, Vector start, Vector velocity );
	static void UseSatchelCharges( entvars_t* owner, SATCHELCODE code );
	void Explode( Vector source, Vector aim );
	void Explode( TraceResult* trace, int damageType );
	void Explode2( TraceResult* trace, int damageType );
	void Explode3( TraceResult* trace, int damageType );
	void SG_Explode( TraceResult* trace, int damageType );
	void EXPORT Smoke();
	void EXPORT Smoke2();
	void EXPORT Smoke3_A();
	void EXPORT Smoke3_B();
	void EXPORT Smoke3_C();
	void EXPORT SG_Smoke();
	void EXPORT BounceTouch( CBaseEntity* other );
	void EXPORT SlideTouch( CBaseEntity* other );
	void EXPORT C4Touch( CBaseEntity* other );
	void EXPORT ExplodeTouch( CBaseEntity* other );
	void EXPORT DangerSoundThink();
	void EXPORT PreDetonate();
	void EXPORT Detonate();
	void EXPORT SG_Detonate();
	void EXPORT Detonate2();
	void EXPORT Detonate3();
	void EXPORT DetonateUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT TumbleThink();
	void EXPORT SG_TumbleThink();
	void EXPORT C4Think();
	virtual void BounceSound();
	Vector m_vecLastBounce; // where the last bounce sound was, not saved
	int BloodColor() override { return DONT_BLEED; }
	void Killed( entvars_t* attacker, int gib ) override;
	int ObjectCaps() override { return m_bIsC4 ? FCAP_CONTINUOUS_USE : 0; }
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	bool		m_bStartDefuse;
	bool		m_bIsC4;
	CBaseEntity* m_pBombDefuser;
	float		m_flDefuseCountDown;
	float		m_flC4Blow;
	float		m_flNextFreqInterval;
	float		m_flNextBeep;
	float		m_flNextFreq;
	const char*	m_sBeepName;
	float		m_fAttenu;
	float		m_flNextBlink;
	float		m_fNextDefuse;
	bool		m_bJustBlew;
	int			m_iTeam;
	int			m_iCurWave;
	edict_t*	m_pentCurBombTarget;
	int			m_SGSmoke;
	int			m_angle;
	unsigned short m_usEvent;
	bool		m_bLightSmoke;
	bool		m_bDetonated;
	int			m_throwerClasstype;
	BOOL		m_fRegisteredSound; // whether or not this grenade has issued its DANGER sound to the world sound list yet.
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
public:
	void SetObjectCollisionBox() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	virtual int AddToPlayer( CBasePlayer* pPlayer );	// return TRUE if the item you want the item added to the player inventory
	virtual int AddDuplicate( CBasePlayerItem* pItem ) { return FALSE; }	// return TRUE if you want your duplicate removed from world
	void EXPORT DestroyItem();
	void EXPORT DefaultTouch( CBaseEntity* pOther );
	void EXPORT FallThink();
	void EXPORT Materialize();
	void EXPORT AttemptToMaterialize();
	CBaseEntity* Respawn() override;
	void FallInit();
	void CheckRespawn();
	virtual int GetItemInfo( ItemInfo* p ) { return 0; }
	virtual BOOL CanDeploy() { return TRUE; }
	virtual BOOL CanDrop() { return TRUE; }
	virtual BOOL Deploy()								// returns is deploy was successful
	{
#ifdef CLIENT_DLL
		return FALSE;
#else
		return TRUE;
#endif
	}
	virtual BOOL IsWeapon() { return FALSE; }
	virtual BOOL CanHolster() { return TRUE; }
	virtual void Holster( int skiplocal = 0 );
	virtual void UpdateItemInfo() {}
	virtual void ItemPreFrame() {}
	virtual void ItemPostFrame() {}
	virtual void Drop();
	virtual void Kill();
	virtual void AttachToPlayer( CBasePlayer* pPlayer );
	virtual int PrimaryAmmoIndex() { return -1; }
	virtual int SecondaryAmmoIndex() { return -1; }
	virtual int UpdateClientData( CBasePlayer* pPlayer ) { return 0; }
	virtual CBasePlayerItem* GetWeaponPtr() { return NULL; }
	virtual float GetMaxSpeed() { return 225; }
	virtual int iItemSlot() { return 0; }

	int iItemPosition() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].iPosition; }
	const char* pszAmmo1() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].pszAmmo1; }
	int iMaxAmmo1() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].iMaxAmmo1; }
	const char* pszAmmo2() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].pszAmmo2; }
	int iMaxAmmo2() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].iMaxAmmo2; }
	const char* pszName() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].pszName; }
	int iMaxClip() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].iMaxClip; }
	int iWeight() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].iWeight; }
	int iFlags() { return ItemInfoArray[m_iId + (m_bIsAccessory ? 32 : 0)].iFlags; }

	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];
	CBasePlayer*		m_pPlayer;
	CBasePlayerItem*	m_pNext;
	int				m_iId;												// WEAPON_???
	BOOL			m_bIsAccessory;

	// int		m_iIdPrimary;										// Unique Id for primary ammo
	// int		m_iIdSecondary;										// Unique Id for secondary ammo
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};


// inventory items that 
class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	void Spawn() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	int AddDuplicate( CBasePlayerItem* pItem ) override;
	virtual int ExtractAmmo( CBasePlayerWeapon* pWeapon ); //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo( CBasePlayerWeapon* pWeapon ); // { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int AddWeapon() { ExtractAmmo( this ); return TRUE; }
	BOOL AddPrimaryAmmo( int count, char* name, int maxClip, int maxCarry );
	BOOL AddSecondaryAmmo( int count, char* name, int maxCarry );
	void UpdateItemInfo() override {}
	virtual BOOL PlayEmptySound();
	virtual void ResetEmptySound();
	virtual void SendWeaponAnim( int iAnim, int skiplocal = 1 );
	BOOL CanDeploy() override { return TRUE; }
	virtual BOOL IsUseable();
	BOOL DefaultDeploy( const char* viewModel, const char* weaponModel, int iAnim, const char* animExt, int skiplocal = 0 );
	BOOL DefaultReload( int clipSize, int iAnim, float delay );
	void ItemPostFrame() override;
	virtual void PrimaryAttack() {}
	virtual void SecondaryAttack() {}
	virtual void Reload() {}
	virtual void WeaponIdle() {}
	int UpdateClientData( CBasePlayer* pPlayer ) override;
	virtual void RetireWeapon();
	virtual BOOL ShouldWeaponIdle() { return FALSE; }
	void Holster( int skiplocal = 0 ) override;
	virtual BOOL UseDecrement() { return FALSE; }
	BOOL IsWeapon() override { return TRUE; }
	int PrimaryAmmoIndex() override
	{
#ifdef CLIENT_DLL
		return -1;
#else
		return m_iPrimaryAmmoType;
#endif
	}
	int SecondaryAmmoIndex() override { return -1; }
	CBasePlayerItem* GetWeaponPtr() override { return this; }
	void FireRemaining( int& shotsFired, float& shootTime, BOOL isGlock18 );
	void KickBack( float upBase, float lateralBase, float upModifier, float lateralModifier,
		float upMax, float lateralMax, int directionChange );
	void EjectBrassLate();
	void MakeBeam();
	void BeamUpdate();
	void ReloadSound();
	virtual unsigned short GetWeaponState() const { return m_WeaponState; }

	float		m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float		m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float		m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int			m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int			m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int			m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int			m_iDefaultAmmo;
	int			m_iClientClip;
	int			m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int			m_fInReload;										// Are we in the middle of a reload;
	int			m_fInSpecialReload;
	int			m_iShellId;
	bool		m_bDelayFire;
	int			m_iDirection;
	bool		m_bSecondarySilencerOn;
	float		m_flAccuracy;
	float		m_flLastFire;
	int			m_iShotsFired;
	Vector		m_vVecAiming;
	string_t	model_name;
	float		m_flGlock18Shoot;
	int			m_iGlock18ShotsFired;
	float		m_flFamasShoot;
	int			m_iFamasShotsFired;
	float		m_fBurstSpread;
	int			m_iWeaponState;
	float		m_flNextReload;
	float		m_flDecreaseShotsFired;
	int			m_iPlayEmptySound;
	int			m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the
	unsigned short m_usFireGlock18;
	unsigned short m_usFireFamas;

protected:
	virtual void SetWeaponState( const unsigned short weaponState ) { m_WeaponState = weaponState; }
	unsigned short m_WeaponState;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CBasePlayerAmmo : public CBaseEntity
{
public:
	CBasePlayerAmmo() : m_numclips( 1 ) {}
	void Spawn() override;
	void KeyValue( KeyValueData* pkvd ) override;
	void EXPORT DefaultTouch( CBaseEntity* pOther );
	virtual BOOL AddAmmo( CBaseEntity* pOther ) { return TRUE; }
	CBaseEntity* Respawn() override;
	void EXPORT Materialize();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int m_numclips;
};

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo. 
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void Precache() override;
	void Spawn() override;
	void Touch( CBaseEntity* pOther ) override;
	void KeyValue( KeyValueData* pkvd ) override;
	BOOL IsEmpty();
	int GiveAmmo( int count, const char* name, int maxCarry, int* index = NULL );
	void SetObjectCollisionBox() override;

public:
	void EXPORT Kill();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	BOOL HasWeapon( CBasePlayerItem* pItem );
	BOOL PackWeapon( CBasePlayerItem* pWeapon );
	BOOL PackAmmo( int name, int count );
	CBasePlayerItem*	m_rgpPlayerItems[MAX_ITEM_TYPES]; // one slot for each 
	int				m_rgiszAmmo[MAX_AMMO_SLOTS]; // ammo names
	int				m_rgAmmo[MAX_AMMO_SLOTS]; // ammo quantities
	int				m_cAmmoTypes; // how many ammo types packed into this box (if packed by a level designer)
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

typedef struct
{
	CBaseEntity*	pEntity;
	float			amount;
	int				type;
} MULTIDAMAGE;

extern MULTIDAMAGE gMultiDamage;
extern int giAmmoIndex;
extern int gmsgWeapPickup;
extern short g_sModelIndexLaser;
extern short g_sModelIndexLaserDot;
extern short g_sModelIndexFireball;
extern short g_sModelIndexSmoke;
extern short g_sModelIndexWExplosion;
extern short g_sModelIndexBubbles;
extern short g_sModelIndexBloodDrop;
extern short g_sModelIndexBloodSpray;
extern const char* g_pModelNameLaser;

void	ClearMultiDamage();
void	ApplyMultiDamage( entvars_t* inflictor, entvars_t* attacker );
void	AddMultiDamage( entvars_t* inflictor, CBaseEntity* entity, float damage, int damageType );
void	SpawnBlood( Vector origin, int bloodColor, float damage );
int		DamageDecal( CBaseEntity* entity, int damageType );
void	DecalGunshot( TraceResult* trace, int bulletType );
void	EjectBrass( const Vector& origin, const Vector& velocity, float rotation, int model, int soundType );
int		MaxAmmoCarry( int name );
void	W_Precache();
void	UTIL_PrecacheOtherWeapon( const char* classname );

class CBlowtorch : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	Vector Use();
	BOOL Deploy() override;
	void Reload() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void SetWeaponState( const unsigned short weaponState ) override;
	void Restart() override;
	unsigned short m_usFireBlowtorch;
	unsigned short m_usIdleBlowtorch;
	unsigned short m_usHolsterBlowtorch;
	BOOL m_playFireSound;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CBriefcase : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void SetWeaponState( const unsigned short weaponState ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	unsigned short m_usFireBriefcase;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CZipline : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL CanDrop() override;
	BOOL UseDecrement() override;
	CBaseEntity* FindZipLineEntity();
	unsigned short m_usFireZipline;

protected:
	void SetWeaponState( const unsigned short weaponState ) override;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CLaws : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void SetWeaponState( const unsigned short weaponState ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void LawsFire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	unsigned short m_usFireLaws;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CLawsRocket : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT FollowThink();
	void EXPORT IgniteThink();
	void EXPORT RocketTouch( CBaseEntity* pOther );
	static CLawsRocket* CreateLawsRocket( Vector origin, Vector angles, CBaseEntity* owner, CLaws* launcher );
	static TYPEDESCRIPTION m_SaveData[];
	int m_iTrail;
	float m_flIgniteTime;
	CLaws* m_pLauncher;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CBomb : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void FollowThink();
	void IgniteThink();
	void EXPORT BombThink();
	void EXPORT BombTouch( CBaseEntity* pOther );
	static CBomb* CreateBomb( Vector origin, Vector angles, CBaseEntity* owner );
	static TYPEDESCRIPTION m_SaveData[];
};

class CRadioControlledBomb : public CBasePlayerWeapon
{
public:
	CRadioControlledBomb() { ClearBombArray(); }
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void ClearBombArray();
	BOOL BombArrayIsEmpty();
	BOOL AddToBombArray( CBomb* bomb );
	void KeyValue( KeyValueData* pkvd ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	BOOL PlantBomb();
	void DetonateBombs();
	void SetWeaponState( const unsigned short weaponState ) override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	static TYPEDESCRIPTION m_SaveData[];
	CBomb* m_BombList[20];
};

class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void AK47Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAK47;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CAUG : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void AUGFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	float GetMaxSpeed() override;
	void WeaponIdle() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAug;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CAWP : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void AWPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireAWP;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void M249Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM249;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CM4A1 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void M4A1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM4A1;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CM60 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void M60Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM60;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CMP5N : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void MP5NFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void Holster( int skiplocal = 0 ) override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMP5N;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CP90 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void P90Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireP90;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CTMP : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void TMPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireTMP;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CUMP45 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void UMP45Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireUMP45;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CMAC10 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void MAC10Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMAC10;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSG552 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void SG552Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	float GetMaxSpeed() override;
	void WeaponIdle() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireSG552;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSG550 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void SG550Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireSG550;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CG3SG1 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void G3SG1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireG3SG1;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSCOUT : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void SCOUTFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireScout;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CGalil : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void GalilFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireGalil;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFamas : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void FamasFire( float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL fBurst );
	BOOL Deploy() override;
	void Reload() override;
	float GetMaxSpeed() override;
	void WeaponIdle() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	int iShellOn;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CP228 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void P228Fire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void MakeBeam();
	void BeamUpdate();
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireP228;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CELITE : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void ELITEFire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireELITE_LEFT;
	unsigned short m_usFireELITE_RIGHT;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFiveSeven : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void FiveSevenFire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void MakeBeam();
	void BeamUpdate();
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireFiveSeven;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CUSP : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void USPFire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void MakeBeam();
	void BeamUpdate();
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireUSP;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CGLOCK18 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void GLOCK18Fire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	bool m_bBurstFire;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CDEAGLE : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void DEAGLEFire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void MakeBeam();
	void BeamUpdate();
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
	int m_iShell;
	unsigned short m_usFireDeagle;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CM3 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void Holster( int skipLocal ) override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireM3;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CXM1014 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireXM1014;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFlashbang : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skipLocal ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL CanDrop() override;
	BOOL CanDeploy() override;
	BOOL UseDecrement() override;
};

class CHEGrenade : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skipLocal ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL CanDrop() override;
	BOOL CanDeploy() override;
	BOOL UseDecrement() override;
	unsigned short m_usCreateExplosion;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSmokeGrenade : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skipLocal ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL CanDrop() override;
	BOOL CanDeploy() override;
	BOOL UseDecrement() override;
	unsigned short m_usCreateSmoke;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CC4 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skipLocal ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	BOOL m_bStartedArming;
	BOOL m_bBombPlacedAnimation;
	float m_fArmedTime;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CKnife : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	void EXPORT SwingAgain();
	void EXPORT Smack();
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponAnimation( int animation );
	int Stab( int first );
	int Swing( int first );
	BOOL Deploy() override;
	void Holster( int skipLocal ) override;
	float GetMaxSpeed() override;
	BOOL CanDrop() override;
	BOOL UseDecrement() override;
	void WeaponIdle() override;
	void SetWeaponState( unsigned short state ) override;
	TraceResult m_trHit;
	unsigned short m_usKnife;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CShieldGun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void ShieldGunFire( float spread, float cycleTime, BOOL useAutoAim );
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	void MakeBeam();
	void BeamUpdate();
	float GetMaxSpeed() override;
	void Holster( int skipLocal ) override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	int m_iShell;
	unsigned short m_usFireShieldGun;
	BOOL m_bShieldDrawn;
	float m_fMaxSpeed;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CCamera : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skipLocal ) override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void SetWeaponState( unsigned short state ) override;
	unsigned short m_usFireCamera;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFiberOpticCamera : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skipLocal ) override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void SetWeaponState( unsigned short state ) override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
#endif
	BOOL m_bIsZoomed;
	unsigned short m_usFireFiberOpticCamera;
	static TYPEDESCRIPTION m_SaveData[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CRadio : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skipLocal ) override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	void SetWeaponState( unsigned short state ) override;
	unsigned short m_usFireRadio;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CMedkit : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skipLocal ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL CanDrop() override;
	BOOL CanDeploy() override;
	unsigned short m_usFireMedkit;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSyringe : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	CBaseEntity* FindInjectionTarget();
	void Inject( CBaseEntity* target );
	void PrimaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skipLocal ) override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
	BOOL CanDrop() override;
	void SetWeaponState( unsigned short state ) override;
	unsigned short m_usFireSyringe;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CHolster : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Holster( int skipLocal ) override;
	void Reload() override;
	void WeaponIdle() override;
	float GetMaxSpeed() override;
};

#endif
