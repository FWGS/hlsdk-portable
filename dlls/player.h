/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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

#ifndef PLAYER_H
#define PLAYER_H

#include "pm_materials.h"
#include "hintmessage.h"

#define PLAYER_FATAL_FALL_SPEED		1024 // approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580 // approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		((float)100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )) // damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD	350

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER		( 1 << 0 )
#define PFLAG_ONSWING		( 1 << 0 )
#define PFLAG_ONTRAIN		( 1 << 1 )
#define PFLAG_ONBARNACLE		( 1 << 2 )
#define PFLAG_DUCKING			( 1 << 3 )		// In the process of ducking, but totally squatted yet
#define PFLAG_USING			( 1 << 4 )		// Using a continuous entity
#define PFLAG_OBSERVER		( 1 << 5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST		4		// max of 4 suit sentences queued up at any time
#define CSUITNOREPEAT		32
#define SUIT_GROUP			TRUE
#define SUIT_SENTENCE		FALSE
#define SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN		60
#define SUIT_NEXT_IN_5MIN		300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define SOUND_FLASHLIGHT_ON	"items/flashlight1.wav"
#define SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"
#define TEAM_NAME_LENGTH		16
#define MAX_ID_RANGE			2048
#define SBAR_STRING_SIZE		128
#define CHAT_INTERVAL		1.0f

#define AUTOAIM_2DEGREES		0.0348994967025
#define AUTOAIM_5DEGREES		0.08715574274766
#define AUTOAIM_8DEGREES		0.1391731009601
#define AUTOAIM_10DEGREES		0.1736481776669

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1,
	PLAYER_ATTACK2,
	PLAYER_FLINCH,
	PLAYER_LARGE_FLINCH,
	PLAYER_RELOAD,
	PLAYER_HOLDBOMB
} PLAYER_ANIM;

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END
};

enum TeamName
{
	UNASSIGNED,
	TERRORIST,
	CT,
	SPECTATOR
};

enum
{
	LEVEL_Training		= 2,
	LEVEL_Recoil			= 4,
	LEVEL_LostCause		= 8,
	LEVEL_SecretWar		= 16,
	LEVEL_BuildingRecon	= 32,
	LEVEL_Druglab		= 64,
	LEVEL_Motorcade		= 128,
	LEVEL_ThinIce		= 256,
	LEVEL_DownedPilot	= 512,
	LEVEL_Hankagai		= 1024,
	LEVEL_Turncrank		= 2048,
	LEVEL_Alamo			= 4096,
	LEVEL_HighRise		= 8192,
	LEVEL_Truth			= 16384,
	LEVEL_Run			= 32768,
	LEVEL_Miami			= 65536,
	LEVEL_Fastline		= 131072,
	LEVEL_Pipedream		= 262144,
	LEVEL_Sandstorm		= 524288,
	LEVEL_GameFinished	= 1048576	// the ending (cz_end) was reached: no mission is the current one on the world map
};

class CTriggerStealth;
class CTriggerUseTool;
class CTriggerCamera;
class CBasePlayerItem;

