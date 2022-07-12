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
//			
//  hud.h
//
// class CHud declaration
//
// CHud handles the message, calculation, and drawing the HUD
//
#pragma once
#if !defined(HUD_H)
#define HUD_H
#define RGB_YELLOWISH 0x00FFA000 //255,160,0
#define RGB_REDISH 0x00FF1010 //255,160,0
#define RGB_GREENISH 0x0000A000 //0,160,0

//modif de Julien

#define CONPRINTF gEngfuncs.Con_Printf


#include "wrect.h"
#include "cl_dll.h"
#include "ammo.h"

#define DHN_DRAWZERO 1
#define DHN_2DIGITS  2
#define DHN_3DIGITS  4
#define MIN_ALPHA	 100	

#define		HUDELEM_ACTIVE	1

typedef struct
{
	int x, y;
} POSITION;

enum 
{ 
	MAX_PLAYERS = 64,
	MAX_TEAMS = 64,
	MAX_TEAM_NAME = 16
};

typedef struct
{
	unsigned char r, g, b, a;
} RGBA;

typedef struct cvar_s cvar_t;

#define HUD_ACTIVE			1
#define HUD_INTERMISSION	2
#define HUD_ALWAYSDRAW		4 //HLINVASION Julien

#define MAX_PLAYER_NAME_LENGTH		32

#define	MAX_MOTD_LENGTH				1536

#define MAX_SERVERNAME_LENGTH	64
#define MAX_TEAMNAME_SIZE 32

//
//-----------------------------------------------------
//
class CHudBase
{
public:
	POSITION  m_pos;
	int   m_type;
	int	  m_iFlags; // active, moving, 
	virtual		~CHudBase() {}
	virtual int Init( void ) { return 0; }
	virtual int VidInit( void ) { return 0; }
	virtual int Draw( float flTime ) { return 0; }
	virtual void Think( void ) { return; }
	virtual void Reset( void ) { return; }
	virtual void InitHUDData( void ) {}		// called every time a server is connected to
};

struct HUDLIST
{
	CHudBase	*p;
	HUDLIST		*pNext;
};

//
//-----------------------------------------------------
#if USE_VGUI
#include "voice_status.h" // base voice handling class
#endif
#include "hud_spectator.h"

//
//-----------------------------------------------------
//
class CHudAmmo : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Think( void );
	void Reset( void );
	int DrawWList( float flTime );
	int MsgFunc_CurWeapon( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeaponList( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AmmoX( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AmmoPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_WeapPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ItemPickup( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideWeapon( const char *pszName, int iSize, void *pbuf );

	void SlotInput( int iSlot );
	void _cdecl UserCmd_Slot1( void );
	void _cdecl UserCmd_Slot2( void );
	void _cdecl UserCmd_Slot3( void );
	void _cdecl UserCmd_Slot4( void );
	void _cdecl UserCmd_Slot5( void );
	void _cdecl UserCmd_Slot6( void );
	void _cdecl UserCmd_Slot7( void );
	void _cdecl UserCmd_Slot8( void );
	void _cdecl UserCmd_Slot9( void );
	void _cdecl UserCmd_Slot10( void );
	void _cdecl UserCmd_Close( void );
	void _cdecl UserCmd_NextWeapon( void );
	void _cdecl UserCmd_PrevWeapon( void );

private:
	float m_fFade;
	RGBA  m_rgba;
	WEAPON *m_pWeapon;
	int m_HUD_bucket0;
	int m_HUD_selection;
};

//
//-----------------------------------------------------
//
class CHudAmmoSecondary : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw(float flTime);

	int MsgFunc_SecAmmoVal( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SecAmmoIcon( const char *pszName, int iSize, void *pbuf );

private:
	enum {
		MAX_SEC_AMMO_VALUES = 4
	};

	int m_HUD_ammoicon; // sprite indices
	int m_iAmmoAmounts[MAX_SEC_AMMO_VALUES];
	float m_fFade;
};

#include "health.h"


#define FADE_TIME 100


//
//-----------------------------------------------------
//
class CHudGeiger: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Geiger( const char *pszName, int iSize, void *pbuf );
	
private:
	int m_iGeigerRange;
};

//
//-----------------------------------------------------
//
class CHudTrain : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Train( const char *pszName, int iSize, void *pbuf );


private:
	HSPRITE m_hSprite;
	int m_iPos;
};

#if !USE_VGUI || USE_NOVGUI_MOTD
class CHudMOTD : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	int MsgFunc_MOTD( const char *pszName, int iSize, void *pbuf );
	void Scroll( int dir );
	void Scroll( float amount );
	float scroll;
	bool m_bShow;

protected:
	static int MOTD_DISPLAY_TIME;
	char m_szMOTD[MAX_MOTD_LENGTH];

	int m_iLines;
	int m_iMaxLength;
};
#endif

