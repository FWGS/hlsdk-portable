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
#ifndef PLAYER_H
#define PLAYER_H

#include "pm_materials.h"

//++ BulliT
#include "items.h"
#include "gamerules.h"
#include "agglobal.h"
#include "agclient.h"
extern int gmsgSpectator;
//-- Martin Webrant

#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	580// approx 20 feet
#define DAMAGE_FOR_FALL_SPEED		(float) 100 / ( PLAYER_FATAL_FALL_SPEED - PLAYER_MAX_SAFE_FALL_SPEED )// damage per unit per second.
#define PLAYER_MIN_BOUNCE_SPEED		200
#define PLAYER_FALL_PUNCH_THRESHHOLD (float)350 // won't punch player's screen/make scrape noise unless player falling at least this fast.

//
// Player PHYSICS FLAGS bits
//
#define PFLAG_ONLADDER		( 1<<0 )
#define PFLAG_ONSWING		( 1<<0 )
#define PFLAG_ONTRAIN		( 1<<1 )
#define PFLAG_ONBARNACLE	( 1<<2 )
#define PFLAG_DUCKING		( 1<<3 )		// In the process of ducking, but totally squatted yet
#define PFLAG_USING		( 1<<4 )		// Using a continuous entity
#define PFLAG_OBSERVER		( 1<<5 )		// player is locked in stationary cam mode. Spectators can move, observers can't.

//
// generic player
//
//-----------------------------------------------------
//This is Half-Life player entity
//-----------------------------------------------------
#define CSUITPLAYLIST	4		// max of 4 suit sentences queued up at any time

#define SUIT_GROUP			TRUE
#define	SUIT_SENTENCE		FALSE

#define	SUIT_REPEAT_OK		0
#define SUIT_NEXT_IN_30SEC	30
#define SUIT_NEXT_IN_1MIN	60
#define SUIT_NEXT_IN_5MIN	300
#define SUIT_NEXT_IN_10MIN	600
#define SUIT_NEXT_IN_30MIN	1800
#define SUIT_NEXT_IN_1HOUR	3600

#define CSUITNOREPEAT		32

#define	SOUND_FLASHLIGHT_ON		"items/flashlight1.wav"
#define	SOUND_FLASHLIGHT_OFF	"items/flashlight1.wav"

#define TEAM_NAME_LENGTH	16

typedef enum
{
	PLAYER_IDLE,
	PLAYER_WALK,
	PLAYER_JUMP,
	PLAYER_SUPERJUMP,
	PLAYER_DIE,
	PLAYER_ATTACK1
} PLAYER_ANIM;

#define MAX_ID_RANGE 2048
#define SBAR_STRING_SIZE 128

enum sbar_data
{
	SBAR_ID_TARGETNAME = 1,
	SBAR_ID_TARGETHEALTH,
	SBAR_ID_TARGETARMOR,
	SBAR_END
};

#define CHAT_INTERVAL 1.0f

class CBasePlayer : public CBaseMonster
{
public:
	// Spectator camera
	void	Observer_FindNextPlayer( bool bReverse );
	void	Observer_HandleButtons();
	void	Observer_SetMode( int iMode );
	void	Observer_CheckTarget();
	void	Observer_CheckProperties();
	EHANDLE	m_hObserverTarget;
	float	m_flNextObserverInput;
	int		m_iObserverWeapon;	// weapon of current tracked target
	int		m_iObserverLastMode;// last used observer mode
	int		IsObserver() { return pev->iuser1; };

	int					random_seed;    // See that is shared between client & server for shared weapons code

	int					m_iPlayerSound;// the index of the sound list slot reserved for this player
	int					m_iTargetVolume;// ideal sound volume. 
	int					m_iWeaponVolume;// how loud the player's weapon is right now.
	int					m_iExtraSoundTypes;// additional classification for this weapon's sound
	int					m_iWeaponFlash;// brightness of the weapon flash
	float				m_flStopExtraSoundTime;

	float				m_flFlashLightTime;	// Time until next battery draw/Recharge
	int					m_iFlashBattery;		// Flashlight Battery Draw

	int					m_afButtonLast;
	int					m_afButtonPressed;
	int					m_afButtonReleased;

	edict_t			   *m_pentSndLast;			// last sound entity to modify player room type
	float				m_flSndRoomtype;		// last roomtype set by sound entity
	float				m_flSndRange;			// dist from player to sound entity

