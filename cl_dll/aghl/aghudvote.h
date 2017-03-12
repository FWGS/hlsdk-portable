//++ BulliT

#if !defined(_AG_VOTE_HUD_)
#define _AG_VOTE_HUD_

class AgHudVote: public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw(float flTime);
  void Reset(void);
	int MsgFunc_Vote(const char *pszName, int iSize, void *pbuf);

private:
  float m_flTurnoff;

  enum VoteStatus { NotRunning = 0, Called = 1, Accepted = 2, Denied = 3, };

	int m_iVoteStatus;
  int m_iFor;
  int m_iAgainst;
  int m_iUndecided;
  char  m_byVoteStatus;
  char  m_szVote[32];
  char  m_szValue[32];
  char  m_szCalled[32];
};

#endif //_AG_VOTE_HUD_

//-- Martin Webrant
