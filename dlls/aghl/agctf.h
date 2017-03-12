// AgCTF.h: interface for the AgCTF class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(__AG_CTF_H__)
#define __AG_CTF_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CTF_TEAM1_NAME "blue"
#define CTF_TEAM2_NAME "red"


class AgCTFFileItemCache;
class AgCTFFileItem  
{
  char      m_szName[32];
	Vector	  m_vOrigin;
  Vector	  m_vAngles;

public:
	AgCTFFileItem();
	virtual ~AgCTFFileItem();

  void Show();

  friend class AgCTFFileItemCache;
};

class AgCTFFileItemCache  
{
  bool  m_bInitDone;
  typedef list<AgCTFFileItem*>  AgCTFFileItemList;
  AgCTFFileItemList m_lstFileItems;

  void Load(CBasePlayer* pPlayer = NULL);
  void Save(CBasePlayer* pPlayer = NULL);

public:
  AgCTFFileItemCache();
  virtual ~AgCTFFileItemCache();

  
  void     Init();

  void     Add(const AgString& sItem,CBasePlayer* pPlayer);
  void     Del(CBasePlayer* pPlayer);
  void     List(CBasePlayer* pPlayer);
};


class AgCTF  
{
  int m_iTeam1Captures;
  int m_iTeam2Captures;

  int m_iPlayerFlag1;
  int m_iPlayerFlag2;

  enum CTFStatus { Waiting, Countdown, Spawning, Playing};
  CTFStatus m_Status;
  float m_fNextCountdown;
  float m_fMatchStart;
  AgString m_sWinner;

public:
	AgCTF();
	virtual ~AgCTF();

  bool CaptureLimit();
  void ResetCaptures();
  void ResetScore(bool bResetCaptures = true);
  void SendCaptures(CBasePlayer* pPlayer);
  void Think();
  void RoundBasedThink();

  void PlayerInitHud(CBasePlayer* pPlayer);
  void ClientDisconnected(CBasePlayer* pPlayer);
  void ClientConnected(CBasePlayer* pPlayer);
  void PlayerKilled(CBasePlayer* pPlayer,entvars_t *pKiller);
  void AddPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled);

  void PlayerDropFlag(CBasePlayer* pPlayer, bool bPlayerDrop = false);

  void RoundOver(const char* pszWinner);

  AgCTFFileItemCache  m_FileItemCache;
};


class AgCTFFlag : public CBaseEntity
{
public:
	void Spawn( void );

  float m_fNextTouch;
	bool m_bDropped;
	char m_szTeamName[64];
	float m_fNextReset;

  static void ResetFlag(const char *pTeamName);

  void ResetFlag();

private:
	void Precache ( void );
	void Capture(CBasePlayer *pPlayer, const char *pTeamName);
	void Materialize( void );
	void FlagTouch( CBaseEntity *pOther );
	BOOL MyTouch( CBasePlayer *pPlayer );
	void Think( void );
};

class AgCTFPlayerFlag : public CBaseEntity
{
public:
	void Spawn( void );
	virtual void UpdateOnRemove( void );

	CBasePlayer* m_pOwner;
	char m_szTeamName[64];

private:
	void Precache ( void );
	void Think( void );
};




#endif // !defined(__AG_CTF_H__)
