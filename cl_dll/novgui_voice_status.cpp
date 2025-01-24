//========= Copyright � 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"

#include "cl_util.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "demo.h"
#include "demo_api.h"
#include "novgui_voice_status.h"
#include "r_efx.h"
#include "entity_types.h"

extern int cam_thirdperson;

// ---------------------------------------------------------------------- //
// The voice manager for the client.
// ---------------------------------------------------------------------- //
CVoiceStatus g_VoiceStatus;

CVoiceStatus* GetClientVoiceMgr()
{
	return &g_VoiceStatus;
}

// ---------------------------------------------------------------------- //
// CVoiceStatus.
// ---------------------------------------------------------------------- //
static CVoiceStatus *g_pInternalVoiceStatus = NULL;

int __MsgFunc_VoiceMask(const char *pszName, int iSize, void *pbuf)
{
	if(g_pInternalVoiceStatus)
		g_pInternalVoiceStatus->HandleVoiceMaskMsg(iSize, pbuf);

	return 1;
}

int __MsgFunc_ReqState(const char *pszName, int iSize, void *pbuf)
{
	if(g_pInternalVoiceStatus)
		g_pInternalVoiceStatus->HandleReqStateMsg(iSize, pbuf);

	return 1;
}

int g_BannedPlayerPrintCount;
void ForEachBannedPlayer(char id[16])
{
	char str[256];
	sprintf(str, "BAN %d: %2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X%2X\n",
			g_BannedPlayerPrintCount++,
		 id[0], id[1], id[2], id[3],
		 id[4], id[5], id[6], id[7],
		 id[8], id[9], id[10], id[11],
		 id[12], id[13], id[14], id[15]
	);

	gEngfuncs.pfnConsolePrint(str);
}

void ShowBannedCallback()
{
	if(g_pInternalVoiceStatus)
	{
		g_BannedPlayerPrintCount = 0;
		gEngfuncs.pfnConsolePrint("------- BANNED PLAYERS -------\n");
		g_pInternalVoiceStatus->m_BanMgr.ForEachBannedPlayer(ForEachBannedPlayer);
		gEngfuncs.pfnConsolePrint("------------------------------\n");
	}
}

// ---------------------------------------------------------------------- //
// CVoiceStatus.
// ---------------------------------------------------------------------- //
CVoiceStatus::CVoiceStatus()
{
	m_bBanMgrInitialized = false;
	m_LastUpdateServerState = 0;

	m_bTalking = m_bServerAcked = false;

	m_bServerModEnable = -1;

	m_pchGameDir = NULL;
}

CVoiceStatus::~CVoiceStatus()
{
	g_pInternalVoiceStatus = NULL;

	if(m_pchGameDir)
	{
		if(m_bBanMgrInitialized)
		{
			m_BanMgr.SaveState(m_pchGameDir);
		}

		free(m_pchGameDir);
	}
}

int CVoiceStatus::Init()
{
	// Setup the voice_modenable cvar.
	gEngfuncs.pfnRegisterVariable("voice_modenable", "1", FCVAR_ARCHIVE);

	gEngfuncs.pfnRegisterVariable("voice_clientdebug", "0", 0);

	gEngfuncs.pfnAddCommand("voice_showbanned", ShowBannedCallback);

	if(gEngfuncs.pfnGetGameDirectory())
	{
		m_BanMgr.Init(gEngfuncs.pfnGetGameDirectory());
		m_bBanMgrInitialized = true;
	}

	assert(!g_pInternalVoiceStatus);
	g_pInternalVoiceStatus = this;

	m_VoiceHeadModel = 0;


	gHUD.AddHudElem(this);
	m_iFlags = HUD_ACTIVE;
	HOOK_MESSAGE(VoiceMask);
	HOOK_MESSAGE(ReqState);

	// Cache the game directory for use when we shut down
	const char *pchGameDirT = gEngfuncs.pfnGetGameDirectory();
	m_pchGameDir = (char *)malloc(strlen(pchGameDirT) + 1);
	strcpy(m_pchGameDir, pchGameDirT);

	return 1;
}

