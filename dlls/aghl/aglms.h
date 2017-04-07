// aglms.h: interface for the AgLMS class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(__AG_LMS_H__)
#define __AG_LMS_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgLMS  
{
  enum LMSStatus { Waiting, Countdown, Spawning, Playing};
  LMSStatus m_Status;
  float m_fNextCountdown;
  float m_fMatchStart;
  float m_fNextSay;
  AgString m_sWinner;

public:
	AgLMS();
	virtual ~AgLMS();

  void Think();

  void ClientDisconnected(CBasePlayer* pPlayer);
  void ClientConnected(CBasePlayer* pPlayer);

  bool CanTakeDamage();
};

inline bool AgLMS::CanTakeDamage()
{
  return Playing == m_Status;
}


#endif // !defined(__AG_LMS_H__)
