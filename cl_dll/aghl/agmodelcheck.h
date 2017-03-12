//++ BulliT

#if !defined(AFX_AGMODELCHECK_H__4596B2F8_81A4_4927_9A82_9637C7F9A3C9__INCLUDED_)
#define AFX_AGMODELCHECK_H__4596B2F8_81A4_4927_9A82_9637C7F9A3C9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "AgGlobal.h"
#include "AgModel.h"

#ifdef AG_USE_CHEATPROTECTION

class AgModelCheck  
{
  typedef set<AgString, less<AgString> > AgCheckedSet;
  AgCheckedSet m_setChecked;
  bool  m_bScannedStandard;

  bool CheckCurrent();

public:
	AgModelCheck();
	virtual ~AgModelCheck();

  bool Check();
  bool CheckOne(const char* pszModel);
};

extern AgModelCheck g_ModelCheck;

#endif //AG_USE_CHEATPROTECTION

#endif // !defined(AFX_AGMODELCHECK_H__4596B2F8_81A4_4927_9A82_9637C7F9A3C9__INCLUDED_)

//-- Martin Webrant
