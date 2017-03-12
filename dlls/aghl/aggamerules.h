//++ BulliT

#if !defined(AFX_AGGAMERULES_H__5F634943_35DD_4A80_922F_A0A82C815D99__INCLUDED_)
#define AFX_AGGAMERULES_H__5F634943_35DD_4A80_922F_A0A82C815D99__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "agscorecache.h"
#include "agadmincache.h"
#include "agglobal.h"
#include "agsettings.h"
#include "agtimer.h"
#include "agmatch.h"
#include "agscorelog.h"
#include "agvote.h"
#include "agclient.h"
#include "agarena.h"
#include "aglms.h"
#include "agctf.h"
#include "agdom.h"
#include "aginfointermission.h"
#include "agsuddendeath.h"
#include "agtimeout.h"
#include "aggamemode.h"
#ifdef AG_NO_CLIENT_DLL
#include "aglocationcache.h"
#endif

class AgGameRules : public CGameRules
{
protected:
  AgString m_sHostname;
  typedef map<int, AgString, less<int> > AgIPAddress;
  AgIPAddress			     m_mapIPAddress;

public:
  AgGameRules();
  virtual ~AgGameRules();
  
  //Overidden
  virtual BOOL ClientCommand(CBasePlayer* pPlayer, const char *pcmd);
  
  virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );
  virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );
  
  virtual BOOL FPlayerCanRespawn(CBasePlayer* pPlayer);
  virtual void PlayerSpawn( CBasePlayer *pPlayer );
  virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );// should the player switch to this weapon?
  virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );

  virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );
  virtual void ClientDisconnected( edict_t *pClient );// a client just disconnected from the server
  
  virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
  virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
  virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );// The player is touching an CBasePlayerItem, do I give it to him?
  virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
  virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );// Called each time a player dies
  virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );  
    
  virtual void ClientUserInfoChanged(CBasePlayer *pPlayer, char *infobuffer);
  virtual void InitHUD( CBasePlayer *pl );
  
  virtual void GoToIntermission();
  
  virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
  
	virtual void RefreshSkillData( void );

  virtual const char *GetGameDescription( void ) { return AgGamename().c_str(); }  // this is the game name that gets seen in the server browser  
  //New
  bool    AgThink();
  void    Start(const AgString& sSpawn);
  void    ChangeNextLevel();
  void    ResendScoreBoard();
  void    HLTV_ResendScoreBoard();
  //AgString GetTeamWithFewestPlayers();
  virtual BOOL IsAllowedToSpawn( const char* pszClass );

  void SendMapListToClient(CBasePlayer* pPlayer, bool bStart);

  const char* GetIPAddress(edict_t *pEntity);
  bool    m_bProxyConnected;
  //Helper classes
  AgScoreCache        m_ScoreCache;
  AgSettings          m_Settings;
  AgTimer             m_Timer;
  AgMatch             m_Match;
  AgScoreLog          m_ScoreLog;
  AgVote              m_Vote;
  AgClient            m_Client;
  AgArena             m_Arena;
  AgLMS               m_LMS;
  AgCTF               m_CTF;
  AgDOM               m_DOM;
  AgInfoIntermission  m_InfoInterMission;
  AgSuddenDeath       m_SuddenDeath;
  AgTimeout           m_Timeout;
#ifdef AG_NO_CLIENT_DLL
  AgLocationCache	  m_LocationCache;
#endif
};




#endif // !defined(AFX_AGGAMERULES_H__5F634943_35DD_4A80_922F_A0A82C815D99__INCLUDED_)

//-- Martin Webrant