int CVoiceStatus::VidInit()
{
	// Figure out the voice head model height.
	m_VoiceHeadModelHeight = 45;
	char *pFile = (char *)gEngfuncs.COM_LoadFile( "scripts/voicemodel.txt", 5, NULL );
	if( pFile )
	{
		char token[4096];

		gEngfuncs.COM_ParseFile( pFile, token );
		if( token[0] >= '0' && token[0] <= '9' )
		{
			m_VoiceHeadModelHeight = (float)atof( token );
		}

		gEngfuncs.COM_FreeFile( pFile );
	}

	m_VoiceHeadModel = gEngfuncs.pfnSPR_Load( "sprites/voiceicon.spr" );

	return TRUE;
}

void CVoiceStatus::Frame(double frametime)
{
	// check server banned players once per second
	if(gEngfuncs.GetClientTime() - m_LastUpdateServerState > 1)
	{
		UpdateServerState(false);
	}
}

void CVoiceStatus::CreateEntities()
{
	if(!m_VoiceHeadModel)
		return;

	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();

	int iOutModel = 0;
	for(int i=0; i < VOICE_MAX_PLAYERS; i++)
	{
		if(!m_VoicePlayers[i])
			continue;

		cl_entity_s *pClient = gEngfuncs.GetEntityByIndex(i+1);

		// Don't show an icon if the player is not in our PVS.
		if(!pClient || pClient->curstate.messagenum < localPlayer->curstate.messagenum)
			continue;

		// Don't show an icon for dead or spectating players (ie: invisible entities).
		if(pClient->curstate.effects & EF_NODRAW)
			continue;

		// Don't show an icon for the local player unless we're in thirdperson mode.
		if(pClient == localPlayer && !cam_thirdperson)
			continue;

		cl_entity_s *pEnt = &m_VoiceHeadModels[iOutModel];
		++iOutModel;

		memset(pEnt, 0, sizeof(*pEnt));

		pEnt->curstate.rendermode = kRenderTransAdd;
		pEnt->curstate.renderamt = 255;
		pEnt->baseline.renderamt = 255;
		pEnt->curstate.renderfx = kRenderFxNoDissipation;
		pEnt->curstate.framerate = 1;
		pEnt->curstate.frame = 0;
		pEnt->model = (struct model_s*)gEngfuncs.GetSpritePointer(m_VoiceHeadModel);
		pEnt->angles[0] = pEnt->angles[1] = pEnt->angles[2] = 0;
		pEnt->curstate.scale = 0.5f;

		pEnt->origin[0] = pEnt->origin[1] = 0;
		pEnt->origin[2] = 45;

		VectorAdd(pEnt->origin, pClient->origin, pEnt->origin);

		// Tell the engine.
		gEngfuncs.CL_CreateVisibleEntity(ET_NORMAL, pEnt);
	}
}

void CVoiceStatus::DrawNoVguiSpeakerIcon(int xpos, int ypos, int playerIndex)
{
	if (!IsPlayerSpeaking(playerIndex))
		return;

	int r, g, b;
	UnpackRGB(r,g,b, RGB_YELLOWISH);
	SPR_Set(m_VoiceHeadModel, r, g, b );
	SPR_DrawAdditive( 0,  xpos, ypos, NULL);
}

int CVoiceStatus::Draw(float time)
{
	char str[256];
	cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();

	int xpos = ScreenWidth - 10;
	int ypos = ScreenHeight / 2;
	int VoiceHeight = 32;

	gHUD.GetAllPlayersInfo();

	if (IsPlayerSpeaking(localPlayer->index))
		DrawNoVguiSpeakerIcon(ScreenWidth - 40, ScreenHeight - gHUD.m_iFontHeight * 3 - 6, localPlayer->index);

	if (gHUD.m_Scoreboard.m_iShowscoresHeld)
		return 1;

	for (int i = 1; i <= MAX_VOICE_SPEAKERS; i++)
	{
		if (!IsPlayerSpeaking(i))
			continue;

		cl_entity_t *pClient = gEngfuncs.GetEntityByIndex(i);

		if (pClient == localPlayer && !CVAR_GET_FLOAT("voice_loopback"))
			continue;

		sprintf(str, "%s", g_PlayerInfoList[i].name);
		int PlNameLength = gHUD.DrawHudStringLen(str);

		int start_xpos = xpos - PlNameLength - 42;
		int icon_xpos = start_xpos + PlNameLength + 8;

		gEngfuncs.pfnFillRGBABlend( start_xpos, ypos, PlNameLength + 42, VoiceHeight, 0, 0, 0, 255 * 0.6 );
		gHUD.DrawHudString(start_xpos + 4, ypos + 6, ScreenWidth, str, 255, 140, 0);

		DrawNoVguiSpeakerIcon(icon_xpos, ypos, i);

		ypos += VoiceHeight + 2;
	}

	return 1;
}