#if !USE_VGUI || USE_NOVGUI_SCOREBOARD
class CHudScoreboard : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int DrawPlayers( int xoffset, float listslot, int nameoffset = 0, const char *team = NULL ); // returns the ypos where it finishes drawing
	void UserCmd_ShowScores( void );
	void UserCmd_HideScores( void );
	int MsgFunc_ScoreInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamInfo( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScore( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamScores( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf );
	void DeathMsg( int killer, int victim );

	int m_iNumTeams;

	int m_iLastKilledBy;
	int m_fLastKillTime;
	int m_iPlayerNum;
	int m_iShowscoresHeld;

	void GetAllPlayersInfo( void );
};
#endif

//
//-----------------------------------------------------
//
class CHudStatusBar : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	void ParseStatusString( int line_num );

	int MsgFunc_StatusText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_StatusValue( const char *pszName, int iSize, void *pbuf );

protected:
	enum
	{ 
		MAX_STATUSTEXT_LENGTH = 128,
		MAX_STATUSBAR_VALUES = 8,
		MAX_STATUSBAR_LINES = 2
	};

	char m_szStatusText[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];  // a text string describing how the status bar is to be drawn
	char m_szStatusBar[MAX_STATUSBAR_LINES][MAX_STATUSTEXT_LENGTH];	// the constructed bar that is drawn
	int m_iStatusValues[MAX_STATUSBAR_VALUES];  // an array of values for use in the status bar

	int m_bReparseString; // set to TRUE whenever the m_szStatusBar needs to be recalculated

	// an array of colors...one color for each line
	float *m_pflNameColors[MAX_STATUSBAR_LINES];
};

struct extra_player_info_t
{
	short frags;
	short deaths;
	short playerclass;
	short teamnumber;
	char teamname[MAX_TEAM_NAME];
};

struct team_info_t
{
	char name[MAX_TEAM_NAME];
	short frags;
	short deaths;
	short ping;
	short packetloss;
	short ownteam;
	short players;
	int already_drawn;
	int scores_overriden;
	int teamnumber;
};

extern hud_player_info_t	g_PlayerInfoList[MAX_PLAYERS + 1];	   // player info from the engine
extern extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS + 1];   // additional player info sent directly to the client dll
extern team_info_t			g_TeamInfo[MAX_TEAMS + 1];
extern int					g_IsSpectator[MAX_PLAYERS + 1];

//
//-----------------------------------------------------
//
class CHudDeathNotice : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_DeathMsg( const char *pszName, int iSize, void *pbuf );

private:
	int m_HUD_d_skull;  // sprite index of skull icon
};

//
//-----------------------------------------------------
//
class CHudMenu : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_ShowMenu( const char *pszName, int iSize, void *pbuf );

	void SelectMenuItem( int menu_item );

	int m_fMenuDisplayed;
	int m_bitsValidSlots;
	float m_flShutoffTime;
	int m_fWaitingForMore;
};

//
//-----------------------------------------------------
//
class CHudSayText : public CHudBase
{
public:
	int Init( void );
	void InitHUDData( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_SayText( const char *pszName, int iSize, void *pbuf );
	void SayTextPrint( const char *pszBuf, int iBufSize, int clientIndex = -1 );
	void EnsureTextFitsInOneLineAndWrapIfHaveTo( int line );
	friend class CHudSpectator;

private:
	struct cvar_s *	m_HUD_saytext;
	struct cvar_s *	m_HUD_saytext_time;
};

//
//-----------------------------------------------------
//

//HLINVASION
//modif de Julien
#define MAX_NORMAL_BATTERY		100
#define MAX_ARMOR_GROUP			7
#define MAX_MEMBER_ARMOR		MAX_NORMAL_BATTERY / MAX_ARMOR_GROUP
#define ARMOR_PAIN_TIME			0.2

class CHudBattery : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_Battery( const char *pszName,  int iSize, void *pbuf );
	
private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	int m_iBat;
	float m_fFade;
	int m_iHeight;		// width of the battery innards

