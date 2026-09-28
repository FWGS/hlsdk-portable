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
#if !defined(PLAYER_H)
#define PLAYER_H

#include "pm_materials.h"

#define PLAYER_FATAL_FALL_SPEED		1024// approx 60 feet
#define PLAYER_MAX_SAFE_FALL_SPEED	600// approx 20 feet
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

#define	SOUND_NIGHTVIEW_ON		"items/nightview1.wav"
#define	SOUND_NIGHTVIEW_OFF		"items/nightview1.wav"

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

	float				m_fNextClearTextTime;

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

	float				m_IntoWaterTime;
	float				m_RecoverTime;
	float				m_ClimbWallTime;
	float				m_MonsterCatchTime;

	int					m_bitsHUDDamage;		// Damage bits for the current fame. These get sent to 
										// the hude via the DAMAGE message
	BOOL				m_fInitHUD;				// True when deferred HUD restart msg needs to be sent
	BOOL				m_fGameHUDInitialized;
	int					m_iTrain;				// Train control position
	BOOL				m_fWeapon;				// Set this to FALSE to force a reset of the current weapon HUD info

	EHANDLE				m_pTank;				// the tank which the player is currently controlling,  NULL if no tank
	float				m_fDeadTime;			// the time at which the player died  (used in PlayerDeathThink())
	float				m_fGameOverTime;

	BOOL			m_fNoPlayerSound;	// a debugging feature. Player makes no sound if this is true. 
	BOOL			m_fLongJump; // does this player have the longjump module?

	BOOL			m_fMask; // ����ַ����������������

	EHANDLE			m_fMoveItem;	//�����
	EHANDLE			m_fContPoint;
	int				m_fContPoint_type;

	BOOL			m_fControlKey;	//������
	int				m_fGlodenKey;	//���Կ��
	BOOL			m_fBloodlyKey;	//ѪɫԿ��
	BOOL			m_fGenerenKey;	//ͨ�Կ��
	BOOL			m_fSecurityKey;	//��ȫԿ��
	BOOL			m_fGreenCard;	//�ɫ�ſ�
	BOOL			m_fSecurityCard;//��ȫ�ſ�
	BOOL			m_fValve;//���

	BOOL			m_fequip1;
	BOOL			m_fequip2;
	BOOL			m_fequip3;
	BOOL			m_fequip4;
	BOOL			m_fequip5;
	BOOL			m_fequip6;

	BOOL			m_fSecondWorld;	//���Ŀģʽ

	BOOL			m_hasflashlight;	//����

	BOOL			m_fPlayerHideMode;

	BOOL			m_fPlayerUseHolySword;
	Vector			m_sword_aim_origin;

	edict_t				*m_wdoor_mynpc;

	int				m_fDeadRespawn;
	Vector			m_old_Respawn_origin;

	BOOL			m_fSelectMode;
	int				m_fSelectNumber;

	float       m_tSneaking;
	int			m_iUpdateTime;		// stores the number of frame ticks before sending HUD update messages
	int			m_iClientHealth;	// the health currently known by the client.  If this changes, send a new
	int			m_iClientBattery;	// the Battery currently known by the client.  If this changes, send a new
	int			m_iHideHUD;		// the players hud weapon info is to be hidden
	int			m_iClientHideHUD;
	int			m_iFOV;			// field of view
	int			m_iClientFOV;	// client's known FOV

	int			m_iClient_Gameover;	// the health currently known by the client.  If this changes, send a new
	int			m_iClient_NVG;	// the health currently known by the client.  If this changes, send a new

	int			m_iClient_oxyan;	// the health currently known by the client.  If this changes, send a new

	int			m_flash_mode;
	int			m_iUseFlag;
	
	int			m_iClient_mynpc;
	int			m_guard_mynpc;

	// usable player items 
	CBasePlayerItem	*m_rgpPlayerItems[MAX_ITEM_TYPES];
	CBasePlayerItem *m_pActiveItem;
	CBasePlayerItem *m_pClientActiveItem;  // client version of the active item
	CBasePlayerItem *m_pLastItem;

	// shared ammo slots
	int	m_rgAmmo[MAX_AMMO_SLOTS];
	int	m_rgAmmoLast[MAX_AMMO_SLOTS];

	//===================================
	int                  m_newcross_active;
	int                  m_newcross_size;
	int                  m_newcross_ontarget;

   	int                  m_enemy_kills;
	int                  m_ending_frags;
	int                  m_player_diamonds;
	int                  m_game_rate;
	float                m_player_time;
	float                m_player_time_now;

	//��Ҽ�����
	int                  m_kadoma_exp;
	int                  m_kadoma_level;
	int                  m_kadoma_skill;
	
	int				 m_skill_locked;	 // ��
	int				 m_skill_punch;		 // ��ɱ�
	int				 m_skill_longjump;   // ���
	int				 m_skill_defguard;	 // ���
	int				 m_skill_valvesword; // ʥ���
	int				 m_skill_miss;		 // ���
	int				 m_skill_respawn;    // ��
	int				 m_skill_darkhide;   // �����
	int				 m_skill_deathmatch; // ������
	int				 m_skill_goddam;	 // �֮̾Ϣ
	int				 m_skill_wrongdoor;	 // ������
	int				 m_skill_reload;	 // �������

	float                m_skill_respawn_time;

	BOOL				 m_skill_darkhide_on;  // ���ģʽ���
	BOOL				 m_level_up_switch;	   // ����ʾ

	float m_blindUntilTime;
	float m_blindStartTime;
	float m_blindHoldTime;
	float m_blindFadeTime;
	float m_blindAlpha;

	float                m_flNextSoundTime1;//���
	float				 m_flNPCguardTime;//���ʹ�
	float				 m_fldarkhideTime;//���ʹ�

	int					 m_new_spawner;

	int					 m_mode_int1;
	int					 m_mode_int2;
	int					 m_mode_int3;
	float				 m_mode_float1;
	float				 m_mode_float2;
	Vector				 m_mode_origin;

   	BOOL                 m_player_died;

    float                m_needlekilled_time;
	float                m_wrongdoor_time;
	float                m_wrongdoor_cover_time;
	
	float                m_needleuse_time;
	float				 m_swordrecover_time;

	int					 m_trainstuck;
	int					 m_title;
	int					 m_trainning;

	int					 m_flAxeCharge;

	int					 m_needleheal;
	int					 m_needleheal2;

	int					m_grenadeboomidle;
	float				m_grenadeboomtime;

	int					m_grenadeboomidle2;
	float				m_grenadeboomtime2;

	Vector				m_vecClimb;
	int                 m_climbspark;
	int					m_iNVG;

	Vector				m_stuck_origin;
	int					m_stuck_inter;

	float                m_flVelocityModifier;
	float                m_flVelocityModifier2;
	
	int 				m_deadtakedmgkill;

	int			    	m_music_save;

	int 					m_skill_maxarmor;
	int						m_greenpoison;
	int				 		m_godposion;
	int				     	m_darkposion;

	int                 m_hate_player;

	int                 m_load_check;
	int                 m_save_check;
	int                 m_save_allow;

	int                 m_Fast_RTP_Show;

    int                 m_barnacle_RTP;
	int                 m_barnacle_RTP_relase;
	int                 m_barnacle_RTP_bar;
	int                 m_barnacle_RTP_button;
	EHANDLE				m_barnacle_catchme;
	int                 m_barnacle_Level;

	float               m_barnacle_god_time;
	float               m_barnacle_draw_time;

	float               m_concussion_time;

	int                 m_air_oxyan;
	int                 m_air_oxyan_max;
	float               m_air_oxyan_stop_time;
	int                 m_air_show;
	float               m_god_time;

	float			    m_waterstepTime;
	float               m_flDeadTime;
	float               m_gameoveralpha;

	int                 m_in_vehicle;

	EHANDLE				m_boss_find;
	int                 m_boss_pov_time;
	int                 m_boss_on;
	int                 m_boss_type;

	Vector				m_old_teleprort_origin;
	int                 m_teleprort_in_xen;
	//====================================
	//WDoor RPG
	int m_rpg_menu_on;
	int m_rpg_menu_select;
	int m_rpg_menu_select2;
	int m_rpg_menu_select_alpha;
