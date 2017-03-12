// agmsgstat.cpp: implementation of the agmsgstat class.
//
//////////////////////////////////////////////////////////////////////
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "gamerules.h"
#include "player.h"
#include "game.h"
#include "agglobal.h"
#include "agmsgstat.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#ifdef AGMSGSTAT


DLL_GLOBAL AgMsgStat g_MsgStat;

class AgMsgStats
{
public:
	AgMsgStats(const AgString& sName, int iSize)
	{
		m_iSize = iSize;
		m_sName = sName;
    m_iBytes = 0;
		memset(&m_iarrDest,0,sizeof(m_iarrDest));
	}
	AgString m_sName;
	int		 m_iSize;
	int		 m_iarrDest[10];
  int    m_iBytes;
	/*
#define	MSG_BROADCAST		0		// unreliable to all
#define	MSG_ONE				1		// reliable to one (msg_entity)
#define	MSG_ALL				2		// reliable to all
#define	MSG_INIT			3		// write to the init string
#define MSG_PVS				4		// Ents in PVS of org
#define MSG_PAS				5		// Ents in PAS of org
#define MSG_PVS_R			6		// Reliable to PVS
#define MSG_PAS_R			7		// Reliable to PAS
#define MSG_ONE_UNRELIABLE	8		// Send to one client, but don't put in reliable stream, put in unreliable datagram ( could be dropped )
#define	MSG_SPEC			9		// Sends to all spectator proxies
	*/
};
typedef map<int, AgMsgStats*, less<int> > AgMsgMap;
static AgMsgMap s_mapMsgs;
static AgMsgStats* s_pCurrentMsg = NULL;
static int s_iStartMsg;

AgMsgStat::AgMsgStat()
{
	s_iStartMsg = 0;

}

AgMsgStat::~AgMsgStat()
{

}

int AgMsgStat::RegUserMsg(const char *pszName, int iSize)
{
	int iMsg = (*g_engfuncs.pfnRegUserMsg)(pszName, iSize);

	if (0 == strcmp("MsgInfo",pszName)) 
		s_iStartMsg = iMsg; //I just wanna log AG's messages too see if I've done anything stupid

	if (iMsg >= s_iStartMsg)
		s_mapMsgs.insert(AgMsgMap::value_type(iMsg, new AgMsgStats(pszName, iSize)));
	return iMsg;
}

void AgMsgStat::MessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed)
{
	//note what type was sent.
	AgMsgMap::iterator itrMsgs = s_mapMsgs.find(msg_type);
	if (itrMsgs != s_mapMsgs.end() /*&& msg_type != 77 && msg_type != 65*/ && msg_type != 105)
	{
		(*itrMsgs).second->m_iarrDest[msg_dest]++;
		AgConsole((*itrMsgs).second->m_sName);
    s_pCurrentMsg = (*itrMsgs).second;
	}
  (*g_engfuncs.pfnMessageBegin)(msg_dest, msg_type, pOrigin, ed);
}

void AgMsgStat::WriteByte(int iValue)
{
  (*g_engfuncs.pfnWriteByte)(iValue);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += sizeof(BYTE);
}

void AgMsgStat::WriteChar(int iValue)
{
  (*g_engfuncs.pfnWriteChar)(iValue);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += sizeof(char);
}

void AgMsgStat::WriteShort(int iValue)
{
  (*g_engfuncs.pfnWriteShort)(iValue);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += sizeof(short);
}

void AgMsgStat::WriteLong(int iValue)
{
  (*g_engfuncs.pfnWriteLong)(iValue);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += sizeof(int);
}

void AgMsgStat::WriteAngle(float flValue)
{
  (*g_engfuncs.pfnWriteAngle)(flValue);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += sizeof(float);
}

void AgMsgStat::WriteCoord(float flValue)
{
  (*g_engfuncs.pfnWriteCoord)(flValue);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += sizeof(float);
}


void AgMsgStat::WriteString(const char *sz)
{
  (*g_engfuncs.pfnWriteString)(sz);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += strlen(sz);
}

void AgMsgStat::WriteEntity(int iValue)
{
  (*g_engfuncs.pfnWriteEntity)(iValue);
  if (s_pCurrentMsg)
    s_pCurrentMsg->m_iBytes += sizeof(int);
}

void AgMsgStat::MessageEnd(void)
{
  (*g_engfuncs.pfnMessageEnd)();
  s_pCurrentMsg = NULL;
}

void AgMsgStat::DumpStats()
{
	char	szFile[MAX_PATH];
	sprintf(szFile,"%s/msgstat.txt",AgGetDirectory());
	FILE* pFile = fopen(szFile,"wb");
	if (!pFile)
		return;
	
	//Dump it.
	for (AgMsgMap::iterator itrMsgs = s_mapMsgs.begin() ;itrMsgs != s_mapMsgs.end(); ++itrMsgs)
	{
		AgMsgStats* pMsg = (*itrMsgs).second;

		int iTotalSent = 0; //Total number of messages sent.
		char szDest[128];
		int iPos = 0;
		
		for (int i = 0; i < (sizeof(pMsg->m_iarrDest) / sizeof(pMsg->m_iarrDest[0])); i++)
		{
			iTotalSent += pMsg->m_iarrDest[i];
			iPos += sprintf(&szDest[iPos], "%d\t", pMsg->m_iarrDest[i]);
		}
		szDest[iPos-1] = '\0';

		//Msg - totalsent - totalsize - sizepermsg - per msgdest
		fprintf(pFile, "%s\t%d\t%d\t%d\t%d\t%s\n", pMsg->m_sName.c_str(), (*itrMsgs).first, iTotalSent, pMsg->m_iBytes, pMsg->m_iSize, szDest);
		AgConsole(UTIL_VarArgs("%s\t%d\t%d\t%d\t%d\t%s", pMsg->m_sName.c_str(), (*itrMsgs).first, iTotalSent, pMsg->m_iBytes, pMsg->m_iSize, szDest));

  //  delete (*itrMsgs).second;
	}
	
	fflush(pFile);
	fclose(pFile);

//  s_mapMsgs.clear();
}

#endif