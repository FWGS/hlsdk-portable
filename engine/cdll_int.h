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
//  cdll_int.h
//
// 4-23-98  
// JOHN:  client dll interface declarations
//
#pragma once
#if !defined(CDLL_INT_H)
#define CDLL_INT_H

#if __cplusplus
extern "C" {
#endif

#include "const.h"

#define MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	char		name[MAX_ALIAS_NAME];
	char		*value;
} cmdalias_t;

// this file is included by both the engine and the client-dll,
// so make sure engine declarations aren't done twice

typedef int HSPRITE;	// handle to a graphic
typedef int (*pfnUserMsgHook)( const char *pszName, int iSize, void *pbuf );

#include "wrect.h"

#define SCRINFO_SCREENFLASH	1
#define SCRINFO_STRETCHED	2

typedef struct SCREENINFO_s
{
	int		iSize;
	int		iWidth;
	int		iHeight;
	int		iFlags;
	int		iCharHeight;
	short		charWidths[256];
} SCREENINFO;

typedef struct client_data_s
{
	// fields that cannot be modified  (ie. have no effect if changed)
	vec3_t		origin;

	// fields that can be changed by the cldll
	vec3_t		viewangles;
	int		iWeaponBits;
	float		fov;		// field of view
} client_data_t;

typedef struct client_sprite_s
{
	char		szName[64];
	char		szSprite[64];
	int		hspr;
	int		iRes;
	wrect_t		rc;
} client_sprite_t;

typedef struct client_textmessage_s
{
	int		effect;
	byte		r1, g1, b1, a1;	// 2 colors for effects
	byte		r2, g2, b2, a2;
	float		x;
	float		y;
	float		fadein;
	float		fadeout;
	float		holdtime;
	float		fxtime;
	const char	*pName;
	const char	*pMessage;
} client_textmessage_t;

#if _MSC_VER == 1200
#define ulonglong_t __int64
#else
#define ulonglong_t unsigned long long
#endif

typedef struct hud_player_info_s
{
	char		*name;
	short		ping;
	byte		thisplayer;	// TRUE if this is the calling player

	// stuff that's unused at the moment,  but should be done
	byte		spectator;
	byte		packetloss;
	char		*model;
	short		topcolor;
	short		bottomcolor;

	ulonglong_t	m_nSteamID;
} hud_player_info_t;

