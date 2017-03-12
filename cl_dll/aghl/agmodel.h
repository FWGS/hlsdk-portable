//++ BulliT

#if !defined(AFX_AGMODEL_H__EC242BA8_B4C4_45B1_A6E7_1BF186C6B9CF__INCLUDED_)
#define AFX_AGMODEL_H__EC242BA8_B4C4_45B1_A6E7_1BF186C6B9CF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef AG_USE_CHEATPROTECTION

#include "AgGlobal.h"

class AgModel  
{
  bool      m_bCorrupt;
  bool      m_bFoundAndChecked;

	Vector	  m_vMinBounds;
	Vector	  m_vMaxBounds;
	long		  m_iVertexCount;

	Vector    m_vMinBone;
	Vector		m_vMaxBone;
	long			m_iBoneCount;

  void			AddVertex(const Vector &vPoint);
	void			AddBone(const Vector &vPoint);
	void			AddBonesToVertices(void);
	void			ReadModel(const char* szModelName);

public:
	AgModel();
	virtual ~AgModel();

  bool      CheckModel(const char* szModelName);
  bool      IsChecked();

};

#endif //AG_USE_CHEATPROTECTION


#endif // !defined(AFX_AGMODEL_H__EC242BA8_B4C4_45B1_A6E7_1BF186C6B9CF__INCLUDED_)

//-- Martin Webrant
