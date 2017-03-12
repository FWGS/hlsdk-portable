//++ BulliT

#if !defined(_AG_GLOBAL_HUD_)
#define _AG_GLOBAL_HUD_

class AgHudGlobal: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_PlaySound(const char *pszName, int iSize, void *pbuf);
  int MsgFunc_CheatCheck(const char *pszName, int iSize, void *pbuf);
  int MsgFunc_WhString(const char *pszName, int iSize, void *pbuf);
  int MsgFunc_SpikeCheck(const char *pszName, int iSize, void *pbuf);
  int MsgFunc_Gametype(const char *pszName, int iSize, void *pbuf);
  int MsgFunc_AuthID(const char *pszName, int iSize, void *pbuf);
  int MsgFunc_MapList(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_CRC32(const char *pszName, int iSize, void *pbuf);
  void UserCmd_Winamp();
  void UserCmd_ToggleWinamp();
  void UserCmd_ToggleMapBrowser();
  void UserCmd_LoadAuthID();
  void UserCmd_UnloadAuthID();
  void UserCmd_AgRecord();
protected:
	float m_fCheckColor;
};

enum enumGameType { STANDARD = 0, ARENA = 1, LMS = 2, CTF = 3, ARCADE = 4, SGBOW = 5, INSTAGIB = 6};
extern BYTE g_GameType;
inline BYTE AgGametype()
{
  return g_GameType;
};

AgString AgGetAuthID(int iPlayer);
AgString AgGetRealName(int iPlayer);

#endif //_AG_GLOBAL_HUD_

//-- Martin Webrant
