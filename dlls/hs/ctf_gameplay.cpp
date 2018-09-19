
// includes needed for everything to compile
#include "extdll.h"
#include "decals.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "ctf_gameplay.h"
#include "teamplay_gamerules.h"

LINK_ENTITY_TO_CLASS( object_flag, CObjectFlag );

void CObjectFlag :: Spawn( )
{
    // Calls the precache function to precache any models, sounds,ect you need
    //before it spawns
    Precache( );
    // Setup what model we want to use for the flag, change this to
    // whatever you want. Just remember precache it.
    SET_MODEL( ENT(pev), "models/flag.mdl" );
   
    UTIL_SetOrigin( pev, pev->origin + Vector(0,0,10) );
    UTIL_SetSize( pev, Vector(-16,-16,0), Vector(16,16,16) );

    // This is so it falls to the brush below it
    pev->movetype = MOVETYPE_TOSS;

    // This allows you to trigger it by walking through it
    pev->solid = SOLID_TRIGGER;

    // This just sets our Touch to Touch, which is down a little further
    SetTouch(Touch);

    // This just makes a GlowShell around the model, you can
    // change the colour of the shell by changing the RGB values,
    // which are the XYZ values in this case. So at the moment we have
    // a nice bright red colour. Feel free to change this to your needs.
    pev->renderfx = kRenderFxGlowShell;
    pev->rendercolor.x = 255;
    pev->rendercolor.y = 0;
    pev->rendercolor.z = 0;
    pev->renderamt = 10; // 10 units off the sides of the model

    // This just sets our IsInPlay bool to false, because its only just spawned and
    // no one has picked it up yet.
    m_fIsInPlay = false;
}

// This is the Precache function which is called from the Spawn function.

void CObjectFlag :: Precache( )
{
    // Precache's the model
    PRECACHE_MODEL( "models/flag.mdl" );
}

// This is the Touch function which was set above, in the Spawn function,
// its basically the stuff that runs when the flag has been touched by a player

void CObjectFlag :: Touch(CBaseEntity *pOther)
{
    // If its in play, do nothing
    if(m_fIsInPlay)
        return;

    // Determine if the object that touches it, is a player
    // and check if the player is alive.
    if(!pOther)
         return;
    if(!pOther->IsPlayer())
         return;
    if(!pOther->IsAlive())
         return;

    CBasePlayer *pPlayer = (CBasePlayer *)pOther;

    // Print to the HUD who has taken the flag
    UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "%s has the Flag!\n", STRING( pPlayer->pev->netname )));
   
    pPlayer->m_fHasObject = true;

    // Set the client attachment using an event
    PLAYBACK_EVENT_FULL(0, pPlayer->edict(), g_usObject, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, FLAG_STOLEN, 0, 0, 0);

    m_fIsInPlay = true;

    // Give the illusion of the player taking the model by not drawing it
    pev->effects = EF_NODRAW;

    // Now don't let it accept any more touch's
    SetTouch(NULL);

	// This is the Flag that is dropped by a player when killed,disconnected,ect.
// Its basically the same as CObjectFlag, with a few changes.

LINK_ENTITY_TO_CLASS( dropped_flag, CDroppedFlag );

void CDroppedFlag :: Spawn( )
{
    // Calls the precache function to precache any models or sounds,ect
    // you need before it spawns.
    Precache( );

    // Setup what model we want to use for the flag, change this to
    // whatever you want. Just remember precache it.
    SET_MODEL( ENT(pev), "models/flag.mdl" );
   
    UTIL_SetOrigin( pev, pev->origin );
    UTIL_SetSize( pev, Vector(-16,-16,0), Vector(16,16,16) );

    // This is so it falls to the brush below it
    pev->movetype = MOVETYPE_TOSS;

    // This allows you to trigger it by walking through it
    pev->solid = SOLID_TRIGGER;

    // This just sets our Touch to Touch, which is down a little further
    SetTouch(Touch);

    // This just makes a GlowShell around the model, you can
    // change the colour of the shell by changing the RGB values,
    // which are the XYZ values in this case. So at the moment we have
    // a nice bright red colour. Feel free to change this to your needs.
    pev->effects = EF_BRIGHTFIELD;
    pev->renderfx = kRenderFxGlowShell;
    pev->rendercolor.x = 255;
    pev->rendercolor.y = 0;
    pev->rendercolor.z = 0;
    pev->renderamt = 10; // 10 units off the sides of the model
    pev->avelocity.z = 25;
}