void CVoiceStatus::UpdateSpeakerStatus( int entindex, qboolean bTalking )
{
	cvar_t *pVoiceLoopback = NULL;

	if ( gEngfuncs.pfnGetCvarFloat( "voice_clientdebug" ) )
	{
		char msg[256];
		sprintf( msg, "CVoiceStatus::UpdateSpeakerStatus: ent %d talking = %d\n", entindex, bTalking );
		gEngfuncs.pfnConsolePrint( msg );
	}

	int iLocalPlayerIndex = gEngfuncs.GetLocalPlayer()->index;

	// Is it the local player talking?
	if ( entindex == -1 )
	{
		m_bTalking = !!bTalking;
		if( bTalking )
		{
			// Enable voice for them automatically if they try to talk.
			gEngfuncs.pfnClientCmd( "voice_modenable 1" );
		}

		// now set the player index to the correct index for the local player
		// this will allow us to have the local player's icon flash in the scoreboard
		entindex = iLocalPlayerIndex;

		pVoiceLoopback = gEngfuncs.pfnGetCvarPointer( "voice_loopback" );
	}
	else if ( entindex == -2 )
	{
		m_bServerAcked = !!bTalking;
	}

	if ( entindex >= 0 && entindex <= VOICE_MAX_PLAYERS )
	{
		int iClient = entindex - 1;
		if ( iClient < 0 )
		{
			return;
		}

		if ( bTalking )
		{
			m_VoicePlayers[iClient] = true;
			m_VoiceEnabledPlayers[iClient] = true;
		}
		else
		{
			m_VoicePlayers[iClient] = false;

		}
	}
}