	// modif de Julien

	int		m_flArmor [MAX_ARMOR_GROUP] [3];
	float	m_flPain [MAX_ARMOR_GROUP];
	float	m_flArmorvalue [MAX_ARMOR_GROUP];

	HSPRITE m_sprCorps;
	wrect_t m_wrcCorps;

};

//
//-----------------------------------------------------
//
class CHudFlashlight: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );
	int MsgFunc_Flashlight( const char *pszName,  int iSize, void *pbuf );
	int MsgFunc_FlashBat( const char *pszName,  int iSize, void *pbuf );

private:
	HSPRITE m_hSprite1;
	HSPRITE m_hSprite2;
	HSPRITE m_hBeam;
	wrect_t *m_prc1;
	wrect_t *m_prc2;
	wrect_t *m_prcBeam;
	float m_flBat;	
	int m_iBat;	
	int m_fOn;
	float m_fFade;
	int m_iWidth;		// width of the battery innards
};

//
//-----------------------------------------------------
//
const int maxHUDMessages = 16;
struct message_parms_t
{
	client_textmessage_t	*pMessage;
	float	time;
	int x, y;
	int	totalWidth, totalHeight;
	int width;
	int lines;
	int lineLength;
	int length;
	int r, g, b;
	int text;
	int fadeBlend;
	float charTime;
	float fadeTime;
};

//
//-----------------------------------------------------
//

class CHudTextMessage : public CHudBase
{
public:
	int Init( void );
	static char *LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size );
	static char *BufferedLocaliseTextString( const char *msg );
	const char *LookupString( const char *msg_name, int *msg_dest = NULL );
	int MsgFunc_TextMsg( const char *pszName, int iSize, void *pbuf );
};

//
//-----------------------------------------------------
//

class CHudMessage : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	int MsgFunc_HudText( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_GameTitle( const char *pszName, int iSize, void *pbuf );

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName, float time );
	void MessageAdd(client_textmessage_t * newMessage );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );

private:
	client_textmessage_t		*m_pMessages[maxHUDMessages];
	float						m_startTime[maxHUDMessages];
	message_parms_t				m_parms;
	float						m_gameTitleTime;
	client_textmessage_t		*m_pGameTitle;

	int m_HUD_title_life;
	int m_HUD_title_half;
};



// modif de Julien
//
// CHudRadio


class CHudRadio : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	int MsgFunc_RadioMsg ( const char *pszName, int iSize, void *pbuf );

	HSPRITE m_sprHead1;
	HSPRITE m_sprHead2;
	HSPRITE m_sprHead3;
	HSPRITE m_sprText1;
	HSPRITE m_sprText2;

	float	m_flStartTime;
	bool	m_bOpen;

	int		m_iHead;
	char	m_cText [256];
};

// modif de Julien
//
// CHudTank


class CHudTank : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	void SetViewPos ( struct ref_params_s *pparams );

	int MsgFunc_TankView ( const char *pszName, int iSize, void *pbuf );

	HSPRITE m_sprTank;

	vec3_t	m_CamPos;
	vec3_t	m_CamAng;
	vec3_t	m_CamVelocity;
	vec3_t	m_CamAVelocity;

	bool	m_iPlayerInTank;
	int		m_iCamEnt;
	float	m_flTankHealth;
	float	m_flPain;

};


// modif de Julien
//
// CHudLensFlare

struct gruntlight_t
{
	int index;
	float length;
	gruntlight_t *pNext;
};

class CHudLensFlare : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	int MsgFunc_LensFlare ( const char *pszName, int iSize, void *pbuf );

	void DrawLight ( void );
	void ClearLights ( void );

	gruntlight_t *m_pLight;
};


// modif de Julien
//
// CHudBriquet

class CHudBriquet : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	int MsgFunc_Briquet ( const char *pszName, int iSize, void *pbuf );

	void DrawFlamme ( void );
};



// modif de Julien
//
// CHudLFlammes


struct flammes_t
{
	int		index;
	float	flBirthTime;

	bool	bXdir;
	bool	bYdir;
	float	angle;
	float	rotspeed;

	int		imode;
	int		flag;

