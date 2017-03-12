//++ BulliT

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "parsemsg.h"
#include "agglobal.h"
#include "agmatchreport.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern cvar_t* g_pcl_matchreport;

AgMatchReport::AgMatchReport()
{

}

AgMatchReport::~AgMatchReport()
{

}

void AgMatchReport::Save()
{
  if (0 == g_pcl_matchreport->value)
    return;

  char	szFile[MAX_PATH];
  sprintf(szFile,"%s/matchreport/%s.txt",AgGetDirectory(),AgMapname().c_str());
  FILE* pFile = fopen(szFile,"a");
  if (!pFile)
  {
    // file error
    return;
  }

  //Write the scores.
  if (gHUD.m_Teamplay)
  {
    fputs("Team/player        \tFrags\tDeaths\tPing\tLoss\tAuthID\r\n",pFile);

    //List teams and frags.
    for (int i = 1; i <= MAX_TEAMS; i++)
    {
      if (0 != strlen(g_TeamInfo[i].name))
      {
        fprintf(pFile,"%-20s\t%d\t%d\t%d\t%d\t\r\n",(const char*)g_TeamInfo[i].name,g_TeamInfo[i].frags,g_TeamInfo[i].deaths,g_TeamInfo[i].ping,g_TeamInfo[i].packetloss);

        //Print the players for that one.
        for (int iPlayer = 1; iPlayer <= gEngfuncs.GetMaxClients(); iPlayer++)
        {
          if (g_PlayerExtraInfo[iPlayer].teamname && g_PlayerInfoList[iPlayer].name && 0 == stricmp(g_PlayerExtraInfo[iPlayer].teamname,g_TeamInfo[i].name))
          {
            fprintf(pFile,"%-20s\t%d\t%d\t%d\t%d\t%s\r\n",(const char*)g_PlayerInfoList[iPlayer].name,g_PlayerExtraInfo[iPlayer].frags,g_PlayerExtraInfo[iPlayer].deaths,g_PlayerInfoList[iPlayer].ping,g_PlayerInfoList[iPlayer].packetloss,AgGetAuthID(iPlayer).c_str());
          }
        }
        fputs("\r\n",pFile);
      }
    }
  }
  else
  {
    //List the frags.
    fputs("Player             \tFrags\tDeaths\tPing\tLoss\tAuthID\r\n",pFile);
    for (int iPlayer = 1; iPlayer <= gEngfuncs.GetMaxClients(); iPlayer++)
    {
      if (g_PlayerInfoList[iPlayer].name != NULL && 0 != strlen(g_PlayerInfoList[iPlayer].name))
      {
        fprintf(pFile,"%-20s\t%d\t%d\t%d\t%d\t%s\r\n",(const char*)g_PlayerInfoList[iPlayer].name,g_PlayerExtraInfo[iPlayer].frags,g_PlayerExtraInfo[iPlayer].deaths,g_PlayerInfoList[iPlayer].ping,g_PlayerInfoList[iPlayer].packetloss,AgGetAuthID(iPlayer).c_str());
      }
    }
    fputs("\r\n",pFile);
  }

  //Close it up.
  fflush(pFile);
  fclose(pFile);
}
//-- Martin Webrant
