//++ BulliT

#if !defined(_AG_NEXTMAP_HUD_)
#define _AG_NEXTMAP_HUD_

class AgHudNextmap: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_Nextmap(const char *pszName, int iSize, void *pbuf);

private:
  float m_flTurnoff;
  char  m_szNextmap[32];
};

#endif //_AG_NEXTMAP_HUD_

//-- Martin Webrant