// This is the Precache function which is called from the Spawn function.

void CDroppedFlag :: Precache( )
{
    // Precache's the model
    PRECACHE_MODEL( "models/flag.mdl" );
}

// This is the Touch function which was set above, in the Spawn function,
// its basically the stuff that runs when the flag has been touched by a player.

void CDroppedFlag :: Touch(CBaseEntity *pOther)
{
    // Determine if the object that touches it, is a player
    // and check if the player is alive.
    if(!pOther)
         return;
    if(!pOther->IsPlayer())
         return;
    if(!pOther->IsAlive())
         return;

    CBasePlayer *pPlayer = (CBasePlayer *)pOther;

    // Print to the HUD who has taken the flag
    UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs( "%s has the Flag!\n", STRING( pPlayer->pev->netname )));

    // Set it to true because the player has the flag
    pPlayer->m_fHasObject = true;

    // Set the client attachment using an event
    PLAYBACK_EVENT_FULL(0, pPlayer->edict(), g_usObject, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, FLAG_STOLEN, 0, 0, 0);

    // remove this item
    SetThink(SUB_Remove);

    // next think time to now!
    pev->nextthink = gpGlobals->time;
}


LINK_ENTITY_TO_CLASS( capture_team1, CCaptureTeam1 );

void CCaptureTeam1 :: Spawn( )
{

    // Calls the precache function to precache any models, sounds,ect
    // you need before it spawns
    Precache( );
   
    SET_MODEL( ENT(pev), STRING(pev->model) );
   
    UTIL_SetOrigin( pev, pev->origin );
    pev->movetype = MOVETYPE_FLY;

    // This allows you to trigger it by walking through it
    pev->solid = SOLID_TRIGGER;

    // This just sets our Touch to Touch and our Think to Think
    // which is down a little further.
    SetTouch(Touch);
    SetThink(Think);

    // Sets next think time
    pev->nextthink = gpGlobals->time + 0.1;

    // Make the entity invisible
    pev->rendermode = kRenderTransTexture;
    pev->renderamt = 0;
   
}

// This is the Precache function which is called from the Spawn function.

void CCaptureTeam1 :: Precache( )
{
    PRECACHE_MODEL( STRING(pev->model) );
}
void CCaptureTeam1 :: Think( )
{
    // loop through every player and check if they are in the area
    for(int i=0; i<gpGlobals->maxClients; i++)
    {
         CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);
         if(pPlayer && pPlayer->m_iTeam == 1)
         {
    if((pPlayer->pev->origin.x >= pev->mins.x) && (pPlayer->pev->origin.x <= pev->maxs.x) &&
    (pPlayer->pev->origin.y >= pev->mins.y) && (pPlayer->pev->origin.y <= pev->maxs.y) &&
    (pPlayer->pev->origin.z >= pev->mins.z) && (pPlayer->pev->origin.z <= pev->maxs.z))
    pPlayer->m_bInCapture = true;
    else
    pPlayer->m_bInCapture = false;
          }
    }
   
    // Set next think time
    pev->nextthink = gpGlobals->time + 0.1;  
}

// This is the Touch function which was set above, in the Spawn function,
// its basically the stuff that runs when the capture point has been touched by a player

void CCaptureTeam1 :: Touch( CBaseEntity* pOther )
{
    // Determine if the object that touches it, is a player
    // and check if the player is alive.
    if(!pOther)
        return;
    if(!pOther->IsPlayer())
        return;
    if(!pOther->IsAlive())
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)pOther;

    if(pPlayer && pPlayer->m_iTeam == 1)
    {
        // Check to see if they have the object
        if( !pPlayer->m_fHasObject )
             return;
        else
        {
                // Print to HUD who has captured the flag
                UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs
                ( "%s has captured the Flag!\n", STRING( pPlayer->pev->netname )));

                 // Remove the flag
                 pPlayer->m_fHasObject = false;

                 PLAYBACK_EVENT_FULL(0, pPlayer->edict(), g_usObject, 0, (float *)&g_vecZero,
                 (float *)&g_vecZero, 0, 0, FLAG_CAPTURE, 0, 0, 0);

                
                 // Reset the flag
                 CObjectFlag *pFlag = (CObjectFlag *)UTIL_FindEntityByClassname(NULL, "object_flag");
                 if(pFlag)
                 {
                        pFlag->m_fIsInPlay = false;

                        // Do a funky effect on the flag when it gets reset
                        pFlag->pev->effects = EF_BRIGHTFIELD;
                 }
        }
    }
    else
        return; // wrong team
}