	vec3_t	offset;		// tient lieu d'origine pour les flammmes deco
	vec3_t	velocity;

	flammes_t *pNext;
};

class CHudLFlammes : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	int MsgFunc_LFlammes ( const char *pszName, int iSize, void *pbuf );

	void DrawFlammes ( void );
	void ClearFlammes ( void );

	flammes_t *m_pFlammes;
};



// modif de Julien
//
// CHudFog

class CHudFog : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	int MsgFunc_Fog ( const char *pszName, int iSize, void *pbuf );

	void DrawFog ( void );

	vec3_t	fogcolor;
	float	mindist;
	float	maxdist;

	float	maxfadetime;
	float	fadetime;

	int		Fade;
	int		bActive;
};



// modif de Julien
//
// CHudRPG

enum
{
	RPG_CROSSHAIR_NORMAL = 0,
	RPG_CROSSHAIR_EMPTY,
	RPG_CROSSHAIR_PROCESS,
	RPG_CROSSHAIR_LOCKED,
};

enum
{
	RPG_TEXT_TOUCHE = 4,
	RPG_TEXT_MANQUE,
};



#define RPG_MENU_ROCKET_SELECTED		( 1 << 0 )
#define RPG_MENU_ROCKET_EMPTY			( 1 << 1 )
#define RPG_MENU_ELECTRO_SELECTED		( 1 << 2 )
#define RPG_MENU_ELECTRO_EMPTY			( 1 << 3 )
#define RPG_MENU_NUCLEAR_SELECTED		( 1 << 4 )
#define RPG_MENU_NUCLEAR_EMPTY			( 1 << 5 )
#define RPG_MENU_ACTIVE					( 1 << 6 )
#define RPG_MENU_CLOSE					( 1 << 7 )


// definition de la classe

class CHudRPG : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	int MsgFunc_RpgViseur ( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_MenuRpg ( const char *pszName, int iSize, void *pbuf );

	HSPRITE m_sprCrosshair;
	HSPRITE m_sprMenu;
	HSPRITE m_sprText;

	wrect_t m_rcElectro;
	wrect_t m_rcNuclear;
	wrect_t m_rcRocket;
	wrect_t m_rcSelect;
	wrect_t m_rcEmpty;
	wrect_t m_rcCrosshair;

	int m_iViseurState;
	int m_iMenuState;
	int m_iTextState;

	int m_iAmmo1;
	int m_iAmmo2;
	int m_iAmmo3;

	float m_flSelectTime;
	float m_flTextTime;
};


// modif de Julien
//
// CHudNVG


// definition de la classe

struct nvg_ennemy_t
{
	int index;
	vec3_t color;
	nvg_ennemy_t *pNext;
};

class CHudNVG : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	int Frame( void );

	int MsgFunc_SwitchNVG ( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_InfosNVG ( const char *pszName, int iSize, void *pbuf );

	HSPRITE m_sprNVG;
	HSPRITE m_sprEnnemy;

	float frame;
	float lasttime;
	int nombre;

	void ClearEnnemies ( void );
	nvg_ennemy_t *IsEnnemy ( int index );
	nvg_ennemy_t *pEnnemy;
};


// modif de Roy
class CHudMusic : public CHudBase // This class is simply a message receiver for the music player
{
public: //The methods are implemented in clientmusic.cpp
	int Init( void ); //This gets called from hud.cpp
	int MsgFunc_CMusicOpen ( const char *pszName, int iSize, void *pbuf );
};

// modif de Julien
//
// CHudSniper


// definition de la classe

class CHudSniper : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	HSPRITE m_sprHG;
	HSPRITE m_sprH;
	HSPRITE m_sprHD;
	HSPRITE m_sprBD;
	HSPRITE m_sprB;
	HSPRITE m_sprBG;
	HSPRITE m_sprG;
	HSPRITE m_sprD;

	HSPRITE m_sprViseur;
	HSPRITE m_sprBlack;

	wrect_t m_wrc640;
	wrect_t m_wrc640Viseur;
	wrect_t m_wrc1024;
	wrect_t m_wrcNoir;

};


//
//-------------------------------------------------------------
//
// modif de Julien
//

#define MAX_PARTICULES		256
#define MAX_DECALS			400

