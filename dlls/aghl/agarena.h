// agarena.h: interface for the AgArena class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AGARENA_H__1929C55A_3034_4C89_8398_1F8243B83499__INCLUDED_)
#define AFX_AGARENA_H__1929C55A_3034_4C89_8398_1F8243B83499__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgArena  
{
  typedef list<int> AgWaitList;
  enum ArenaStatus { Waiting, Countdown, Spawning, Playing, PlayerDied };
  
  AgWaitList  m_lstWaitList;
  ArenaStatus m_Status;

  float m_fNextCountdown;
  float m_fMatchStart;
	float m_fNextSay;

  EHANDLE m_Player1;
  EHANDLE m_Player2;

  AgString m_sWinner;
  
  CBasePlayer* GetPlayer1();
  CBasePlayer* GetPlayer2();

  void Add(CBasePlayer* pPlayer);
  void Remove(CBasePlayer* pPlayer);

public:
	AgArena();
	virtual ~AgArena();

  void Think();

  void Ready(CBasePlayer* pPlayer); 
  void NotReady(CBasePlayer* pPlayer);

  void ClientDisconnected(CBasePlayer* pPlayer);
  void ClientConnected(CBasePlayer* pPlayer);

  bool CanTakeDamage();
  bool CanHaveItem();

};


inline  CBasePlayer* AgArena::GetPlayer1()
{
  if (m_Player1)
    return (CBasePlayer*)(CBaseEntity*)m_Player1;
  else 
    return NULL;
};

inline CBasePlayer* AgArena::GetPlayer2()
{
  if (m_Player2)
    return (CBasePlayer*)(CBaseEntity*)m_Player2;
  else 
    return NULL;
};  

inline bool AgArena::CanTakeDamage()
{
  return Playing == m_Status || PlayerDied == m_Status;
}

inline bool AgArena::CanHaveItem()
{
  return Spawning == m_Status;
}


#endif // !defined(AFX_AGARENA_H__1929C55A_3034_4C89_8398_1F8243B83499__INCLUDED_)