	float				m_flFallVelocity;

	int					m_rgItems[MAX_ITEMS];
	int					m_fKnownItem;		// True when a new item needs to be added
	int					m_fNewAmmo;			// True when a new item has been added

	unsigned int		m_afPhysicsFlags;	// physics flags - set when 'normal' physics should be revisited or overriden
	float				m_fNextSuicideTime; // the time after which the player can next use the suicide command

	// these are time-sensitive things that we keep track of
	float				m_flTimeStepSound;	// when the last stepping sound was made
	float				m_flTimeWeaponIdle; // when to play another weapon idle animation.
	float				m_flSwimTime;		// how long player has been underwater
	float				m_flDuckTime;		// how long we've been ducking
	float				m_flWallJumpTime;	// how long until next walljump

	float				m_flSuitUpdate;					// when to play next suit update
	int					m_rgSuitPlayList[CSUITPLAYLIST];// next sentencenum to play for suit update
	int					m_iSuitPlayNext;				// next sentence slot for queue storage;
	int					m_rgiSuitNoRepeat[CSUITNOREPEAT];		// suit sentence no repeat list
	float				m_rgflSuitNoRepeatTime[CSUITNOREPEAT];	// how long to wait before allowing repeat
	int					m_lastDamageAmount;		// Last damage taken
	float				m_tbdPrev;				// Time-based damage timer

	float				m_flgeigerRange;		// range to nearest radiation source
	float				m_flgeigerDelay;		// delay per update of range msg to client
	int					m_igeigerRangePrev;
	int					m_iStepLeft;			// alternate left/right foot stepping sound
	char				m_szTextureName[CBTEXTURENAMEMAX];	// current texture name we're standing on
	char				m_chTextureType;		// current texture type

	int					m_idrowndmg;			// track drowning damage taken
	int					m_idrownrestored;		// track drowning damage restored

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
										// the hude via the DAMAGE message
	BOOL				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	BOOL				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	BOOL				m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE				m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())

	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump; // does this player have the longjump module?

	float       m_tSneaking;
	int			m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV

	// usable player items 
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;  // client version of the active item
	CBasePlayerItem *m_pLastItem;

	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_SLOTS];
	int	m_rgAmmoLast[MAX_AMMO_SLOTS];

	Vector				m_vecAutoAim;
	BOOL				m_fOnTarget;
	int					m_iDeaths;
	float				m_iRespawnFrames;	// used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	virtual void Spawn( void );
	void Pain( void );

	//virtual void Think( void );
	virtual void Jump( void );
	virtual void Duck( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual Vector GetGunPosition( void );
	virtual int TakeHealth( float flHealth, int bitsDamageType );
	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual void	Killed( entvars_t *pevAttacker, int iGib );
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center( ) + pev->view_ofs * RANDOM_FLOAT( 0.5, 1.1 ); };		// position to shoot at
	virtual void StartSneaking( void ) { m_tSneaking = gpGlobals->time - 1; }
	virtual void StopSneaking( void ) { m_tSneaking = gpGlobals->time + 30; }
	virtual BOOL IsSneaking( void ) { return m_tSneaking <= gpGlobals->time; }
	virtual BOOL IsAlive( void ) { return (pev->deadflag == DEAD_NO) && pev->health > 0; }
	virtual BOOL ShouldFadeOnDeath( void ) { return FALSE; }
//++ BulliT
	virtual	BOOL IsPlayer( void ) { return !IsSpectator(); }			// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned
