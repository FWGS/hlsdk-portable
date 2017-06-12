//========= Copyright © 2011, Half-Screwed Team, Released under the "Do whatever you want" license. ============//
//																									  //
// Purpose:																							  //
//																									  //
// $NoKeywords: $																					  //
//====================================================================================================//

//THIS IS CURRENTLY BROKEN IN THE DLL SIDE. IT CAUSED LOTS OF LAG.

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <string.h>
#include <stdio.h>
//#include "mp3.h"

int g_iRoundtime;
int smbSoundCounter = 0; //This will count up, where when it hits 5 it'll play a sound.

DECLARE_MESSAGE(m_Timer, Timer)
int CHudTimer::Init(void)
{
	HOOK_MESSAGE( Timer ); 
	gHUD.AddHudElem(this); 
	return 1; 
};

int CHudTimer::VidInit(void)
{
	gHUD.m_iRoundtime = -1;
	g_hud_timerbg = gHUD.GetSpriteIndex( "nestimerbg" );
	g_hud_timercolon = gHUD.GetSpriteIndex( "nestime" );
 	return 1;
};

int CHudTimer::MsgFunc_Timer( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	int x = READ_LONG();
	g_iRoundtime = x;
	gHUD.m_iRoundtime = x;

	m_iFlags |= HUD_ACTIVE;
 	return 1;
}

int CHudTimer::Draw( float flTime )
{	
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
	return 1; 

	if (!(gHUD.m_iWeaponBits & (1 <<(WEAPON_SUIT)) ))
	return 1; 

	if(g_iRoundtime > 0) //no negative numbers allowed
	{
		if (g_iRoundtime == 120)
		{
			smbSoundCounter += 1; //Start the countup to the super mario sound effect
		}
		if (smbSoundCounter == 5) //Note: the counter will still continue until 120 seconds turns into 119.
		{
			char * songchoice = "media/hurryup.mp3";
			//gMP3.PlayMP3NL( songchoice );
		}
		//Draw BG
		SPR_Set(gHUD.GetSprite(g_hud_timerbg), 255, 255, 255 );
		SPR_Draw(0, (ScreenWidth/2-69), 0, &gHUD.GetSpriteRect(g_hud_timerbg));

		gHUD.DrawHudNumberNES((ScreenWidth/2)-44, 20, DHN_2DIGITS | DHN_PREZERO, g_iRoundtime/60, 255, 255, 255); 
		//Draw the numbers
		SPR_Set(gHUD.GetSprite(g_hud_timercolon), 255, 255, 255 );
		SPR_Draw(0, (ScreenWidth/2)-4, 20, &gHUD.GetSpriteRect(g_hud_timercolon));							
		//Draw a String in this case ':'
		gHUD.DrawHudNumberNES((ScreenWidth/2)+6, 20, DHN_2DIGITS | DHN_PREZERO, g_iRoundtime%60, 255, 255, 255);  
	}
	else //If the round is over write it, add something -- GOAHEAD
	{

	}

		if (smbSoundCounter >= 0 && g_iRoundtime > 120) //If server manually changed the round time or new round, Reset the sound timer.
		{
			smbSoundCounter = 0;
		}

	return 1;
}
