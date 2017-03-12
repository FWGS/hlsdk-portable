//++ BulliT

#if !defined(AFX_AGADMINCACHE_H__638C4983_F4E5_4118_B00C_BC595A3C3415__INCLUDED_)
#define AFX_AGADMINCACHE_H__638C4983_F4E5_4118_B00C_BC595A3C3415__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "agadmin.h"
class CBasePlayer;

// Contains list of all admins and ability to save/load/restore and admin.
class AgAdminCache  
{
  typedef list<AgAdmin*> AgAdminList;
  AgAdminList m_lstAdmins;

public:
	AgAdminCache();
	virtual ~AgAdminCache();

  void RestoreAdmin(CBasePlayer* pPlayer);
  void Load();
  void Save(CBasePlayer* pPlayer = NULL);

  void AddAdmin(const AgString& sAdmin, const AgString& sPassword, CBasePlayer* pPlayer = NULL);
  void ListAdmins(CBasePlayer* pPlayer = NULL);
  void DelAdmin(const AgString& sAdmin, CBasePlayer* pPlayer = NULL);

  void Auth(const AgString& sAdmin, const AgString& sPassword, CBasePlayer* pPlayer);
  void Newpass(const AgString& sOldpassword, const AgString& sPassword, CBasePlayer* pPlayer);

};

extern DLL_GLOBAL AgAdminCache AdminCache;

#endif // !defined(AFX_AGADMINCACHE_H__638C4983_F4E5_4118_B00C_BC595A3C3415__INCLUDED_)

//-- Martin Webrant