class CBasePlayer : public CBaseMonster
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	CBasePlayer()
	{
		m_bFirstRestore = FALSE;
		m_iHudIcons = 0;
		m_iHudIconsCorrectWeapon = 0;
		m_iHudIconsInPosition = 0;
		m_stealthTrigger = NULL;
		m_toolTrigger = NULL;
		m_endingMission = FALSE;
		m_LevelLocks = 0;
		savedHudBits = 0;
		savedIconBits = 0;
		m_bHasNightVision = FALSE;
		m_bInitClientMsgs = TRUE;
		m_cameraTrigger = NULL;
		m_inFiberCam = FALSE;
		m_chatterTime = 0;
		m_forceEquip = FALSE;
	}

	void Spawn() override;
	void Pain( int hitGroup, bool hasArmour );
	virtual void Jump();
	virtual void Duck();
	virtual void PreThink();
	virtual void PostThink();
	Vector GetGunPosition() override;
	int TakeHealth( float health, int damageType ) override;
	void TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType ) override;
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void Killed( entvars_t* attacker, int gib ) override;
	Vector BodyTarget( const Vector& source ) override;
	void StartSneaking() override;
	void StopSneaking() override;
	BOOL IsSneaking() override;
	BOOL IsAlive() override;
	BOOL ShouldFadeOnDeath() override;
	BOOL IsPlayer() override { return TRUE; }
	BOOL IsNetClient() override { return TRUE; }
	const char* TeamID() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_playerSaveData[];
	void Reset();
	void GiveDefaultItems();
	void SetBombIcon( BOOL flashing );
	void SetProgressBarTime( int time );
	void SetPlayerModel( BOOL custom );
	void RenewItems();
	void PackDeadPlayerItems();
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem* weapon );
	virtual void UpdateClientData();
	int ObjectCaps() override;
	void Precache() override;
	BOOL IsOnLadder();
	BOOL FlashlightIsOn();
	void FlashlightTurnOn();
	void FlashlightTurnOff();
	BOOL NightVisionIsOn();
	void NightVisionTurnOn();
	void NightVisionTurnOff();
	BOOL SniperLaserIsOn();
	void SniperLaserTurnOn();
	void SniperLaserTurnOff();
	void StartFiberCamera( Vector origin, Vector angles, const int entityID );
	void StopFiberCamera();
	BOOL HandleBombDefuse();
	void UpdatePlayerSound();
	void DeathSound() override;
	int Classify() override;
	void SetAnimation( PLAYER_ANIM animation );
	void SetWeaponAnimType( const char* extension );
	virtual void ImpulseCommands();
	void CheatImpulseCommands( int impulse );
	void StartDeathCam();
	void StartObserver( Vector position, Vector viewAngle );
	void AddPoints( int score, BOOL allowNegative ) override;
	void AddPointsToTeam( int score, BOOL allowNegative ) override;
	int AddPlayerItem( CBasePlayerItem* item ) override;
	int RemovePlayerItem( CBasePlayerItem* item ) override;
	void DropActiveItem();
	void DropPlayerItem( const char* name );
	BOOL HasPlayerItem( CBasePlayerItem* item );
	BOOL HasNamedPlayerItem( const char* name );
	BOOL HasWeapons();
	void SelectPrevItem( int slot );
	void SelectNextItem( int slot );
	BOOL SelectLastItem();
	void SelectItem( const char* name );
	void ItemPreFrame();
	void ItemPostFrame();
	void GiveNamedItem( const char* name );
	void FireWeapon();
	void AltFireWeapon();
	void EnableControl( BOOL control );
	void SetMaxSpeed( const float speed );
	void ResetMaxSpeed();
	bool HintMessage( const char* message, BOOL displayIfDead = FALSE, BOOL override = FALSE );
	int GiveAmmo( int count, char* name, int maximum ) override;
	void SendAmmoUpdate();
	void WaterMove();
	void EXPORT PlayerDeathThink();
	void PlayerUse();
	void CheckSuitUpdate();
	void SetSuitUpdate( const char *name, int group, int noRepeatTime );
	void UpdateGeigerCounter();
	void CheckTimeBasedDamage();
	BOOL FBecomeProne() override;
	void BarnacleVictimBitten( entvars_t* barnacle ) override;
	void BarnacleVictimReleased() override;
	static int GetAmmoIndex( const char* name );
	int AmmoInventory( int index );
	int Illumination() override;
	void ResetAutoaim();
	Vector GetAutoaimVector( float delta );
	Vector AutoaimDeflection( Vector& source, float distance, float delta );
	void ForceClientDllUpdate();
	void DeathMessage( entvars_t* killer );
	void SetCustomDecalFrames( int frames );
	int GetCustomDecalFrames();
	void TabulateAmmo();
	void InitStatusBar();
	void UpdateStatusBar();
	BOOL IsArmored( int hitGroup );
	BOOL ShouldDoLargeFlinch( int hitGroup, int gunType );
	void SetPrefsFromUserinfo( char* infoBuffer );
	void UpdateShieldCrosshair( bool draw );
	int GetActiveWeaponClassname();
	void Activate() override;
	void SendFogMessage();
	void SendWeatherInfo();
	void SendLevelLockInfo();
	void SendWindInfo();
	void SendGrassInfo();
	bool isEquippingSilently() { return m_equipSilent != FALSE; }
	void setEquipSilent( BOOL silent ) { m_equipSilent = silent; }
	void SetSecurityCamera( bool status, CTriggerCamera* camera );
	void SendInitClientMessages();
	void DisplayObjectives();
	void GetCTHelp( CBaseEntity* enemy );

	float			m_flFlashLightTime;	// Time until next battery draw/Recharge
	int				m_iFlashBattery;		// Flashlight Battery Draw
	int				m_afButtonLast;
	int				m_afButtonPressed;
	int				m_afButtonReleased;
	int				m_rgItems[5];
	unsigned int	m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden


