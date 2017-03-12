//++ BulliT

#if !defined(_AG_SUDDENDEATH_HUD_)
#define _AG_SUDDENDEATH_HUD_

class AgHudSuddenDeath: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_SuddenDeath(const char *pszName, int iSize, void *pbuf);

private:
  char  m_bySuddenDeath;
};

#endif //_AG_SUDDENDEATH_HUD_

//-- Martin Webrant