void CCaptureTeam1 :: KeyValue( KeyValueData* Data )
{
   // This will store the points but its not fully implemented. I'll leave it for you to do.
    if( FStrEq( Data->szKeyName, "points" ) )
    {
    int m_iPoints = atoi(Data->szValue);
    Data->fHandled = true;
    }

    CBaseEntity::KeyValue(Data); // call the parent function
}


LINK_ENTITY_TO_CLASS( capture_team2, CCaptureTeam2 );

void CCaptureTeam2 :: Spawn( )
{
    // Calls the precache function to precache any models, sounds,ect    
    // you need before it spawns.
    Precache( );
   
    SET_MODEL( ENT(pev), STRING(pev->model) );
   
    UTIL_SetOrigin( pev, pev->origin );
    pev->movetype = MOVETYPE_FLY;

    // This allows you to trigger it by walking through it
    pev->solid = SOLID_TRIGGER;

    // This just sets our Touch to Touch and our Think to Think
    // which is down a little further.
    SetTouch(Touch);
    SetThink(Think);

    // Set next think time
    pev->nextthink = gpGlobals->time + 0.1;

    // Make the entity invisible
    pev->rendermode = kRenderTransTexture;
    pev->renderamt = 0;
}

void CCaptureTeam2 :: Precache( )
{
    PRECACHE_MODEL( STRING(pev->model) );
}

void CCaptureTeam2 :: Think( )
{
    // loop through every player and check if they are in the area
    for(int i=0; i<gpGlobals->maxClients; i++)
    {
         CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex(i);
         if(pPlayer && pPlayer->m_iTeam == 2)
         {
             if((pPlayer->pev->origin.x >= pev->mins.x) && (pPlayer->pev->origin.x <= pev->maxs.x) &&
             (pPlayer->pev->origin.y >= pev->mins.y) && (pPlayer->pev->origin.y <= pev->maxs.y) &&
             (pPlayer->pev->origin.z >= pev->mins.z) && (pPlayer->pev->origin.z <= pev->maxs.z))
             pPlayer->m_bInCapture = true;
             else
             pPlayer->m_bInCapture = false;
         }
    }
   
    // Set the next think time
    pev->nextthink = gpGlobals->time + 0.1;
   
}
void CCaptureTeam2 :: Touch( CBaseEntity* pOther )
{
    // Determine if the object that touches it, is a player
    // and check if the player is alive.
    if(!pOther)
        return;
    if(!pOther->IsPlayer())
        return;
    if(!pOther->IsAlive())
        return;

    CBasePlayer *pPlayer = (CBasePlayer *)pOther;

     if(pPlayer && pPlayer->m_iTeam == 2)
     {
          // Check to see if they have the object
          if( !pPlayer->m_fHasObject )
              return;
          else
          {    
              // Print to HUD who has captured the flag
              UTIL_ClientPrintAll( HUD_PRINTCENTER, UTIL_VarArgs
              ( "%s has captured the Flag!\n", STRING( pPlayer->pev->netname )));
  
              // Remove the object
              pPlayer->m_fHasObject = false;

              PLAYBACK_EVENT_FULL(0, pPlayer->edict(), g_usObject, 0, (float *)&g_vecZero,
              (float *)&g_vecZero, 0, 0, FLAG_CAPTURE, 0, 0, 0);

              // Reset the object
              CObjectFlag *pFlag = (CObjectFlag *)UTIL_FindEntityByClassname(NULL, "object_flag");
              if(pFlag)
              {
                   pFlag->m_fIsInPlay = false;

                   // Do a funky effect on the flag when it gets reset
                   pFlag->pev->effects = EF_BRIGHTFIELD;
              }
          }
    }
    else
        return; // Wrong team
}


void CCaptureTeam2 :: KeyValue( KeyValueData* Data )
{
// This will store the points but its not fully implemented. I'll leave it for you to do.
    if( FStrEq( Data->szKeyName, "points" ) )
    {
        int m_iPoints = atoi(Data->szValue);
        Data->fHandled = true;
    }

    CBaseEntity::KeyValue(Data); // call the parent function
}

