//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <demo_api.h>
#include "parsemsg.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"


int AgHudScoreboard::Init(void)
{
  m_bShowScoreboard = false;
	m_iFlags = 0;

	gHUD.AddHudElem(this);
	return 1;
};

int AgHudScoreboard::VidInit(void)
{
	m_iFlags |= HUD_ACTIVE;		
	m_iFlags |= HUD_INTERMISSION; // is always drawn during an intermission

  int iSprite = 0;
  iSprite = gHUD.GetSpriteIndex("icon_ctf_score");
	m_IconFlagScore.spr = gHUD.GetSprite(iSprite);
	m_IconFlagScore.rc = gHUD.GetSpriteRect(iSprite);
	m_bShowScoreboard = false;

	return 1;
}

void AgHudScoreboard::Reset(void)
{
}

bool AgHudScoreboard::IsVisible()
{
	return m_bShowScoreboard;
}

void AgHudScoreboard::ShowScoreboard(bool bShow)
{
  if (bShow && gViewPort && gViewPort->m_pScoreBoard)
    gViewPort->m_pScoreBoard->RebuildTeams();
  m_bShowScoreboard = bShow;
}

/* The scoreboard
We have a minimum width of 1-320 - we could have the field widths scale with it?
*/

// X positions
// relative to the side of the scoreboard
#define NAME_RANGE_MIN  20
#define NAME_RANGE_MAX  145
#define KILLS_RANGE_MIN 130
#define KILLS_RANGE_MAX 170
#define DIVIDER_POS		180
#define DEATHS_RANGE_MIN  185
#define DEATHS_RANGE_MAX  210
#define PING_RANGE_MIN	245
#define PING_RANGE_MAX	295

#define SCOREBOARD_WIDTH 320

// Y positions
#define ROW_GAP  13
#define ROW_RANGE_MIN 15
#define ROW_RANGE_MAX ( ScreenHeight - 50 )
#define ROW_TOP 40

#define TEAM_NO				0
#define TEAM_YES			1
#define TEAM_SPECTATORS		2
#define TEAM_BLANK			3

extern int arrHudColor[3];

