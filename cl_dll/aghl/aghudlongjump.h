//++ BulliT

#if !defined(_AG_LONGJUMP_HUD_)
#define _AG_LONGJUMP_HUD_

class AgHudLongjump: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_Longjump(const char *pszName, int iSize, void *pbuf);

private:
  float m_flTurnoff;
};

#endif //_AG_LONGJUMP_HUD_

//-- Martin Webrant