void CVoiceStatus::UpdateServerState(bool bForce)
{
	// Can't do anything when we're not in a level.
	char const *pLevelName = gEngfuncs.pfnGetLevelName();
	if( pLevelName[0] == 0 )
	{
		if( gEngfuncs.pfnGetCvarFloat("voice_clientdebug") )
		{
			gEngfuncs.pfnConsolePrint( "CVoiceStatus::UpdateServerState: pLevelName[0]==0\n" );
		}

		return;
	}

	int bCVarModEnable = !!gEngfuncs.pfnGetCvarFloat("voice_modenable");
	if(bForce || m_bServerModEnable != bCVarModEnable)
	{
		m_bServerModEnable = bCVarModEnable;

		char str[256];
		sprintf(str, "VModEnable %d", m_bServerModEnable);
		ServerCmd(str);

		if(gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
		{
			char msg[256];
			sprintf(msg, "CVoiceStatus::UpdateServerState: Sending '%s'\n", str);
			gEngfuncs.pfnConsolePrint(msg);
		}
	}

	char str[2048] = "vban";
	bool bChange = false;

	for(unsigned long dw=0; dw < VOICE_MAX_PLAYERS_DW; dw++)
	{
		unsigned int serverBanMask = 0;
		unsigned int banMask = 0;
		for(unsigned int i=0; i < 32; i++)
		{
			char playerID[16];
			if(!gEngfuncs.GetPlayerUniqueID(i+1, playerID))
				continue;

			if(m_BanMgr.GetPlayerBan(playerID))
				banMask |= 1 << i;

			if(m_ServerBannedPlayers[dw*32 + i])
				serverBanMask |= 1 << i;
		}

		if(serverBanMask != banMask)
			bChange = true;

		// Ok, the server needs to be updated.
		char numStr[512];
		sprintf(numStr, " %x", banMask);
		strcat(str, numStr);
	}

	if(bChange || bForce)
	{
		if(gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
		{
			char msg[256];
			sprintf(msg, "CVoiceStatus::UpdateServerState: Sending '%s'\n", str);
			gEngfuncs.pfnConsolePrint(msg);
		}

		gEngfuncs.pfnServerCmdUnreliable(str);	// Tell the server..
	}
	else
	{
		if (gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
		{
			gEngfuncs.pfnConsolePrint( "CVoiceStatus::UpdateServerState: no change\n" );
		}
	}

	m_LastUpdateServerState = gEngfuncs.GetClientTime();
}

void CVoiceStatus::HandleVoiceMaskMsg(int iSize, void *pbuf)
{
	BEGIN_READ( pbuf, iSize );

	unsigned int dw;
	for(dw=0; dw < VOICE_MAX_PLAYERS_DW; dw++)
	{
		m_AudiblePlayers.SetDWord(dw, (unsigned long)READ_LONG());
		m_ServerBannedPlayers.SetDWord(dw, (unsigned long)READ_LONG());

		if(gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
		{
			char str[256];
			gEngfuncs.pfnConsolePrint("CVoiceStatus::HandleVoiceMaskMsg\n");

			sprintf(str, "    - m_AudiblePlayers[%d] = %lu\n", dw, m_AudiblePlayers.GetDWord(dw));
			gEngfuncs.pfnConsolePrint(str);

			sprintf(str, "    - m_ServerBannedPlayers[%d] = %lu\n", dw, m_ServerBannedPlayers.GetDWord(dw));
			gEngfuncs.pfnConsolePrint(str);
		}
	}

	m_bServerModEnable = READ_BYTE();
}

void CVoiceStatus::HandleReqStateMsg(int iSize, void *pbuf)
{
	if(gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
	{
		gEngfuncs.pfnConsolePrint("CVoiceStatus::HandleReqStateMsg\n");
	}

	UpdateServerState(true);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the target client has been banned
// Input  : playerID -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVoiceStatus::IsPlayerBlocked(int iPlayer)
{
	char playerID[16];
	if (!gEngfuncs.GetPlayerUniqueID(iPlayer, playerID))
		return false;

	return m_BanMgr.GetPlayerBan(playerID);
}

//-----------------------------------------------------------------------------
// Purpose: returns true if the player can't hear the other client due to game rules (eg. the other team)
// Input  : playerID -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CVoiceStatus::IsPlayerAudible(int iPlayer)
{
	return !!m_AudiblePlayers[iPlayer-1];
}

bool CVoiceStatus::IsPlayerSpeaking(int iPlayerIndex)
{
	assert(iPlayerIndex >= 1 && iPlayerIndex <= MAX_PLAYERS);
	return m_VoicePlayers[iPlayerIndex - 1];
}

//-----------------------------------------------------------------------------
// Purpose: blocks/unblocks the target client from being heard
// Input  : playerID -
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
void CVoiceStatus::SetPlayerBlockedState(int iPlayer, bool blocked)
{
	if (gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
	{
		gEngfuncs.pfnConsolePrint( "CVoiceStatus::SetPlayerBlockedState part 1\n" );
	}

	char playerID[16];
	if (!gEngfuncs.GetPlayerUniqueID(iPlayer, playerID))
		return;

	if (gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
	{
		gEngfuncs.pfnConsolePrint( "CVoiceStatus::SetPlayerBlockedState part 2\n" );
	}

	// Squelch or (try to) unsquelch this player.
	if (gEngfuncs.pfnGetCvarFloat("voice_clientdebug"))
	{
		char str[256];
		sprintf(str, "CVoiceStatus::SetPlayerBlockedState: setting player %d ban to %d\n", iPlayer, !m_BanMgr.GetPlayerBan(playerID));
		gEngfuncs.pfnConsolePrint(str);
	}

	m_BanMgr.SetPlayerBan( playerID, blocked );
	UpdateServerState(false);
}
