//++ BulliT

#if !defined(AFX_AGCOMMANDS_H__B6D8EF5B_9423_4422_B935_1D71B6146DCA__INCLUDED_)
#define AFX_AGCOMMANDS_H__B6D8EF5B_9423_4422_B935_1D71B6146DCA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "agglobal.h"


class AgCommand  
{
public:
	AgCommand();
	virtual ~AgCommand();

  static void Init();

  static bool HandleCommand(CBasePlayer* pPlayer);

  static void AddAdmin(const AgString& sAdmin, const AgString& sPassword, CBasePlayer* pPlayer = NULL);
  static void ListAdmins(CBasePlayer* pPlayer = NULL);
  static void DelAdmin(const AgString& sAdmin,CBasePlayer* pPlayer = NULL);

  static void Start(const AgString& sSpawn);
  static void Allow(const AgString& sPlayerIdOrName,CBasePlayer* pPlayer = NULL);
  static void Abort(CBasePlayer* pPlayer);
  static void Pause(CBasePlayer* pPlayer);

  static void Kick(const AgString& sPlayerIdOrName);
  static void Map(const AgString& sMap);
  static void NextMap(const AgString& sMap, CBasePlayer* pPlayer = NULL);

  static void Setting(const AgString& sSetting, const AgString& sValue, CBasePlayer* pPlayer = NULL);

  static void Help(CBasePlayer* pPlayer);
  static void Variables(CBasePlayer* pPlayer);

  static void TeamUp(CBasePlayer* pPlayer, const AgString& sPlayerIdOrName, const AgString& sTeam);
  static void Spectator(CBasePlayer* pPlayer, const AgString& sPlayerIdOrName);

  static void Exec(const AgString& sExec, CBasePlayer* pPlayer);
};

extern DLL_GLOBAL AgCommand Command;

#endif // !defined(AFX_AGCOMMANDS_H__B6D8EF5B_9423_4422_B935_1D71B6146DCA__INCLUDED_)
//-- Martin Webrant
