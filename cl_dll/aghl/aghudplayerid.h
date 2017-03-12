//++ BulliT

#if !defined(_AG_PLAYERID_HUD_)
#define _AG_PLAYERID_HUD_

class AgHudPlayerId: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_PlayerId(const char *pszName, int iSize, void *pbuf);

private:
  float m_flTurnoff;
	int   m_iPlayer;
  bool  m_bTeam;
  int   m_iHealth;
  int   m_iArmour;


};

#endif //_AG_PLAYERID_HUD_

//-- Martin Webrant
