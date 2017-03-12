//++ BulliT

#if !defined(AFX_AGMATCH_H__AD3BF401_1118_465A_9BF2_699D72665B5C__INCLUDED_)
#define AFX_AGMATCH_H__AD3BF401_1118_465A_9BF2_699D72665B5C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgMatch  
{
  float m_fMatchStart;
  float m_fNextSay;
  float m_fNextHLTV;
  AgString m_sSpawnFlag;
  void MatchStart();

public:
  AgMatch();
  virtual ~AgMatch();

  void Think();

  void Start(const AgString& sSpawn);
  void Abort();
  void Allow(CBasePlayer* pPlayer);
};

#endif // !defined(AFX_AGMATCH_H__AD3BF401_1118_465A_9BF2_699D72665B5C__INCLUDED_)

//-- Martin Webrant
