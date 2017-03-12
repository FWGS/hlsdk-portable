//++ BulliT

#if !defined(AFX_AGSCORE_H__7014D216_A0B2_432F_9A67_E54F7D3D6B1D__INCLUDED_)
#define AFX_AGSCORE_H__7014D216_A0B2_432F_9A67_E54F7D3D6B1D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgScore  
{
public:
	AgScore();
	virtual ~AgScore();

	int		        m_iFrags;
	int		        m_iDeaths;
  bool          m_bIngame;
};

#endif // !defined(AFX_AGSCORE_H__7014D216_A0B2_432F_9A67_E54F7D3D6B1D__INCLUDED_)

//-- Martin Webrant