typedef struct cl_enginefuncs_s
{
	// sprite handlers
	HSPRITE	(*pfnSPR_Load)( const char *szPicName );
	int	(*pfnSPR_Frames)( HSPRITE hPic );
	int	(*pfnSPR_Height)( HSPRITE hPic, int frame );
	int	(*pfnSPR_Width)( HSPRITE hPic, int frame );
	void	(*pfnSPR_Set)( HSPRITE hPic, int r, int g, int b );
	void	(*pfnSPR_Draw)( int frame, int x, int y, const wrect_t *prc );
	void	(*pfnSPR_DrawHoles)( int frame, int x, int y, const wrect_t *prc );
	void	(*pfnSPR_DrawAdditive)( int frame, int x, int y, const wrect_t *prc );
	void	(*pfnSPR_EnableScissor)( int x, int y, int width, int height );
	void	(*pfnSPR_DisableScissor)( void );
	client_sprite_t *(*pfnSPR_GetList)( const char *psz, int *piCount );

	// screen handlers
	void	(*pfnFillRGBA)( int x, int y, int width, int height, int r, int g, int b, int a );
	int	(*pfnGetScreenInfo)( SCREENINFO *pscrinfo );
	void	(*pfnSetCrosshair)( HSPRITE hspr, wrect_t rc, int r, int g, int b );

	// cvar handlers
	struct cvar_s *(*pfnRegisterVariable)( const char *szName, const char *szValue, int flags );
	float	(*pfnGetCvarFloat)( const char *szName );
	char*	(*pfnGetCvarString)( const char *szName );

	// command handlers
	int	(*pfnAddCommand)( const char *cmd_name, void (*function)(void) );
	int	(*pfnHookUserMsg)( const char *szMsgName, pfnUserMsgHook pfn );
	int	(*pfnServerCmd)( const char *szCmdString );
	int	(*pfnClientCmd)( const char *szCmdString );

	void	(*pfnGetPlayerInfo)( int ent_num, hud_player_info_t *pinfo );

	// sound handlers
	void	(*pfnPlaySoundByName)( const char *szSound, float volume );
	void	(*pfnPlaySoundByIndex)( int iSound, float volume );

	// vector helpers
	void	(*pfnAngleVectors)( const float *vecAngles, float *forward, float *right, float *up );

	// text message system
	client_textmessage_t *(*pfnTextMessageGet)( const char *pName );
	int	(*pfnDrawCharacter)( int x, int y, int number, int r, int g, int b );
	int	(*pfnDrawConsoleString)( int x, int y, const char *string );
	void	(*pfnDrawSetTextColor)( float r, float g, float b );
	void	(*pfnDrawConsoleStringLen)(  const char *string, int *length, int *height );

	void	(*pfnConsolePrint)( const char *string );
	void	(*pfnCenterPrint)( const char *string );

	// Added for user input processing
	int	(*GetWindowCenterX)( void );
	int	(*GetWindowCenterY)( void );
	void	(*GetViewAngles)( float * );
	void	(*SetViewAngles)( const float * );
	int	(*GetMaxClients)( void );
	void	(*Cvar_SetValue)( const char *cvar, float value );

	int       (*Cmd_Argc)( void );	
	char	*(*Cmd_Argv)( int arg );
	void	(*Con_Printf)( const char *fmt, ... );
	void	(*Con_DPrintf)( const char *fmt, ... );
	void	(*Con_NPrintf)( int pos, const char *fmt, ... );
	void	(*Con_NXPrintf)( struct con_nprint_s *info, const char *fmt, ... );

	const char* (*PhysInfo_ValueForKey)( const char *key );
	const char* (*ServerInfo_ValueForKey)( const char *key );
	float	(*GetClientMaxspeed)( void );
	int	(*CheckParm)( const char *parm, const char **ppnext );

	void	(*Key_Event)( int key, int down );
	void	(*GetMousePosition)( int *mx, int *my );
	int	(*IsNoClipping)( void );

	struct cl_entity_s *(*GetLocalPlayer)( void );
	struct cl_entity_s *(*GetViewModel)( void );
	struct cl_entity_s *(*GetEntityByIndex)( int idx );

	float	(*GetClientTime)( void );
	void	(*V_CalcShake)( void );
	void	(*V_ApplyShake)( const float *origin, const float *angles, float factor );

	int	(*PM_PointContents)( const float *point, int *truecontents );
	int	(*PM_WaterEntity)( const float *p );
	struct pmtrace_s *(*PM_TraceLine)( const float *start, const float *end, int flags, int usehull, int ignore_pe );

	struct model_s *(*CL_LoadModel)( const char *modelname, int *index );
	int	(*CL_CreateVisibleEntity)( int type, struct cl_entity_s *ent );

	const struct model_s* (*GetSpritePointer)( HSPRITE hSprite );
	void	(*pfnPlaySoundByNameAtLocation)( const char *szSound, float volume, const float *origin );
	
	unsigned short (*pfnPrecacheEvent)( int type, const char* psz );
	void	(*pfnPlaybackEvent)( int flags, const struct edict_s *pInvoker, unsigned short eventindex, float delay, const float *origin, const float *angles, float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 );
	void	(*pfnWeaponAnim)( int iAnim, int body );
	float	(*pfnRandomFloat)( float flLow, float flHigh );	
	int	(*pfnRandomLong)( int lLow, int lHigh );
	void	(*pfnHookEvent)( const char *name, void ( *pfnEvent )( struct event_args_s *args ));
	int	(*Con_IsVisible) ();
	const char *(*pfnGetGameDirectory)( void );
	struct cvar_s *(*pfnGetCvarPointer)( const char *szName );
	const char *(*Key_LookupBinding)( const char *pBinding );
	const char *(*pfnGetLevelName)( void );
	void	(*pfnGetScreenFade)( struct screenfade_s *fade );
	void	(*pfnSetScreenFade)( struct screenfade_s *fade );
	void*	(*VGui_GetPanel)( );
	void	(*VGui_ViewportPaintBackground)( int extents[4] );

	byte*	(*COM_LoadFile)( const char *path, int usehunk, int *pLength );
	char*	(*COM_ParseFile)( const char *data, const char *token );
	void	(*COM_FreeFile)( void *buffer );

	struct triangleapi_s	*pTriAPI;
	struct efx_api_s		*pEfxAPI;
	struct event_api_s		*pEventAPI;	
	struct demo_api_s		*pDemoAPI;
	struct net_api_s		*pNetAPI;
	struct IVoiceTweak_s	*pVoiceTweak;

	// returns 1 if the client is a spectator only (connected to a proxy), 0 otherwise or 2 if in dev_overview mode	
	int	(*IsSpectateOnly)( void );
	struct model_s *(*LoadMapSprite)( const char *filename );

	// file search functions
	void	 (*COM_AddAppDirectoryToSearchPath)( const char *pszBaseDir, const char *appName );
	int	 (*COM_ExpandFilename)( const char *fileName, char *nameOutBuffer, int nameOutBufferSize );

	// User info
	// playerNum is in the range (1, MaxClients)
	// returns NULL if player doesn't exit
	// returns "" if no value is set
	const char *( *PlayerInfo_ValueForKey )( int playerNum, const char *key );
	void	(*PlayerInfo_SetValueForKey )( const char *key, const char *value );

	// Gets a unique ID for the specified player. This is the same even if you see the player on a different server.
	// iPlayer is an entity index, so client 0 would use iPlayer=1.
	// Returns false if there is no player on the server in the specified slot.
	qboolean	(*GetPlayerUniqueID)(int iPlayer, const char playerID[16]);

	// TrackerID access
	int	(*GetTrackerIDForPlayer)(int playerSlot);
	int	(*GetPlayerForTrackerID)(int trackerID);

	// Same as pfnServerCmd, but the message goes in the unreliable stream so it can't clog the net stream
	// (but it might not get there).
	int	( *pfnServerCmdUnreliable )( const char *szCmdString );

	void	(*pfnGetMousePos)( struct tagPOINT *ppt );
	void	(*pfnSetMousePos)( int x, int y );
	void	(*pfnSetMouseEnable)( qboolean fEnable );

	// undocumented interface starts here
	struct cvar_s*	(*pfnGetFirstCvarPtr)( void );
	void*		(*pfnGetFirstCmdFunctionHandle)( void );
	void*		(*pfnGetNextCmdFunctionHandle)( void *cmdhandle );
	const char*	(*pfnGetCmdFunctionName)( void *cmdhandle );
	float		(*pfnGetClientOldTime)( void );
	float		(*pfnGetGravity)( void );
	struct model_s*	(*pfnGetModelByIndex)( int index );
	void		(*pfnSetFilterMode)( int mode ); // same as gl_texsort in original Quake
	void		(*pfnSetFilterColor)( float red, float green, float blue );
	void		(*pfnSetFilterBrightness)( float brightness );
	void		*(*pfnSequenceGet)( const char *fileName, const char *entryName );
	void		(*pfnSPR_DrawGeneric)( int frame, int x, int y, const wrect_t *prc, int blendsrc, int blenddst, int width, int height );
	void		*(*pfnSequencePickSentence)( const char *groupName, int pickMethod, int *entryPicked );
	int		(*pfnDrawString)( int x, int y, const char *str, int r, int g, int b );
	int		(*pfnDrawStringReverse)( int x, int y, const char *str, int r, int g, int b );
	const char	*(*LocalPlayerInfo_ValueForKey)( const char* key );
	int		(*pfnVGUI2DrawCharacter)( int x, int y, int ch, unsigned int font );
	int		(*pfnVGUI2DrawCharacterAdditive)( int x, int y, int ch, int r, int g, int b, unsigned int font );
	unsigned int	(*pfnGetApproxWavePlayLen)( const char *filename );
	void*		(*GetCareerGameUI)( void );	// g-cont. !!!! potential crash-point!
	void		(*Cvar_Set)( const char *name, const char *value );
	int		(*pfnIsPlayingCareerMatch)( void );
	void		(*pfnPlaySoundVoiceByName)( const char *szSound, float volume, int pitch );
	void		(*pfnPrimeMusicStream)( const char *filename, int looping );
	double		(*pfnSys_FloatTime)( void );

	// decay funcs
	void		(*pfnProcessTutorMessageDecayBuffer)( int *buffer, int buflen );
	void		(*pfnConstructTutorMessageDecayBuffer)( int *buffer, int buflen );
	void		(*pfnResetTutorMessageDecayData)( void );

	void		(*pfnPlaySoundByNameAtPitch)( const char *szSound, float volume, int pitch );
	void		(*pfnFillRGBABlend)( int x, int y, int width, int height, int r, int g, int b, int a );
	int		(*pfnGetAppID)( void );
	cmdalias_t	*(*pfnGetAliases)( void );
	void		(*pfnVguiWrap2_GetMouseDelta)( int *x, int *y );

	// added in 2019 update, not documented yet
	int             (*pfnFilteredClientCmd)( const char *cmd );
} cl_enginefunc_t;