//	int m_rpg_menu_origin;

	int m_rpg_menu_item_e;
	int m_rpg_menu_item_t;
	int m_rpg_menu_item_s;
	int m_rpg_menu_item1;
	int m_rpg_menu_item2;
	int m_rpg_menu_item3;
	int m_rpg_menu_item4;
	int m_rpg_menu_item5;
	int m_rpg_menu_item6;
	int m_rpg_menu_item7;
	int m_rpg_menu_item8;
	int m_rpg_menu_item9;
	int m_rpg_menu_item10;
	int m_rpg_menu_item11;
	int m_rpg_menu_item12;
	int m_rpg_menu_skill1;
	int m_rpg_menu_skill2;
	int m_rpg_menu_skill3;
	int m_rpg_menu_skill4;
	int m_rpg_menu_skill5;
	int m_rpg_menu_skill6;
	int m_rpg_menu_skill7;
	int m_rpg_menu_skill8;
	int m_rpg_menu_skill9;
	int m_rpg_menu_skill10;
	int m_rpg_menu_skill11;
	int m_rpg_menu_skill12;
	int m_rpg_menu_skill_chater;

	EHANDLE				m_team_npc1;
	EHANDLE				m_team_npc2;
	EHANDLE				m_team_npc3;
	EHANDLE				m_team_npc4;
	EHANDLE				m_team_npc5;
	EHANDLE				m_team_npc6;
	EHANDLE				m_team_npc7;
	EHANDLE				m_team_npc8;
	EHANDLE				m_team_npc9;
	EHANDLE				m_team_npc10;
	EHANDLE				m_team_npc11;
	EHANDLE				m_team_npc12;

	EHANDLE				m_team_prot;
	EHANDLE				m_player_camera;