// these are time-sensitive things that we keep track of
	float			m_flTimeStepSound;	// when the last stepping sound was made
	float			m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float			m_flSwimTime;		// how long player has been underwater
	float			m_flDuckTime;		// how long we've been ducking
	float			m_flWallJumpTime;	// how long until next walljump
	float			m_flSuitUpdate;					// when to play next suit update
	int				m_rgSuitPlayList[CSUITPLAYLIST]; // next sentencenum to play for suit update
	int				m_iSuitPlayNext;				// next sentence slot for queue storage;
	int				m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float			m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int				m_lastDamageAmount;		// Last damage taken
	CBasePlayerItem*	m_rgpPlayerItems[7];
	CBasePlayerItem*	m_pActiveItem;
	CBasePlayerItem*	m_pClientActiveItem;  // client version of the active item
	CBasePlayerItem*	m_pLastItem;
	int				m_rgAmmo[32];
	int				m_rgAmmoLast[32];
	int				m_idrowndmg;			// track drowning damage taken
	int				m_idrownrestored;		// track drowning damage restored
	float			m_tSneaking;
	int				m_iTrain;				// Train control position
	BOOL			m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info
	int				m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
	float			m_flFallVelocity;
	int				m_iTargetVolume; // ideal sound volume. 
	int				m_iWeaponVolume; // how loud the player's weapon is right now.
	int				m_iExtraSoundTypes; // additional classification for this weapon's sound
	int				m_iWeaponFlash; // brightness of the weapon flash
	BOOL			m_fLongJump; // does this player have the longjump module?
												// the hude via the DAMAGE message
	BOOL			m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	float			m_tbdPrev;				// Time-based damage timer
	EHANDLE			m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	int				m_iHideHUD;		// the players hud weapon info is to be hidden
	int				m_iFOV;
	int				m_iIncognito;
	int				m_iAccessoryBits;
	int				m_iOldAccessoryBits;
	TeamName		m_iTeam;
	float			m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())
	BOOL			m_fGameHUDInitialized;
	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	float			m_flStopExtraSoundTime;
	int				m_fKnownItem;		// True when a new item needs to be added
	int				m_iPlayerSound; // the index of the sound list slot reserved for this player
	edict_t*		m_pentSndLast;			// last sound entity to modify player room type
	float			m_flSndRoomtype;		// last roomtype set by sound entity
	float			m_flSndRange;			// dist from player to sound entity
	int				m_fNewAmmo;			// True when a new item has been added
	float			m_flgeigerRange;		// range to nearest radiation source
	float			m_flgeigerDelay;		// delay per update of range msg to client
	int				m_igeigerRangePrev;
	int				m_iStepLeft;			// alternate left/right foot stepping sound
	char			m_szTextureName[13];	// current texture name we're standing on
	char			m_chTextureType;		// current texture type
	int				m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int				m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int				m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int				m_iClientHideHUD;
	int				m_nCustomSprayFrames; // Custom clan logo frames for this player
	Vector			m_vecAutoAim;
	BOOL			m_fOnTarget;
	int				random_seed;    // See that is shared between client & server for shared weapons code
	float			m_flFlinchTime;
	float			m_flAnimTime;
	BOOL			m_bHighDamage;
	float			m_flVelocityModifier;
	int				m_iLastZoom;
	BOOL			m_bResumeZoom;
	float			m_flEjectBrass;
	int				m_iKevlar;
	BOOL			m_bNotKilled;
	int				m_iAccount;
	BOOL			m_bHasPrimary;
	float			m_flDeathThrowTime;
	int				m_iThrowDirection;
	float			m_flLastTalk;
	BOOL			m_bJustConnected;
	BOOL			m_bContextHelp;
	CBaseEntity*	m_pIntroCamera;
	float			m_fIntroCamTime;
	float			m_fLastMovement;
	BOOL			m_bMissionBriefing;
	BOOL			m_bTeamChanged;
	int				m_iTeamKills;
	int				m_iIgnoreGlobalChat;
	BOOL			m_bHasNightVision;
	BOOL			m_bNightVisionOn;
	Vector			m_vRecentPath[20];
	float			m_flIdleCheckTime;
	float			m_flRadioTime;
	int				m_iRadioMessages;
	BOOL			m_bIgnoreRadio;
	BOOL			m_bHasC4;
	BOOL			m_bHasDefuser;
	BOOL			m_bKilledByBomb;
	Vector			m_vBlastVector;
	BOOL			m_bKilledByGrenade;
	CHintMessageQueue m_hintMessageQueue;
	int				m_flDisplayHistory;
	int				m_iChaseTarget;
	CBaseEntity*	m_pChaseTarget;
	float			m_fCamSwitch;
	BOOL			m_bEscaped;
	BOOL			m_bIsVIP;
	float			m_tmNextRadarUpdate;
	Vector			m_vLastOrigin;
	int				m_iCurrentKickVote;
	float			m_flNextVoteTime;
	BOOL			m_bJustKilledTeammate;
	int				m_iHostagesKilled;
	int				m_iMapVote;
	BOOL			m_bCanShoot;
	float			m_flLastFired;
	float			m_flLastAttackedTeammate;
	BOOL			m_bHeadshotKilled;
	BOOL			m_bPunishedForTK;
	BOOL			m_bHasChangedName;
	BOOL			m_bIsDefusing;
	BOOL			m_bFirstConnected;
	BOOL			m_bFirstRestore;
	float			m_fNextSuicideTime; // the time after which the player can next use the suicide command
	int				m_iClientFOV;	// client's known FOV
	int				m_iHudIcons;
	int				m_iHudIconsCorrectWeapon;
	int				m_iHudIconsInPosition;
	int				ammo_buckshot;
	int				ammo_9mm;
	int				ammo_556nato;
	int				ammo_556natobox;
	int				ammo_762nato;
	int				ammo_762natobox;
	int				ammo_45acp;
	int				ammo_50ae;
	int				ammo_338mag;
	int				ammo_57mm;
	int				ammo_357sig;
	int				ammo_66mm;
	int				m_iDeaths;
	float			m_iRespawnFrames;
	CTriggerStealth*	m_stealthTrigger;
	CTriggerUseTool*	m_toolTrigger;
	CTriggerCamera*	m_cameraTrigger;
	int				m_lastx;
	int				m_lasty;
	float			m_flNextDecalTime; // next time this player can spray a decal
	char			m_szTeamName[TEAM_NAME_LENGTH];
	float			m_flStartCharge;
	float			m_flAmmoStartCharge;
	float			m_flPlayAftershock;
	float			m_flNextAmmoBurn; // while charging, when to absorb another unit of player's ammo?
	int				m_izSBarState[SBAR_END];
	float			m_flNextSBarUpdateTime;
	float			m_flStatusBarDisappearDelay;
	char			m_SbarString0[SBAR_STRING_SIZE];
	char			m_SbarString1[SBAR_STRING_SIZE];
	float			m_flNextChatTime;
	int				m_iAutoWepSwitch;
	BOOL			m_bVGUIMenus;
	BOOL			m_bShowHints;
	BOOL			m_bShieldDrawn;
	BOOL			m_bLeftHanded;
	float			m_defuseTime;
	char			m_szAnimExtention[32];
	Vector			m_fiberCamOrigin;
	Vector			m_fiberCamAngle;
	int				m_fiberCamID;
	BOOL			m_inFiberCam;
	BOOL			m_endingMission;
	int				m_LevelLocks;
	static int		gLevelLocks;
	int				savedHudBits;
	int				savedIconBits;
	BOOL			m_equipSilent;
	BOOL			m_bInitClientMsgs;
	float			m_chatterTime;
	BOOL			m_forceEquip;
};

extern int gmsgHudText;
extern int gmsgItemPickup;
extern int gmsgWorldMap;
extern BOOL gInitHUD;

#endif