#define CLDLL_INTERFACE_VERSION	7

// ********************************************************
// Functions exported by the client .dll
// ********************************************************

// Function type declarations for client exports
typedef int (*INITIALIZE_FUNC)	( struct cl_enginefuncs_s*, int );
typedef void (*HUD_INIT_FUNC)		( void );
typedef int (*HUD_VIDINIT_FUNC)	( void );
typedef int (*HUD_REDRAW_FUNC)	( float, int );
typedef int (*HUD_UPDATECLIENTDATA_FUNC) ( struct client_data_s*, float );
typedef void (*HUD_RESET_FUNC)    ( void );
typedef void (*HUD_CLIENTMOVE_FUNC)( struct playermove_s *ppmove, qboolean server );
typedef void (*HUD_CLIENTMOVEINIT_FUNC)( struct playermove_s *ppmove );
typedef char (*HUD_TEXTURETYPE_FUNC)( char *name );
typedef void (*HUD_IN_ACTIVATEMOUSE_FUNC) ( void );
typedef void (*HUD_IN_DEACTIVATEMOUSE_FUNC)		( void );
typedef void (*HUD_IN_MOUSEEVENT_FUNC)		( int mstate );
typedef void (*HUD_IN_CLEARSTATES_FUNC)		( void );
typedef void (*HUD_IN_ACCUMULATE_FUNC ) ( void );
typedef void (*HUD_CL_CREATEMOVE_FUNC)		( float frametime, struct usercmd_s *cmd, int active );
typedef int (*HUD_CL_ISTHIRDPERSON_FUNC) ( void );
typedef void (*HUD_CL_GETCAMERAOFFSETS_FUNC )( float *ofs );
typedef struct kbutton_s * (*HUD_KB_FIND_FUNC) ( const char *name );
typedef void ( *HUD_CAMTHINK_FUNC )( void );
typedef void ( *HUD_CALCREF_FUNC ) ( struct ref_params_s *pparams );
typedef int	 ( *HUD_ADDENTITY_FUNC ) ( int type, struct cl_entity_s *ent, const char *modelname );
typedef void ( *HUD_CREATEENTITIES_FUNC ) ( void );
typedef void ( *HUD_DRAWNORMALTRIS_FUNC ) ( void );
typedef void ( *HUD_DRAWTRANSTRIS_FUNC ) ( void );
typedef void ( *HUD_STUDIOEVENT_FUNC ) ( const struct mstudioevent_s *event, const struct cl_entity_s *entity );
typedef void ( *HUD_POSTRUNCMD_FUNC ) ( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );
typedef void ( *HUD_SHUTDOWN_FUNC ) ( void );
typedef void ( *HUD_TXFERLOCALOVERRIDES_FUNC )( struct entity_state_s *state, const struct clientdata_s *client );
typedef void ( *HUD_PROCESSPLAYERSTATE_FUNC )( struct entity_state_s *dst, const struct entity_state_s *src );
typedef void ( *HUD_TXFERPREDICTIONDATA_FUNC ) ( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd );
typedef void ( *HUD_DEMOREAD_FUNC ) ( int size, unsigned char *buffer );
typedef int ( *HUD_CONNECTIONLESS_FUNC )( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
typedef	int	( *HUD_GETHULLBOUNDS_FUNC ) ( int hullnumber, float *mins, float *maxs );
typedef void (*HUD_FRAME_FUNC)		( double );
typedef int (*HUD_KEY_EVENT_FUNC ) ( int eventcode, int keynum, const char *pszCurrentBinding );
typedef void (*HUD_TEMPENTUPDATE_FUNC) ( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, 	int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ),	void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ) );
typedef struct cl_entity_s *(*HUD_GETUSERENTITY_FUNC ) ( int index );
typedef void (*HUD_VOICESTATUS_FUNC)(int entindex, qboolean bTalking);
typedef void (*HUD_DIRECTORMESSAGE_FUNC)( int iSize, void *pbuf );
typedef int ( *HUD_STUDIO_INTERFACE_FUNC )( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio );
typedef void (*HUD_CHATINPUTPOSITION_FUNC)( int *x, int *y );
typedef int (*HUD_GETPLAYERTEAM)(int iplayer);
typedef void *(*CLIENTFACTORY)(); // this should be CreateInterfaceFn but that means including interface.h
									// which is a C++ file and some of the client files a C only...
									// so we return a void * which we then do a typecast on later.


