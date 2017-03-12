//++ BulliT

#if !defined(AFX_AGTIMEOUT_H__699E98F5_E1CB_4F41_8492_F741C0450C4D__INCLUDED_)
#define AFX_AGTIMEOUT_H__699E98F5_E1CB_4F41_8492_F741C0450C4D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgTimeout  
{
  enum enumState { Inactive = 0, Called = 1, Pause = 2, Countdown = 3, DisablePause = 4};
  enumState m_State;
  double m_fDisablePause;
  double m_fTimeout;
  AgStringList m_lstStrings;

  bool CanTimeout(CBasePlayer* pPlayer);
  void AddTimeout(CBasePlayer* pPlayer);

public:
	AgTimeout();
	virtual ~AgTimeout();

	void Think();
  void Timeout(CBasePlayer* pPlayer);
  void Reset();
  void TogglePause(enumState State = DisablePause);
};



#endif // !defined(AFX_AGTIMEOUT_H__699E98F5_E1CB_4F41_8492_F741C0450C4D__INCLUDED_)

//-- Martin Webrant