#define FLAG_DECAL_WIDEGLASS		( 1 << 0 )
#define FLAG_DECAL_GLASS			( 1 << 1 )
#define FLAG_DECAL_DISK				( 1 << 2 )
#define FLAG_DECAL_SG				( 1 << 3 )
#define FLAG_DECAL_CROWBAR			( 1 << 4 )
#define FLAG_DECAL_CROWBAR_INVERT	( 1 << 5 )
#define FLAG_DECAL_XENO				( 1 << 6 )

#define FLAG_PARTICULE_WOOD			( 1 << 0 )
#define FLAG_PARTICULE_OUTRO1		( 1 << 1 )
#define FLAG_PARTICULE_OUTRO2		( 1 << 2 )
#define FLAG_PARTICULE_OUTRO3		( 1 << 3 )
#define FLAG_PARTICULE_SMOKE		( 1 << 4 )




//structure info particule

typedef struct particule_s
{

	vec3_t origin;
	vec3_t angles;
	vec3_t velocity;
	vec3_t avelocity;

	char model [64];
	float deathtime;
	int flags;

	int libre;

} particule_t;


//structure info decal

typedef struct spritedecal_s
{

	vec3_t origin;
	vec3_t angles;

	char model [64];
	float deathtime;
	int flags;

	int libre;

	int entity;
	vec3_t offset;
	vec3_t angoffset;
	vec3_t oldangles;

	float iRndSensX;
	float iRndSensY;

	float flFrame;


} spritedecal_t;



// definition de la classe

class CHudParticules : public CHudBase
{

public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);

	void DrawAll ( void );

	// messages du serveur

	int MsgFunc_ClientDecal( const char *pszName, int iSize, void *pbuf );



	// ajout de particules ou de decals

	void	AddParticule		( vec3_t origin, vec3_t angles, vec3_t velocity, vec3_t avelocity, char model [64], int deathtime, int flags );
	void	AddDecal			( vec3_t origin, vec3_t angles, char model [64], float deathtime, int flags );

	void	RemoveParticule		( int iIndex );
	void	RemoveDecal			( int iIndex );


	// array particules

	struct particule_s		m_pParticules [ MAX_PARTICULES ];

	// array decals

	struct spritedecal_s	m_pDecals [ MAX_DECALS ];

	// temps

	float m_flLastTime;

};


//----------------------------------
// modif de Julien - d

wrect_t CreateWrect ( int left, int top, int right, int bottom );



//
//-----------------------------------------------------
//
#define MAX_SPRITE_NAME_LENGTH	24

class CHudStatusIcons : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	void Reset( void );
	int Draw( float flTime );
	int MsgFunc_StatusIcon( const char *pszName, int iSize, void *pbuf );

	enum
	{
		MAX_ICONSPRITENAME_LENGTH = MAX_SPRITE_NAME_LENGTH,
		MAX_ICONSPRITES = 4
	};
	
	//had to make these public so CHud could access them (to enable concussion icon)
	//could use a friend declaration instead...
	void EnableIcon( const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue );
	void DisableIcon( const char *pszIconName );

private:
	typedef struct
	{
		char szSpriteName[MAX_ICONSPRITENAME_LENGTH];
		HSPRITE spr;
		wrect_t rc;
		unsigned char r, g, b;
	} icon_sprite_t;

	icon_sprite_t m_IconList[MAX_ICONSPRITES];
};

//
//-----------------------------------------------------
//
class CHud
{
private:
	HUDLIST						*m_pHudList;
	HSPRITE						m_hsprLogo;
	int							m_iLogo;
	client_sprite_t				*m_pSpriteList;
	int							m_iSpriteCount;
	int							m_iSpriteCountAllRes;
	float						m_flMouseSensitivity;
	int							m_iConcussionEffect; 

public:
	HSPRITE						m_hsprCursor;
	float m_flTime;	   // the current client time
	float m_fOldTime;  // the time at which the HUD was last redrawn
	double m_flTimeDelta; // the difference between flTime and fOldTime
	Vector	m_vecOrigin;
	Vector	m_vecAngles;
	int		m_iKeyBits;
	int		m_iHideHUDDisplay;
	int		m_iFOV;
	int		m_Teamplay;
	int		m_iRes;
	cvar_t  *m_pCvarStealMouse;
	cvar_t	*m_pCvarDraw;

