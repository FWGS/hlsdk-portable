// agmsgstat.h: interface for the agmsgstat class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AGMSGSTAT_H__B7E22ED8_5544_445A_9AE5_24DCD9413FF2__INCLUDED_)
#define AFX_AGMSGSTAT_H__B7E22ED8_5544_445A_9AE5_24DCD9413FF2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef AGMSGSTAT
class AgMsgStat  
{
public:
	AgMsgStat();
	virtual ~AgMsgStat();

	int RegUserMsg(const char *pszName, int iSize);
	void MessageBegin(int msg_dest, int msg_type, const float *pOrigin, edict_t *ed);
	void MessageEnd(void);

  void WriteByte(int iValue);
	void WriteChar(int iValue);
	void WriteShort(int iValue);
	void WriteLong(int iValue);
	void WriteAngle(float flValue);
	void WriteCoord(float flValue);
	void WriteString(const char *sz);
	void WriteEntity(int iValue);
	
	void DumpStats();
};

extern AgMsgStat g_MsgStat;

#undef REG_USER_MSG
#define REG_USER_MSG (g_MsgStat.RegUserMsg)

inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin = NULL, edict_t *ed = NULL ) 
{
	g_MsgStat.MessageBegin(msg_dest, msg_type, pOrigin, ed);
}

inline void MESSAGE_BEGIN( int msg_dest, int msg_type, const float *pOrigin, entvars_t *ent ) 
{
	g_MsgStat.MessageBegin(msg_dest, msg_type, pOrigin, ENT(ent));
}

#undef  MESSAGE_END
#define MESSAGE_END		(g_MsgStat.MessageEnd)

#undef WRITE_BYTE
#define WRITE_BYTE		(g_MsgStat.WriteByte)

#undef WRITE_CHAR
#define WRITE_CHAR		(g_MsgStat.WriteChar)

#undef WRITE_SHORT
#define WRITE_SHORT		(g_MsgStat.WriteShort)

#undef WRITE_LONG
#define WRITE_LONG		(g_MsgStat.WriteLong)

#undef WRITE_ANGLE
#define WRITE_ANGLE		(g_MsgStat.WriteAngle)

#undef WRITE_COORD
#define WRITE_COORD		(g_MsgStat.WriteCoord)

#undef WRITE_STRING
#define WRITE_STRING	(g_MsgStat.WriteString)

#undef WRITE_ENTITY
#define WRITE_ENTITY	(g_MsgStat.WriteEntity)


#endif AGMSGSTAT

#endif // !defined(AFX_AGMSGSTAT_H__B7E22ED8_5544_445A_9AE5_24DCD9413FF2__INCLUDED_)
