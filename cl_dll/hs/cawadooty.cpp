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
// Health.cpp
//
// implementation of CHudCOD class
//

#include "STDIO.H"
#include "STDLIB.H"
#include "MATH.H"

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <fstream>
#include <string>
using namespace std;

DECLARE_MESSAGE(m_COD, COD )

#define RANK_DELAY 8.0

int CHudCOD::Init(void)
{
	HOOK_MESSAGE(COD);
	m_iRank = 0;

	gHUD.AddHudElem(this);
	return 1;
}

int CHudCOD::VidInit(void)
{
	m_HUD_codrank = gHUD.GetSpriteIndex( "cod1000" );
	m_HUD_codrank2 = gHUD.GetSpriteIndex( "cod1" );
	m_prc2 = &gHUD.GetSpriteRect(m_HUD_codrank2);
	m_iHeight = m_prc2->bottom - m_prc2->top;
	m_iWidth = (m_prc2->right - m_prc2->left)/2;
	m_fFade = 0;

	int dev = CVAR_GET_FLOAT( "developer" );

	if (dev)
		ConsolePrint("\n\n****#### COD - Rank Scanner -- START\n");

	int i;

	for (i=0;i<101;i++)
	{
		if (RankExists(i))
		{
			if (dev)
			{
				char cod[12] = "FOUND: ";
				char lvl[3] = "";
				itoa(i, lvl, 10);
				strcat(cod, lvl);
				strcat(cod, "\n");
				ConsolePrint( cod );
			}
			Ranks[i] = TRUE;
		}
		else
			Ranks[i] = FALSE;
	}
	Ranks[360] = TRUE;			//Hardcoded NOSCOPE
	Ranks[1000] = TRUE;			//Hardcoded Grinman

	if (dev)
		ConsolePrint("****#### COD - Rank Scanner -- END SUCCESSFULLY\n\n");

	return 1;
}

int CHudCOD::Draw(float flTime)
{
//	wrect_t rc;

	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || gEngfuncs.IsSpectateOnly() )
		return 1;

	if (m_fFade > gHUD.m_flTime)
	{
		if (m_iRank && (Ranks[m_iRank]))
		{
			SPR_Set(gHUD.GetSprite(m_HUD_codrank), 12, 255, 12); 
			SPR_DrawAdditive(0, (ScreenWidth/2)-m_iWidth, (ScreenHeight/2)-((m_iHeight/2)+(m_iHeight)), &gHUD.GetSpriteRect(m_HUD_codrank));
		}
	}

	return 1;
}

int CHudCOD:: MsgFunc_COD(const char *pszName,  int iSize, void *pbuf )
{
	// TODO: update local health data
	BEGIN_READ( pbuf, iSize );
	int x = READ_BYTE();

	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if (x != m_iRank)
	{
		m_fFade = gHUD.m_flTime + RANK_DELAY;
		m_iRank = x;

		char lvl[3] = "";
		sprintf(lvl, "%i", m_iRank);

		if (Ranks[m_iRank])
		{
			char temp[256];
			sprintf(temp, "cod/cod%s.wav", lvl);
			PlaySound( temp, 1 );
		}

		char temp2[256];
		sprintf(temp2, "cod%s", lvl);
		charSpriteName = temp2;
		m_HUD_codrank = gHUD.GetSpriteIndex(charSpriteName);
	}

	return 1;
}

int CHudCOD::RankExists(int rank)
{
	char cod[6] = "cod";
	char lvl[3] = "";
	itoa(rank, lvl, 10);
	strcat(cod, lvl);

	if (gHUD.GetSpriteIndex( cod ) > -1)
		return 1;
	else
		return 0;
}