// Pointers to the exported client functions themselves
typedef struct
{
	INITIALIZE_FUNC						pInitFunc;
	HUD_INIT_FUNC						pHudInitFunc;
	HUD_VIDINIT_FUNC					pHudVidInitFunc;
	HUD_REDRAW_FUNC						pHudRedrawFunc;
	HUD_UPDATECLIENTDATA_FUNC			pHudUpdateClientDataFunc;
	HUD_RESET_FUNC						pHudResetFunc;
	HUD_CLIENTMOVE_FUNC					pClientMove;
	HUD_CLIENTMOVEINIT_FUNC				pClientMoveInit;
	HUD_TEXTURETYPE_FUNC				pClientTextureType;
	HUD_IN_ACTIVATEMOUSE_FUNC			pIN_ActivateMouse;
	HUD_IN_DEACTIVATEMOUSE_FUNC			pIN_DeactivateMouse;
	HUD_IN_MOUSEEVENT_FUNC				pIN_MouseEvent;
	HUD_IN_CLEARSTATES_FUNC				pIN_ClearStates;
	HUD_IN_ACCUMULATE_FUNC				pIN_Accumulate;
	HUD_CL_CREATEMOVE_FUNC				pCL_CreateMove;
	HUD_CL_ISTHIRDPERSON_FUNC			pCL_IsThirdPerson;
	HUD_CL_GETCAMERAOFFSETS_FUNC		pCL_GetCameraOffsets;
	HUD_KB_FIND_FUNC					pFindKey;
	HUD_CAMTHINK_FUNC					pCamThink;
	HUD_CALCREF_FUNC					pCalcRefdef;
	HUD_ADDENTITY_FUNC					pAddEntity;
	HUD_CREATEENTITIES_FUNC				pCreateEntities;
	HUD_DRAWNORMALTRIS_FUNC				pDrawNormalTriangles;
	HUD_DRAWTRANSTRIS_FUNC				pDrawTransparentTriangles;
	HUD_STUDIOEVENT_FUNC				pStudioEvent;
	HUD_POSTRUNCMD_FUNC					pPostRunCmd;
	HUD_SHUTDOWN_FUNC					pShutdown;
	HUD_TXFERLOCALOVERRIDES_FUNC		pTxferLocalOverrides;
	HUD_PROCESSPLAYERSTATE_FUNC			pProcessPlayerState;
	HUD_TXFERPREDICTIONDATA_FUNC		pTxferPredictionData;
	HUD_DEMOREAD_FUNC					pReadDemoBuffer;
	HUD_CONNECTIONLESS_FUNC				pConnectionlessPacket;
	HUD_GETHULLBOUNDS_FUNC				pGetHullBounds;
	HUD_FRAME_FUNC						pHudFrame;
	HUD_KEY_EVENT_FUNC					pKeyEvent;
	HUD_TEMPENTUPDATE_FUNC				pTempEntUpdate;
	HUD_GETUSERENTITY_FUNC				pGetUserEntity;
	HUD_VOICESTATUS_FUNC				pVoiceStatus;		// Possibly null on old client dlls.
	HUD_DIRECTORMESSAGE_FUNC			pDirectorMessage;	// Possibly null on old client dlls.
	HUD_STUDIO_INTERFACE_FUNC			pStudioInterface;	// Not used by all clients
	HUD_CHATINPUTPOSITION_FUNC			pChatInputPosition;	// Not used by all clients
	HUD_GETPLAYERTEAM					pGetPlayerTeam; // Not used by all clients
	CLIENTFACTORY						pClientFactory;
} cldll_func_t;

#if __cplusplus
}
#endif

#endif//CDLL_INT_H
