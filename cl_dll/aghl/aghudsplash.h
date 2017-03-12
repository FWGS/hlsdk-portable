//++ BulliT

#if !defined(_AG_SPLASH_HUD_)
#define _AG_SPLASH_HUD_


class AgHudSplash: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_Splash(const char *pszName, int iSize, void *pbuf);

private:
  float m_flTurnoff;
	HSPRITE m_hSprite;
};

#endif //_AG_SPLASH_HUD_

//-- Martin Webrant
