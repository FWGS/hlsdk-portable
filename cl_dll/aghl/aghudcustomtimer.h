//++ BulliT

#if !defined(_AG_CUSTOMTIMER_HUD_)
#define _AG_CUSTOMTIMER_HUD_

class AgHudCustomTimer: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);

private:
  float m_flTurnoff;

public:
  void UserCmd_CustomTimer();
};

#endif //_AG_CUSTOMTIMER_HUD_

//-- Martin Webrant
