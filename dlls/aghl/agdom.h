//++ muphicks  
// AGDomination mode header file

#if !defined(__AG_DOM_H__)
#define __AG_DOM_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define DOM_TEAM1_NAME "blue"
#define DOM_TEAM2_NAME "red"
#define DOM_NEUTRAL_NAME "neutral"


// This is an exact copy of the CTF itterator. Could inherit but for 
// simplicity at this point copying is easier. May tidy this up later :P

class AgDOMFileItemCache;
class AgDOMFileItem  
{
  char      m_szName[32];
	Vector	  m_vOrigin;
  Vector	  m_vAngles;
  char      m_szData1[64];

public:
	AgDOMFileItem();
	virtual ~AgDOMFileItem();

  void Show();

  friend class AgDOMFileItemCache;
};

class AgDOMFileItemCache  
{
  bool  m_bInitDone;
  typedef list<AgDOMFileItem*>  AgDOMFileItemList;
  AgDOMFileItemList m_lstFileItems;

  void Load(CBasePlayer* pPlayer = NULL);
  void Save(CBasePlayer* pPlayer = NULL);

public:
  AgDOMFileItemCache();
  virtual ~AgDOMFileItemCache();

  
  void     Init();

  void     Add(const AgString& sItem,CBasePlayer* pPlayer);
  void     Del(CBasePlayer* pPlayer);
  void     List(CBasePlayer* pPlayer);
};



class AgDOMControlPoint : public CBaseEntity
{
public:
	void Spawn( void ); // spawn a control point in the map
  void Reset( void ); // reset control point to neutral state
  void ClientDisconnected(CBasePlayer* pPlayer);

  char m_szTeamName[64]; // who controls this point?
  char m_szLocation[64]; 

  float m_fCaptureTime;
  float m_fNextTouch; 
  int  m_iConsecutiveScores; // how many times in a row has same team scored
  CBasePlayer *pCapturingPlayer;


private:
	void Precache ( void );
	void Think( void );
  void ChangeControllingTeam( const char *szTeamName );
  void Touch( CBaseEntity *pOther );
  void Capture(CBasePlayer *pPlayer, const char *szTeamName);
  

};



class AgDOM
{
private:
  int m_iTeam1Score;
  int m_iTeam2Score;

public:
	AgDOM();
	virtual ~AgDOM();

  void Think();
  void ResetControlPoints();
  bool ScoreLimit(void);

  void ClientDisconnected(CBasePlayer* pPlayer);
  void ClientConnected(CBasePlayer* pPlayer);

  void PlayerInitHud(CBasePlayer* pPlayer);
  void SendControlScores(CBasePlayer* pPlayer);

  AgDOMFileItemCache  m_FileItemCache;
};





#endif // !defined(__AG_DOM_H__)
