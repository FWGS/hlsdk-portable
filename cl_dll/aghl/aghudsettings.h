//++ BulliT

#if !defined(_AG_SETTINGS_HUD_)
#define _AG_SETTINGS_HUD_

class AgHudSettings: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_Settings(const char *pszName, int iSize, void *pbuf);

private:
  float m_flTurnoff;
  int m_iMatch;
  char m_szGamemode[16];
  int m_iTimeLimit;
  int m_iFragLimit;
  int m_iFriendlyFire;
  int m_iWeaponstay;
  char m_szVersion[8];

  char m_szWallgauss[8];
  char m_szHeadShot[8];
  char m_szBlastRadius[8];
};

bool AgIsMatch();

#endif //_AG_SETTINGS_HUD_

//-- Martin Webrant