//-- Martin Webrant
	virtual BOOL IsNetClient( void ) { return TRUE; }		// Bots should return FALSE for this, they can't receive NET messages
															// Spectators should return TRUE for this
	virtual const char *TeamID( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	void RenewItems(void);
	void PackDeadPlayerItems( void );
	void RemoveAllItems( BOOL removeSuit );
	BOOL SwitchWeapon( CBasePlayerItem *pWeapon );

	// JOHN:  sends custom messages if player HUD data has changed  (eg health, ammo)
	virtual void UpdateClientData( void );
	
	static	TYPEDESCRIPTION m_playerSaveData[];

	// Player is moved across the transition by other means
	virtual int		ObjectCaps( void ) { return CBaseMonster :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
	virtual void	Precache( void );
	BOOL			IsOnLadder( void );
	BOOL			FlashlightIsOn( void );
	void			FlashlightTurnOn( void );
	void			FlashlightTurnOff( void );

	void UpdatePlayerSound ( void );
	void DeathSound ( void );

	int Classify ( void );
	void SetAnimation( PLAYER_ANIM playerAnim );
	void SetWeaponAnimType( const char *szExtention );
	char m_szAnimExtention[32];

	// custom player functions
	virtual void ImpulseCommands( void );
	void CheatImpulseCommands( int iImpulse );

	void StartDeathCam( void );
	void StartObserver( Vector vecPosition, Vector vecViewAngle );
	void StopObserver();

	void AddPoints( int score, BOOL bAllowNegativeScore );
	void AddPointsToTeam( int score, BOOL bAllowNegativeScore );
	BOOL AddPlayerItem( CBasePlayerItem *pItem );
	BOOL RemovePlayerItem( CBasePlayerItem *pItem, bool bCallHoster );
	void DropPlayerItem ( char *pszItemName );
	BOOL HasPlayerItem( CBasePlayerItem *pCheckItem );
	BOOL HasNamedPlayerItem( const char *pszItemName );
	BOOL HasWeapons( void );// do I have ANY weapons?
	void SelectPrevItem( int iItem );
	void SelectNextItem( int iItem );
	void SelectLastItem(void);
	void SelectItem(const char *pstr);
	void ItemPreFrame( void );
	void ItemPostFrame( void );
	void GiveNamedItem( const char *szName );
	void EnableControl(BOOL fControl);

	int  GiveAmmo( int iAmount, const char *szName, int iMax );
	void SendAmmoUpdate(void);

	void WaterMove( void );
	void EXPORT PlayerDeathThink( void );
	void PlayerUse( void );

	void CheckSuitUpdate();
	void SetSuitUpdate( const char *name, int fgroup, int iNoRepeat );
	void UpdateGeigerCounter( void );
	void CheckTimeBasedDamage( void );

	BOOL FBecomeProne ( void );
	void BarnacleVictimBitten ( entvars_t *pevBarnacle );
	void BarnacleVictimReleased ( void );
	static int GetAmmoIndex(const char *psz);
	int AmmoInventory( int iAmmoIndex );
	int Illumination( void );

	void ResetAutoaim( void );
	Vector GetAutoaimVector( float flDelta  );
	Vector AutoaimDeflection( Vector &vecSrc, float flDist, float flDelta  );

	void ForceClientDllUpdate( void );  // Forces all client .dll specific data to be resent to client.

	void DeathMessage( entvars_t *pevKiller );

	void SetCustomDecalFrames( int nFrames );
	int GetCustomDecalFrames( void );

	void TabulateAmmo( void );

	Vector m_vecLastViewAngles;

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?

	//Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	float m_flNextChatTime;
//++ BulliT
protected:
	bool m_bAdmin;		//Player gained admin status.
	bool m_bIngame;		//Player was in the game when match started.

	float m_fPlayerIdCheck;   //Next time to check player id.

	float m_fDisplayGamemode; //Next time to display gamemode.

	float m_fLongjumpTimer;   //Long jump timer.

	Vector m_vKilled;         //Where the player got killed last time.

	double	m_fFloodLockTill;   //Flood protection: Locked from talking
	double	m_afFloodWhen[10];  //Flood protection: When messages were said
	int	m_iFloodWhenHead;   //Flood protection: Head pointer for when said

	double	m_fPrevSoundFlood;
	double	m_fPrevTextFlood;

	bool	m_bSpawnFull;      //Spawn with all weapons.

	double m_fSpectateTime;  //Dont flood the spectate functions.
	int m_iSpectateWeapon;
	int m_iSpectateAmmoClip;

	int	m_iSpot;          //Info intermission spot.
	bool	m_bSentWallhackInfo;

	//int	m_iWeaponWeight[MAX_WEAPONS];
	bool	m_bInitLocation;     //True to init location on client.

	int m_iLastPlayerId;
	float m_fNextPlayerId;
	bool m_bSentCheatCheck;

public:
	int	m_iVote;             //What player voted. -1 (not voted), 0 no, 1 yes.
	bool m_bDoneFirstSpawn;   //True if player has done the first spawn.
	bool m_bInSpawn;          //True if player is spawning.
	bool m_bReady;	          //True if player is ready to enter lts/lms
	int m_iNumTeams;
	//Userinfo
	int   m_iDisableSpecs;
	int   m_iAutoWepSwitch;
	char  m_szModel[64];

	//CTF
	bool m_bFlagTeam1;        //True if you got flag 1.
	bool m_bFlagTeam2;        //True if you got flag 2.

	enum enumFlagStatus { Off = -1, Home = 0, Stolen = 1, Lost = 2, Carry = 3};
	int m_iFlagStatus1Last;
	int	m_iFlagStatus2Last;
  
	enum { Invalid, Alive, GotKilled, SelfKilled, WorldKilled };
	int m_iStatus;

	//Maps
	int m_iMapListSent;

	void Init();     //Init all extra variables.
	const char *GetAuthID(); //Get steam ID
	const char *GetName();  //Get name
  
	bool IsAdmin();  //Returns true if a voted or real admin.
	void SetIsAdmin(bool bAdmin); //Set to true to be admin.

	bool IsIngame(); //Returns true if allowed to enter the game. If false go specmode.
	void SetIngame(bool bIngame); //Set to true to allow player in the game.

	bool ShouldWeaponSwitch(); //Should weapon switch when walking over?

	bool DisableSpecs(); //Does this player allow spectators.

	void UpdatePlayerId(); //Check if we should send player name.

	void OnPickupLongjump(); //Player picked it up.
	void LongjumpThink(); //The lj think. Handles timer.

	void SetDisplayGamemode(float fTime); //Display gamemode.

	Vector GetKilledPosition(); //Where the player got killed last time.

	void ShowVGUI( int iMenu );
	void ChangeTeam( const char *pszNewTeam, bool bSendScores = true );  //Change team/model.
	bool RespawnMatch(); //Respawn and removes players old enties.
	void ResetScore(); //Reset score.

	bool IsSpectator(); //Returns true if spectating.
	bool IsProxy();	//Returns true if proxy server client.

	bool FloodCheck(); //Returns true if user is flooding us. Use this one for public messages.
	bool FloodSound(); //Returns true if user is flooding us.
	bool FloodText(); //Returns true if user is flooding us.

	void MoveToInfoIntermission( edict_t *pSpot ); //Move to a info intermission spot.
	friend class AgClient; //AgClient class should be able to access protected variables.

	int BloodColor() { return BLOOD_COLOR_RED; } 

	void SetSpawnFull( bool bSpawnFull ); //Set to true to spawn american way.
	bool GetSpawnFull(); //Get spawn full with ammo

	void RemoveAllItemsNoClientMessage();
	EHANDLE m_hSpectateTarget;//The player this player is watching.
	void Spectate_Init();
	void Spectate_Spectate();
	void Spectate_Start( bool bResetScore = true );
	void Spectate_Stop( bool bIntermediateSpawn = false );
	void Spectate_SetMode( int iMode );
	void Spectate_UpdatePosition();
	void Spectate_HandleButtons();
	void Spectate_Nextplayer( bool bReverse );
	void Spectate_Nextspot( bool bReverse );
	bool Spectate_Think();
	bool Spectate_Follow( EHANDLE &pPlayer, int iMode );
	bool Spectate_HLTV();

	void SendWallhackInfo();

	void UpdateFlagStatus( CBasePlayer *pPlayer );

	int  GetWeaponWeight( CBasePlayerItem *pItem );
/*
	void SetWeaponWeight( const char *pszWeaponWeights );
	void InitWeaponWeight();
*/
//-- Martin Webrant
};

//++ BulliT
inline void CBasePlayer::Init()
{
	m_bSentWallhackInfo = false;
	m_bInSpawn = false;

	m_bAdmin = false;
	m_bIngame = 1 > ag_match_running.value;
	m_bDoneFirstSpawn = false;

	m_fPlayerIdCheck = gpGlobals->time + 5.0;

	m_fDisplayGamemode = 0.0;

	m_fLongjumpTimer = 0.0;

	m_iVote = -1;

	m_iStatus = Invalid;

	m_fDisplayGamemode = gpGlobals->time + 5;

	m_vKilled = g_vecZero;

	m_fFloodLockTill = AgTime();

	for( int i = 0; i < sizeof( m_afFloodWhen ) / sizeof( m_afFloodWhen[0] ); i++ )
	{
		m_afFloodWhen[i] = AgTime();
	}

	m_iFloodWhenHead = 0;

	m_fPrevSoundFlood = AgTime();
	m_fPrevTextFlood = AgTime();

	m_bSpawnFull = false;

	Spectate_Init();
	if( ARENA == AgGametype() )
	{
		//Never allow new players in arena mode to spawn directly.
		m_bIngame = false;
		g_pGameRules->m_Arena.ClientConnected( this );
	}
	else if( LMS == AgGametype() )
	{
		//Never allow new players in arena mode to spawn directly.
		m_bIngame = false;
		g_pGameRules->m_LMS.ClientConnected( this );
	}
	else if( CTF == AgGametype() )
	{
		g_pGameRules->m_CTF.ClientConnected( this );

		if( 1 == ag_match_running.value )
		{
			//Check score cache if he was in the game.
			g_pGameRules->m_ScoreCache.RestoreInGame( this );
		}
	}
	else
	{
		if( 1 == ag_match_running.value )
		{
			//Check score cache if he was in the game.
			g_pGameRules->m_ScoreCache.RestoreInGame( this );
		}
	}

	if( 0 < ag_auto_admin.value )
	{
		//Autoadmin the player.
		AdminCache.RestoreAdmin( this );
	}
	
	m_bFlagTeam1 = false;
	m_bFlagTeam2 = false;
	m_iFlagStatus1Last = Off;
	m_iFlagStatus2Last = Off;

	m_iAutoWepSwitch = 1;
	m_iDisableSpecs = 0;

	//InitWeaponWeight();

	m_bReady = true;
	m_iNumTeams = 0;

	m_bInitLocation  = true;
	m_iMapListSent = -1;

	m_szModel[0] = '\0';

	m_iLastPlayerId = -1;
	m_fNextPlayerId = 0.0;
	m_bSentCheatCheck = false;

#ifdef _DEBUG
	if( 0 == strcmp( GetAuthID(),"237555" ) )
		m_bAdmin = true;
#endif
};

inline const char* CBasePlayer::GetAuthID()
{
	if( g_bLangame )
		return "";
	return GETPLAYERAUTHID( edict() );
};

inline const char* CBasePlayer::GetName()
{
	return pev->netname ? ( STRING( pev->netname ) )[0] ? STRING( pev->netname ) : "" : "";
};

inline bool CBasePlayer::IsAdmin()
{
	return m_bAdmin;
};

inline void CBasePlayer::SetIsAdmin(bool bAdmin)
{
	m_bAdmin = bAdmin;
};

inline bool CBasePlayer::IsIngame()
{
  return m_bIngame;
};

inline void CBasePlayer::SetIngame(bool bIngame)
{
	m_bIngame = bIngame;
};

inline bool CBasePlayer::IsSpectator()
{
	return pev->iuser1 > 0 || IsProxy();
};

inline bool CBasePlayer::IsProxy()
{
	if (pev->flags & FL_PROXY )
		return true;
	return false;
};

inline Vector CBasePlayer::GetKilledPosition()
{
	return m_vKilled;
}

inline bool CBasePlayer::ShouldWeaponSwitch()
{
	return 0 != m_iAutoWepSwitch;
};

inline void CBasePlayer::SetDisplayGamemode(float fTime)
{
	m_fDisplayGamemode = gpGlobals->time + fTime;
};


inline void CBasePlayer::SetSpawnFull(bool bSpawnFull)
{
	m_bSpawnFull = bSpawnFull;
}

inline bool CBasePlayer::GetSpawnFull()
{
	return m_bSpawnFull;
}

inline bool CBasePlayer::DisableSpecs()
{
	return 0 != m_iDisableSpecs;
}

// Spectator Movement modes (stored in pev->iuser1, so the physics code can get at them)
#define OBS_NONE				0
#define OBS_CHASE_LOCKED		1
#define OBS_CHASE_FREE			2
#define OBS_ROAMING				3		
#define OBS_IN_EYE				4
#define OBS_MAP_FREE			5
#define OBS_MAP_CHASE			6

//-- Martin Webrant


#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int gmsgHudText;
extern BOOL gInitHUD;

#endif // PLAYER_H
