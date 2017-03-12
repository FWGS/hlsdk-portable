//++ BulliT

#if !defined(AFX_AGINFOINTERMISSION_H__92743C98_BDED_4776_ABE6_7FDAA798F87E__INCLUDED_)
#define AFX_AGINFOINTERMISSION_H__92743C98_BDED_4776_ABE6_7FDAA798F87E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgInfoIntermission  
{
  typedef vector<edict_t*>  AgEdictArray;
  AgEdictArray m_arrInfoIntermission;
  bool         m_bInitialized;

public:
	AgInfoIntermission();
	virtual ~AgInfoIntermission();

  void Think();

  int      GetCount();
  edict_t* GetSpot(int iSpot);
  edict_t* GetRandomSpot();
};

inline int AgInfoIntermission::GetCount()
{
  return m_arrInfoIntermission.size();
}

inline edict_t* AgInfoIntermission::GetSpot(int iSpot)
{
  if (iSpot >= 0 && iSpot < (int)m_arrInfoIntermission.size())
    return m_arrInfoIntermission[iSpot];
  return NULL;
}

inline edict_t* AgInfoIntermission::GetRandomSpot()
{
  if (0 == m_arrInfoIntermission.size())
    return NULL;

  return GetSpot(RANDOM_LONG(0, GetCount()-1));
}

#endif // !defined(AFX_AGINFOINTERMISSION_H__92743C98_BDED_4776_ABE6_7FDAA798F87E__INCLUDED_)


//-- Martin Webrant