int AgHudScoreboard::Draw(float fTime)
{
	if (!m_bShowScoreboard)
		return 1;

	// just sort the list on the fly
	// list is sorted first by frags, then by deaths
	float list_slot = 0;
	int xpos_rel = (ScreenWidth - SCOREBOARD_WIDTH) / 2;

	// print the heading line
	int ypos = ROW_TOP + ROW_RANGE_MIN + (list_slot * ROW_GAP);
	int	xpos = NAME_RANGE_MIN + xpos_rel;

	if ( !gHUD.m_Teamplay ) 
		gHUD.DrawHudString( xpos, ypos, NAME_RANGE_MAX + xpos_rel, "Player", arrHudColor[0], arrHudColor[1], arrHudColor[2] );
	else
		gHUD.DrawHudString( xpos, ypos, NAME_RANGE_MAX + xpos_rel, "Teams", arrHudColor[0], arrHudColor[1], arrHudColor[2] );

	gHUD.DrawHudStringReverse( KILLS_RANGE_MAX + xpos_rel, ypos, 0, CHudTextMessage::BufferedLocaliseTextString("#SCORE"), arrHudColor[0], arrHudColor[1], arrHudColor[2] );
	gHUD.DrawHudString( DIVIDER_POS + xpos_rel, ypos, ScreenWidth, "/", arrHudColor[0], arrHudColor[1], arrHudColor[2] );
	gHUD.DrawHudString( DEATHS_RANGE_MIN + xpos_rel + 5, ypos, ScreenWidth, CHudTextMessage::BufferedLocaliseTextString("#DEATHS"), arrHudColor[0], arrHudColor[1], arrHudColor[2] );
	gHUD.DrawHudString( PING_RANGE_MAX + xpos_rel - 35, ypos, ScreenWidth, CHudTextMessage::BufferedLocaliseTextString("#LATENCY"), arrHudColor[0], arrHudColor[1], arrHudColor[2] );

	list_slot += 1.5;
	ypos = ROW_TOP + ROW_RANGE_MIN + (list_slot * ROW_GAP);
	xpos = NAME_RANGE_MIN + xpos_rel;
	FillRGBA( xpos, ypos, PING_RANGE_MAX, 1, arrHudColor[0], arrHudColor[1], arrHudColor[2], 255);  // draw the seperator line
	
	list_slot += 0.8;

  // draw the players, in order,  and restricted to team if set
  for (int iRow = 0; iRow < gViewPort->m_pScoreBoard->m_iRows; iRow++)
  {
	  if (gViewPort->m_pScoreBoard->m_iIsATeam[iRow] == TEAM_BLANK)
	  {
		  continue;
	  }
	  else if (gViewPort->m_pScoreBoard->m_iIsATeam[iRow] && gHUD.m_Teamplay)
    {
      team_info_t* team_info = &g_TeamInfo[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]];
      
      if (0 != iRow)
        list_slot += 0.3;
      ypos = ROW_TOP + ROW_RANGE_MIN + (list_slot * ROW_GAP);
      
      // check we haven't drawn too far down
      if ( ypos > ROW_RANGE_MAX )  // don't draw to close to the lower border
        break;
      
      xpos = NAME_RANGE_MIN + xpos_rel;
      int r = 255, g = 225, b = 55; // draw the stuff kinda yellowish
      r = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][0];
      g = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][1];
      b = iTeamColors[team_info->teamnumber % iNumberOfTeamColors][2];
      if (gViewPort->m_pScoreBoard->m_iIsATeam[iRow] == TEAM_SPECTATORS)
      {
        r = g = b = 100;
        strcpy(team_info->name,CHudTextMessage::BufferedLocaliseTextString( "#Spectators"));
      }
      
       // draw their name (left to right)
      gHUD.DrawHudString( xpos, ypos, NAME_RANGE_MAX + xpos_rel, team_info->name, r, g, b );
      
      // draw kills (right to left)
      xpos = KILLS_RANGE_MAX + xpos_rel;
      gHUD.DrawHudNumberString( xpos, ypos, KILLS_RANGE_MIN + xpos_rel, team_info->frags, r, g, b );
      
      // draw divider
      xpos = DIVIDER_POS + xpos_rel;
      gHUD.DrawHudString( xpos, ypos, xpos + 20, "/", r, g, b );
      
      // draw deaths
      xpos = DEATHS_RANGE_MAX + xpos_rel;
      gHUD.DrawHudNumberString( xpos, ypos, DEATHS_RANGE_MIN + xpos_rel, team_info->deaths, r, g, b );
      
      // draw ping
      // draw ping & packetloss
      static char buf[64];
      sprintf( buf, "%d/%d", (int)team_info->ping,(int)team_info->packetloss);
      xpos = ((PING_RANGE_MAX - PING_RANGE_MIN) / 2) + PING_RANGE_MIN + xpos_rel + 25;
      gHUD.DrawHudStringReverse( xpos, ypos, xpos - 50, buf, r, g, b );

      list_slot++;
    }
    else if (gViewPort->m_pScoreBoard->m_iIsATeam[iRow] == 0)
    {
      //Draw team info
      int nameoffset = 0;
      if (gHUD.m_Teamplay)
        nameoffset = 10;

      hud_player_info_t* pl_info = &g_PlayerInfoList[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]];
      extra_player_info_t* pl_info_extra = &g_PlayerExtraInfo[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]];

		  int ypos = ROW_TOP + ROW_RANGE_MIN + (list_slot * ROW_GAP);

		  // check we haven't drawn too far down
		  if ( ypos > ROW_RANGE_MAX )  // don't draw to close to the lower border
			  break;

		  int xpos = NAME_RANGE_MIN + xpos_rel;
		  int r = 255, g = 255, b = 255;
      r = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][0];
      g = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][1];
      b = iTeamColors[pl_info_extra->teamnumber % iNumberOfTeamColors][2];
      //ScaleColors(r,g,b,135);
	  
		  if (pl_info->thisplayer) // if it is their name, draw it a different color
		  {
        r = g = b = 255;
			  // overlay the background in blue,  then draw the score text over it
			  FillRGBA( NAME_RANGE_MIN + xpos_rel - 5, ypos + 5, PING_RANGE_MAX - 5, ROW_GAP, 0, 0, 255, 70 );
		  }


      if (g_iPlayerFlag1 == gViewPort->m_pScoreBoard->m_iSortedRows[iRow] || g_iPlayerFlag2 == gViewPort->m_pScoreBoard->m_iSortedRows[iRow])
      {
		    SPR_Set(m_IconFlagScore.spr, 200, 200, 200 );
		    SPR_DrawAdditive( 0, xpos - 26, ypos, &m_IconFlagScore.rc );
      }


      static char szName[128];
      if (g_IsSpectator[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]])
				sprintf(szName, "%s  (S)", AgGetRealName(gViewPort->m_pScoreBoard->m_iSortedRows[iRow]).c_str());
      else
        sprintf(szName, "%s", AgGetRealName(gViewPort->m_pScoreBoard->m_iSortedRows[iRow]).c_str());

          /*
      if (g_IsSpectator[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]])
				sprintf(szName, "%s  (S)", pl_info->name);
      else
        sprintf(szName, "%s", pl_info->name);
        */
      /*
      if (g_IsSpectator[gViewPort->m_pScoreBoard->m_iSortedRows[iRow]])
				sprintf(szName, "%s (S) %ld", pl_info->name,pl_info_extra->m_iWonId);
      else
        sprintf(szName, "%s %ld", pl_info->name,pl_info_extra->m_iWonId);
        */

		  // draw their name (left to right)
		  gHUD.DrawHudString( xpos + nameoffset, ypos, NAME_RANGE_MAX + xpos_rel, szName, r, g, b );

		  // draw kills (right to left)
		  xpos = KILLS_RANGE_MAX + xpos_rel;
		  gHUD.DrawHudNumberString( xpos, ypos, KILLS_RANGE_MIN + xpos_rel, pl_info_extra->frags, r, g, b );

		  // draw divider
		  xpos = DIVIDER_POS + xpos_rel;
		  gHUD.DrawHudString( xpos, ypos, xpos + 20, "/", r, g, b );

		  // draw deaths
		  xpos = DEATHS_RANGE_MAX + xpos_rel;
		  gHUD.DrawHudNumberString( xpos, ypos, DEATHS_RANGE_MIN + xpos_rel, pl_info_extra->deaths, r, g, b );

		  // draw ping & packetloss
		  static char buf[64];
		  sprintf( buf, "%d/%d", (int)pl_info->ping,(int)pl_info->packetloss );
		  xpos = ((PING_RANGE_MAX - PING_RANGE_MIN) / 2) + PING_RANGE_MIN + xpos_rel + 25;
		  gHUD.DrawHudStringReverse( xpos, ypos, xpos - 50, buf, r, g, b );

		  list_slot++;
    }
	}

  return 1;
}


//-- Martin Webrant
