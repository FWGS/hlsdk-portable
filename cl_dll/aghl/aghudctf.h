//++ BulliT

#if !defined(_AG_CTF_HUD_)
#define _AG_CTF_HUD_


class AgHudCTF: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_CTF(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_CTFSound(const char *pszName, int iSize, void *pbuf);
	int MsgFunc_CTFFlag(const char *pszName, int iSize, void *pbuf);

private:
	typedef struct
	{
    
		HSPRITE spr;
		wrect_t rc;
	} icon_flagstatus_t;
	icon_flagstatus_t m_IconFlagStatus[4];
  enum enumFlagStatus { Off = -1, Home = 0, Stolen = 1, Lost = 2, Carry = 3};
  int  m_iFlagStatus1;
  int  m_iFlagStatus2;
};

extern int g_iPlayerFlag1;
extern int g_iPlayerFlag2;
#endif //_AG_CTF_HUD_

//-- Martin Webrant
