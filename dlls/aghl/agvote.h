//++ BulliT

#if !defined(AFX_AGVOTE_H__9CC79DD3_49A1_42BF_8757_9F250760B2BD__INCLUDED_)
#define AFX_AGVOTE_H__9CC79DD3_49A1_42BF_8757_9F250760B2BD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgVote  
{
protected:
  AgString m_sVote;
  AgString m_sValue;
  AgString m_sCalled;
  AgString m_sAuthID;

  double m_fMaxTime;
  double m_fNextCount;
  double m_fNextVote;

  enum VoteStatus { NotRunning = 0, Called = 1, Accepted = 2, Denied = 3, };

  bool m_bRunning;

public:
	AgVote();
	virtual ~AgVote();

  bool HandleCommand(CBasePlayer* pPlayer);
  
  bool CallVote(CBasePlayer* pPlayer);
  void Think();

  bool ResetVote();

};

#endif // !defined(AFX_AGVOTE_H__9CC79DD3_49A1_42BF_8757_9F250760B2BD__INCLUDED_)

//-- Martin Webrant
