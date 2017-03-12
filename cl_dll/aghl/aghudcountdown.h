//++ BulliT

#if !defined(_AG_COUNTDOWN_HUD_)
#define _AG_COUNTDOWN_HUD_

class AgHudCountdown: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
	void Reset(void);
	int MsgFunc_Countdown(const char *pszName, int iSize, void *pbuf);

private:
	char    m_btCountdown;
  char    m_szPlayer1[32];
  char    m_szPlayer2[32];
};

#endif //_AG_COUNTDOWN_HUD_

//-- Martin Webrant
