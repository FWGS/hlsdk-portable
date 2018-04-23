//++ BulliT

#if !defined(AFX_AGTIMER_H__699E98F5_E1CB_4F41_8492_F741C0450C4D__INCLUDED_)
#define AFX_AGTIMER_H__699E98F5_E1CB_4F41_8492_F741C0450C4D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgTimer  
{
	cvar_t* m_pmp_timelimit;
	float m_fNextTimerUpdate;
	float m_fLastTimeCheck;
	float m_fEffectiveTime;

public:
	AgTimer();
	virtual ~AgTimer();

	void Think();
};



#endif // !defined(AFX_AGTIMER_H__699E98F5_E1CB_4F41_8492_F741C0450C4D__INCLUDED_)

//-- Martin Webrant
