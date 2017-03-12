//++ BulliT

#if !defined(_AG_TIMER_HUD_)
#define _AG_TIMER_HUD_

class AgHudTimer: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_Timer(const char *pszName, int iSize, void *pbuf);

  char m_szTime[64];
private:
	long    m_lTimelimit;
  long    m_lEffectiveTime;
  float   m_flTurnoff;

  void    LiveUpdate();
};

#endif //_AG_TIMER_HUD_

//-- Martin Webrant
