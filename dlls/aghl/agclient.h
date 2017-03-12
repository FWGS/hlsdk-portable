//++ BulliT

#if !defined(AFX_AGCLIENT_H__F5D30437_55C8_4113_B813_61931ACAC42B__INCLUDED_)
#define AFX_AGCLIENT_H__F5D30437_55C8_4113_B813_61931ACAC42B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class AgClient  
{
public:
	AgClient();
	virtual ~AgClient();

  enum say_type { All, Team, Close };
  bool HandleCommand(CBasePlayer* pPlayer);
  void Say(CBasePlayer* pPlayer, say_type Type );
  void Play(CBasePlayer* pPlayer, say_type Type, const char* pszWave);

};

#endif // !defined(AFX_AGCLIENT_H__F5D30437_55C8_4113_B813_61931ACAC42B__INCLUDED_)

//-- Martin Webrant
