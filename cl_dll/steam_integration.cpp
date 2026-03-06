#include "steam_integration.h"
#include "cl_dll.h"

#if STEAMWORKS_ENABLED || STEAMWORKS_ENABLED_LEGACY
#include "steam/steam_api.h"

static void OutputDebugString(const char* str)
{
    gEngfuncs.Con_DPrintf("%s", str);
}

#define _ACH_ID( id, name ) { id, #id, name, "", 0, 0 }
struct Achievement_t
{
	int m_eAchievementID;
	const char *m_pchAchievementID;
	char m_rgchName[128];
	char m_rgchDescription[256];
	bool m_bAchieved;
	int m_iIconImage;
};

class CSteamAchievements
{
private:
	int64 m_iAppID; // Our current AppID
	Achievement_t *m_pAchievements; // Achievements data
	int m_iNumAchievements; // The number of Achievements
	bool m_bInitialized; // Have we called Request stats and received the callback?

public:
	CSteamAchievements(Achievement_t *Achievements, int NumAchievements);

	bool RequestStats();
	bool SetAchievement(const char* ID);

	STEAM_CALLBACK( CSteamAchievements, OnUserStatsReceived, UserStatsReceived_t,
		m_CallbackUserStatsReceived );
	STEAM_CALLBACK( CSteamAchievements, OnUserStatsStored, UserStatsStored_t,
		m_CallbackUserStatsStored );
	STEAM_CALLBACK( CSteamAchievements, OnAchievementStored,
		UserAchievementStored_t, m_CallbackAchievementStored );
};

CSteamAchievements::CSteamAchievements(Achievement_t *Achievements, int NumAchievements):
 m_iAppID( 0 ),
 m_bInitialized( false ),
 m_CallbackUserStatsReceived( this, &CSteamAchievements::OnUserStatsReceived ),
 m_CallbackUserStatsStored( this, &CSteamAchievements::OnUserStatsStored ),
 m_CallbackAchievementStored( this, &CSteamAchievements::OnAchievementStored )
{
     m_iAppID = SteamUtils()->GetAppID();
     m_pAchievements = Achievements;
     m_iNumAchievements = NumAchievements;
     RequestStats();
}

bool CSteamAchievements::RequestStats()
{
	// Is Steam loaded? If not we can't get stats.
	if ( NULL == SteamUserStats() || NULL == SteamUser() )
	{
		return false;
	}
	// Is the user logged on?  If not we can't get stats.
	if ( !SteamUser()->BLoggedOn() )
	{
		return false;
	}
	// Request user stats.
	return SteamUserStats()->RequestCurrentStats();
}

bool CSteamAchievements::SetAchievement(const char* ID)
{
	// Have we received a call back from Steam yet?
	if (m_bInitialized)
	{
		bool success = SteamUserStats()->SetAchievement(ID);
		if (success)
			gEngfuncs.Con_DPrintf("Successfully set achievement %s\n", ID);
		else
			gEngfuncs.Con_DPrintf("Couldn't set achievement %s\n", ID);
		return SteamUserStats()->StoreStats();
	}
	// If not then we can't set achievements yet
	return false;
}

void CSteamAchievements::OnUserStatsReceived( UserStatsReceived_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_iAppID == pCallback->m_nGameID )
	{
		gEngfuncs.Con_DPrintf("App ID: %d\n", m_iAppID);
		if ( k_EResultOK == pCallback->m_eResult )
		{
			OutputDebugString("Received stats and achievements from Steam\n");
			m_bInitialized = true;

			// load achievements
			// We don't need achievement descriptions on game side. Leaving it just as an example
			/*for ( int iAch = 0; iAch < m_iNumAchievements; ++iAch )
			{
				Achievement_t &ach = m_pAchievements[iAch];

				SteamUserStats()->GetAchievement(ach.m_pchAchievementID, &ach.m_bAchieved);
				_snprintf( ach.m_rgchName, sizeof(ach.m_rgchName), "%s",
					SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID,
					"name"));
				_snprintf( ach.m_rgchDescription, sizeof(ach.m_rgchDescription), "%s",
					SteamUserStats()->GetAchievementDisplayAttribute(ach.m_pchAchievementID,
					"desc"));
			}*/
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "RequestStats - failed, %d\n", pCallback->m_eResult );
			OutputDebugString( buffer );
		}
	}
}

void CSteamAchievements::OnUserStatsStored( UserStatsStored_t *pCallback )
{
	// we may get callbacks for other games' stats arriving, ignore them
	if ( m_iAppID == pCallback->m_nGameID )
	{
		if ( k_EResultOK == pCallback->m_eResult )
		{
			OutputDebugString( "Stored stats for Steam\n" );
		}
		else
		{
			char buffer[128];
			_snprintf( buffer, 128, "StatsStored - failed, %d\n", pCallback->m_eResult );
			OutputDebugString( buffer );
		}
	}
}

void CSteamAchievements::OnAchievementStored( UserAchievementStored_t *pCallback )
{
     // we may get callbacks for other games' stats arriving, ignore them
     if ( m_iAppID == pCallback->m_nGameID )
     {
          OutputDebugString( "Stored Achievement for Steam\n" );
     }
}

CSteamAchievements*	g_SteamAchievements = NULL;
#endif

void InitSteam()
{
#if STEAMWORKS_ENABLED || STEAMWORKS_ENABLED_LEGACY
    bool bRet = SteamAPI_Init();
    if (bRet)
    {
        OutputDebugString("Steam initialized\n");
        if (g_SteamAchievements == NULL)
        {
            g_SteamAchievements = new CSteamAchievements(NULL, 0);
        }
    }
	else
		OutputDebugString("Couldn't initialize Steam\n");
#endif
}

void ShutdownSteam()
{
#if STEAMWORKS_ENABLED || STEAMWORKS_ENABLED_LEGACY
    SteamAPI_Shutdown();
    if (g_SteamAchievements)
    {
	    delete g_SteamAchievements;
        g_SteamAchievements = NULL;
    }
#endif
}

void SteamRunCallbacks()
{
#if STEAMWORKS_ENABLED || STEAMWORKS_ENABLED_LEGACY
	SteamAPI_RunCallbacks();
#endif
}

void SetAchievement(const char* ID)
{
#if STEAMWORKS_ENABLED || STEAMWORKS_ENABLED_LEGACY
    if (g_SteamAchievements)
	    g_SteamAchievements->SetAchievement(ID);
#endif
}