	int m_iFontHeight;
	int DrawHudNumber( int x, int y, int iFlags, int iNumber, int r, int g, int b );
	int DrawHudString( int x, int y, int iMaxX, const char *szString, int r, int g, int b );
	int DrawHudStringReverse( int xpos, int ypos, int iMinX, const char *szString, int r, int g, int b );
	int DrawHudNumberString( int xpos, int ypos, int iMinX, int iNumber, int r, int g, int b );
	int GetNumWidth( int iNumber, int iFlags );
	int DrawHudStringLen( const char *szIt );
	void DrawDarkRectangle( int x, int y, int wide, int tall );

private:
	// the memory for these arrays are allocated in the first call to CHud::VidInit(), when the hud.txt and associated sprites are loaded.
	// freed in ~CHud()
	HSPRITE *m_rghSprites;	/*[HUD_SPRITE_COUNT]*/			// the sprites loaded from hud.txt
	wrect_t *m_rgrcRects;	/*[HUD_SPRITE_COUNT]*/
	char *m_rgszSpriteNames; /*[HUD_SPRITE_COUNT][MAX_SPRITE_NAME_LENGTH]*/

	struct cvar_s *default_fov;
public:
	HSPRITE GetSprite( int index ) 
	{
		return ( index < 0 ) ? 0 : m_rghSprites[index];
	}

	wrect_t& GetSpriteRect( int index )
	{
		return m_rgrcRects[index];
	}
	
	int GetSpriteIndex( const char *SpriteName );	// gets a sprite index, for use in the m_rghSprites[] array

	CHudAmmo		m_Ammo;
	CHudHealth		m_Health;
	CHudSpectator		m_Spectator;
	CHudGeiger		m_Geiger;
	CHudBattery		m_Battery;
	CHudTrain		m_Train;
	CHudFlashlight	m_Flash;
	CHudMessage		m_Message;
	CHudStatusBar   m_StatusBar;
	CHudDeathNotice m_DeathNotice;
	CHudSayText		m_SayText;
	CHudMenu		m_Menu;
	CHudAmmoSecondary	m_AmmoSecondary;
	CHudTextMessage m_TextMessage;
	CHudStatusIcons m_StatusIcons;

	// modifs de Julien
	CHudParticules m_Particules;
	CHudSniper m_Sniper;
	CHudNVG m_NVG;
	CHudMusic m_MusicPlayer; //modif de Roy, we need an instance of the music player's message receiver
	CHudRPG m_RPG;
	CHudFog m_Fog;
	CHudLFlammes m_LFlammes;
	CHudBriquet m_Briquet;
	CHudLensFlare m_LensFlare;
	CHudTank m_HudTank;
	CHudRadio m_HudRadio;

#if !USE_VGUI || USE_NOVGUI_SCOREBOARD
	CHudScoreboard	m_Scoreboard;
#endif
#if !USE_VGUI || USE_NOVGUI_MOTD
	CHudMOTD	m_MOTD;
#endif

	void Init( void );
	void VidInit( void );
	void Think(void);
	int Redraw( float flTime, int intermission );
	int UpdateClientData( client_data_t *cdata, float time );

	CHud() : m_iSpriteCount(0), m_pHudList(NULL) {}  
	~CHud();			// destructor, frees allocated memory

	// user messages
	int _cdecl MsgFunc_Damage( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_GameMode( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_Logo( const char *pszName,  int iSize, void *pbuf );
	int _cdecl MsgFunc_ResetHUD( const char *pszName,  int iSize, void *pbuf );
	void _cdecl MsgFunc_InitHUD( const char *pszName, int iSize, void *pbuf );
	void _cdecl MsgFunc_ViewMode( const char *pszName, int iSize, void *pbuf );
	int _cdecl MsgFunc_SetFOV( const char *pszName,  int iSize, void *pbuf );
	int  _cdecl MsgFunc_Concuss( const char *pszName, int iSize, void *pbuf );

	// Screen information
	SCREENINFO	m_scrinfo;

	int	m_iWeaponBits;
	int	m_fPlayerDead;
	int m_iIntermission;

	// sprite indexes
	int m_HUD_number_0;

	int m_iNoConsolePrint;

	void AddHudElem( CHudBase *p );

	float GetSensitivity();

	void GetAllPlayersInfo( void );
};

extern CHud gHUD;

extern int g_iPlayerClass;
extern int g_iTeamNumber;
extern int g_iUser1;
extern int g_iUser2;
extern int g_iUser3;
#endif
