//++ BulliT

#if !defined(_AG_TIMEOUT_HUD_)
#define _AG_TIMEOUT_HUD_

class AgHudTimeout: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_Timeout(const char *pszName, int iSize, void *pbuf);

private:
  enum enumState { Inactive = 0, Called = 1, Pause = 2, Countdown = 3 };
  int       m_State;
  int       m_iTime;
};

#endif //_AG_TIMEOUT_HUD_

//-- Martin Webrant
