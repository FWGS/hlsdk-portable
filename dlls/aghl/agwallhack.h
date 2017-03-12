//++ BulliT

#if !defined(__AG_WALLHACK_H__)
#define __AG_WALLHACK_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgWallhack
{
  AgString m_sData;

public:
	AgWallhack();
	virtual ~AgWallhack();

  void Init();

  void SendToPlayer(CBasePlayer* pPlayer);
};

extern DLL_GLOBAL AgWallhack Wallhack;

#endif // !__AG_WALLHACK_H__

//-- Martin Webrant