//Wrong Door RPG

	int m_rpg_menu_actor1;

	int m_rpg_menu_actor2;
	float m_rpg_menu_hp2;
	float m_rpg_menu_maxhp2;
	int m_rpg_menu_level2;
	int m_rpg_menu_exp2;
	int m_rpg_menu_maxexp2;
	int m_rpg_menu_status2;

	int m_rpg_menu_actor3;
	float m_rpg_menu_hp3;
	float m_rpg_menu_maxhp3;
	int m_rpg_menu_level3;
	int m_rpg_menu_exp3;
	int m_rpg_menu_maxexp3;
	int m_rpg_menu_status3;

	int m_rpg_menu_actor4;
	float m_rpg_menu_hp4;
	float m_rpg_menu_maxhp4;
	int m_rpg_menu_level4;
	int m_rpg_menu_exp4;
	int m_rpg_menu_maxexp4;
	int m_rpg_menu_status4;

	int m_rpg_menu_actor5;
	float m_rpg_menu_hp5;
	float m_rpg_menu_maxhp5;
	int m_rpg_menu_level5;
	int m_rpg_menu_exp5;
	int m_rpg_menu_maxexp5;
	int m_rpg_menu_status5;

	//================================
	int m_rpg_password_on;
	int m_rpg_password_select;
	int m_rpg_password_light1;
	int m_rpg_password_light2;
	int m_rpg_password_light3;
	int m_rpg_password_light4;
	int m_rpg_password_light5;
	int m_rpg_password_light6;
	int m_rpg_password_light7;
	int m_rpg_password_light8;
	int m_rpg_password_light9;

	Vector				m_vecAutoAim;
	BOOL				m_fOnTarget;
	int					m_iDeaths;
	float				m_flRespawnTimer;	// used in PlayerDeathThink() to make sure players can always respawn

	int m_lastx, m_lasty;  // These are the previous update's crosshair angles, DON"T SAVE/RESTORE

	int m_nCustomSprayFrames;// Custom clan logo frames for this player
	float	m_flNextDecalTime;// next time this player can spray a decal

	char m_szTeamName[TEAM_NAME_LENGTH];

	virtual void Spawn( void );
	//void Pain( void );
	void BOSS_Find( void );

	int Game_Save_SecondData(  );
	int Game_Load_SecondData(  );

	void Blind(float flUntilTime, float flHoldTime, float flFadeTime, int iAlpha);

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
	virtual	BOOL IsPlayer( void ) { return TRUE; }			// Spectators should return FALSE for this, they aren't "players" as far as game logic is concerned

	virtual BOOL IsNetClient( void ) { return TRUE; }		// Bots should return FALSE for this, they can't receive NET messages
															// Spectators should return TRUE for this
	virtual const char *TeamID( void );

	void		ShowVGUIMenu(int iMenuID); // VGUI 

	void 		Clear_SayText( void );

	//Wdoor RPG
	void		TeamMate_add( CBaseMonster *pAllynpc );
	void		TeamMate_expadd( CBaseMonster *pAllynpc , CBaseMonster *pKillnpc);
	void		TeamMate_remove( CBaseMonster *pAllynpc );
	void		TeamMate_GetSkill( CBaseMonster *pMonster );
	void		TeamMate_GetNagamatagi( void );
	void		TeamMate_Nagamatagi_Allclear( int mode );
	void		TeamMate_Nagamatagi_Teleport( int mode );
	void		TeamMate_NPC_add( CBaseMonster *pMonster );
	void		TeamMate_Nagamatagi_RespawnStone( int mode );
	void		TeamMate_Nagamatagi_Switch( int ally1,int ally2 );
	void		TeamMate_Nagamatagi_Switch_Auto( CBaseMonster *pMonster );
	BOOL		HasTeamMate_CanAdd( CBaseMonster *pAllynpc );

	void		MenuItem_add( int iMenu_Item );
	void		MenuItem_use( int iMenu_Item);
	void		MenuItem_drop( int iMenu_Item);
	void		MenuItem_equip( int iMenu_Item);
	void		MenuItem_remove( int iMenu_Item_ID );

	void		PassWordBordUse( int use );

	void		GetGame_Playcvar( void );

	BOOL		HasMenuItem_Full( void );

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

	BOOL			NightViewIsOn( void );
	void			NightViewTurnOn( void );
	void			NightViewTurnOff( void );

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
	BOOL HasPlayerItemFromID( int nID );
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

	float m_flStartCharge;
	float m_flAmmoStartCharge;
	float m_flPlayAftershock;
	float m_flNextAmmoBurn;// while charging, when to absorb another unit of player's ammo?

	// Player ID
	void InitStatusBar( void );
	void UpdateStatusBar( void );
	int m_izSBarState[SBAR_END];
	float m_flNextSBarUpdateTime;
	float m_flStatusBarDisappearDelay;
	char m_SbarString0[SBAR_STRING_SIZE];
	char m_SbarString1[SBAR_STRING_SIZE];

	void SetPrefsFromUserinfo( char *infobuffer );

	float m_flNextChatTime;

	int m_iAutoWepSwitch;

	Vector m_vecLastViewAngles;

	// rain tutorial
//	int	Rain_dripsPerSecond, Rain_ideal_dripsPerSecond, Rain_needsUpdate;
//	float	Rain_randX, Rain_randY, Rain_windX, Rain_windY, Rain_ideal_windX, Rain_ideal_windY, Rain_ideal_randX, Rain_ideal_randY, Rain_endFade, Rain_nextFadeUpdate;

};

#define AUTOAIM_2DEGREES  0.0348994967025
#define AUTOAIM_5DEGREES  0.08715574274766
#define AUTOAIM_8DEGREES  0.1391731009601
#define AUTOAIM_10DEGREES 0.1736481776669

extern int gmsgHudText;
extern BOOL gInitHUD;

#endif // PLAYER_H
