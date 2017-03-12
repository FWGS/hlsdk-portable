//++ BulliT

#if !defined(__AG_STATS__)
#define __AG_STATS__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "agglobal.h"

class AgStats  
{
protected:
	class AgPlayerStats
	{
	public:
		class AgWeaponStats
		{
		public:
			AgWeaponStats()
			{
				m_iShots = 0;
        m_iHits = 0;
			}
			int m_iShots;			
			int m_iHits;		
		};

		AgPlayerStats()
		{
			m_iKills = 0;
			m_iDeaths = 0;
			m_iTeamKills = 0;
			m_iDamageTaken = 0;
			m_iDamageGiven = 0;	
		}
		virtual ~AgPlayerStats()
		{
		  for (AgWeaponStatsMap::iterator itrWeaponStats = m_mapWeaponStats.begin() ;itrWeaponStats != m_mapWeaponStats.end(); ++itrWeaponStats)
			  delete (*itrWeaponStats).second;

		  m_mapWeaponStats.clear();
		}

		int m_iKills;				  //Kills
		int m_iDeaths;				//Deaths
		int m_iTeamKills;			//Teamkills
		int m_iDamageTaken;		//Damage taken
    int m_iDamageGiven;   //Damage given
		AgString m_sName;
typedef map<AgString, AgWeaponStats*, less<AgString> > AgWeaponStatsMap;
		AgWeaponStatsMap m_mapWeaponStats;
		AgWeaponStats* GetWeaponStats(const char* pszItem);
	};

typedef map<AgString, AgPlayerStats*, less<AgString> > AgPlayerStatsMap;
	AgPlayerStatsMap m_mapPlayerStats;

	AgPlayerStats* GetStats(CBasePlayer* pPlayer);

  void PrintStats(CBasePlayer* pPlayer, CBasePlayer* pStatsPlayer);
public:
	AgStats();
	virtual ~AgStats();

	void Reset();
    
	void FireShot(CBasePlayer* pPlayer, const char* pszItem);
	void FireHit(CBasePlayer* pPlayer, int iDamage, entvars_t* pAttacker);
	void PlayerKilled(CBasePlayer* pInflictor, CBasePlayer* pKilled);

	bool HandleCommand(CBasePlayer* pPlayer);

  void OnChangeLevel();
};

extern DLL_GLOBAL AgStats Stats;

#endif // !defined(__AG_STATS__)

//-- Martin Webrant
