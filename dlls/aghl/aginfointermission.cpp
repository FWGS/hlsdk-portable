//++ BulliT

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "agglobal.h"
#include "aginfointermission.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AgInfoIntermission::AgInfoIntermission()
{
  m_bInitialized = false;
}

AgInfoIntermission::~AgInfoIntermission()
{
  m_arrInfoIntermission.clear();
}

void AgInfoIntermission::Think()
{
  if (!m_bInitialized)
  {
    //Doing a check for two good tommyD maps that he has failed to put valid intermission points on.
    if (0 != strnicmp(STRING(gpGlobals->mapname),"stalkx",6)
      &&0 != strnicmp(STRING(gpGlobals->mapname),"boot_campx",10))
    {
      //Check for intermission points.
	    edict_t* pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "info_intermission" );
	    while ( !FNullEnt( pentFind ) )
	    {
        m_arrInfoIntermission.push_back(pentFind);
		    pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "info_intermission" );
	    }
    }

    //If no intermission points where found, use spawn points.
    if (0 == m_arrInfoIntermission.size())
    {
	    edict_t* pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "info_player_deathmatch" );
	    while ( !FNullEnt( pentFind ) )
	    {
        m_arrInfoIntermission.push_back(pentFind);
		    pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "info_player_deathmatch" );
	    }
    }

    //If no intermission points where found, use spawn points.
    if (0 == m_arrInfoIntermission.size())
    {
	    edict_t* pentFind = FIND_ENTITY_BY_CLASSNAME( NULL, "info_player_start" );
	    while ( !FNullEnt( pentFind ) )
	    {
        m_arrInfoIntermission.push_back(pentFind);
		    pentFind = FIND_ENTITY_BY_CLASSNAME( pentFind, "info_player_start" );
	    }
    }

    m_bInitialized = true;
  }
}

//-- Martin Webrant
