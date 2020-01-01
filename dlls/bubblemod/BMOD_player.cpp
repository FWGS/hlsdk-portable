// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//        Tyler Lund <halflife@bubblemod.org>
//
// LICENSE                                                            
//                                                                    
//        Permission is granted to anyone to use this  software  for  
//        any purpose on any computer system, and to redistribute it  
//        in any way, subject to the following restrictions:          
//                                                                    
//        1. The author is not responsible for the  consequences  of  
//           use of this software, no matter how awful, even if they  
//           arise from defects in it.                                
//        2. The origin of this software must not be misrepresented,  
//           either by explicit claim or by omission.                 
//        3. Altered  versions  must  be plainly marked as such, and  
//           must  not  be  misrepresented  (by  explicit  claim  or  
//           omission) as being the original software.                
//        3a. It would be nice if I got  a  copy  of  your  improved  
//            version  sent to halflife@bubblemod.org. 
//        4. This notice must not be removed or altered.              
//                                                                    
// ---------------------------------------------------------------

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"

#include "BMOD_messaging.h"
#include "BMOD_CameraPoint.h"
#include "BMOD_player.h"

extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgTeamInfo;
extern int gmsgSpectator;

extern void respawn(entvars_t *pev, BOOL fCopyCorpse);

extern cvar_t bm_spawnkilltime;
extern cvar_t bm_typekills;
extern cvar_t bm_bantime;
extern cvar_t bm_antispam;
extern cvar_t bm_spamlimit;
extern cvar_t bm_typecam;

// Player has become a spectator. Set it up.
// This was moved from player.cpp.
void CBasePlayer::StartObserver( Vector vecPosition, Vector vecViewAngle )
{
	// ??
	// clear any clientside entities attached to this player
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_KILLPLAYERATTACHMENTS );
		WRITE_BYTE( (BYTE)entindex() );
	MESSAGE_END();
	
	// Holster weapon immediately, to allow it to cleanup
	if (m_pActiveItem)
		m_pActiveItem->Holster( );

	// ?? Let go of tanks?
	if( m_pTank != 0 )
	{
	 	m_pTank->Use( this, this, USE_OFF, 0 );
	 	m_pTank = NULL;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
	 	WRITE_BYTE(0);
	 	WRITE_BYTE(0XFF);
	 	WRITE_BYTE(0xFF);
	MESSAGE_END();

	// reset FOV
	m_iFOV = 0;
	m_iClientFOV = -1;
	pev->fov = m_iFOV;
	SET_VIEW(edict(), edict());

	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE(0);
	MESSAGE_END();

	// Setup flags
	m_iHideHUD = (HIDEHUD_FLASHLIGHT | HIDEHUD_WEAPONS | HIDEHUD_HEALTH);
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->effects |= EF_NODRAW;
	pev->view_ofs = g_vecZero;
	pev->angles = pev->v_angle = vecViewAngle;
	pev->fixangle = TRUE;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NOCLIP;
	ClearBits( m_afPhysicsFlags, PFLAG_DUCKING );
	ClearBits( pev->flags, FL_DUCKING );
	pev->deadflag = DEAD_RESPAWNABLE;
	pev->health = 1;
	
	// Clear out the status bar
	m_fInitHUD = TRUE;

	// Update Team Status
	// pev->team = 0;

	// Remove all the player's stuff
	// RemoveAllItems( FALSE );

	// Move them to the new position
	UTIL_SetOrigin( pev, vecPosition );

	// Find a player to watch
	Observer_SetMode(OBS_ROAMING);

	// Tell all clients this player is now a spectator
	//MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
	//	WRITE_BYTE( ENTINDEX( edict() ) );
	//	WRITE_BYTE( 1 );
	//MESSAGE_END();

	 //MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
	 //	WRITE_BYTE( ENTINDEX(edict()) );
	 //  WRITE_STRING( "" );
	 //MESSAGE_END();

	 m_fMsgTimer = gpGlobals->time + 0.5f;
	 m_bSentMsg = FALSE;
}

// Leave observer mode
void CBasePlayer::StopObserver( void )
{
	// Turn off spectator
	if ( pev->iuser1 || pev->iuser2 )
	{
		// Tell all clients this player is not a spectator anymore
		//MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );  
		// 	WRITE_BYTE( ENTINDEX( edict() ) );
		// 	WRITE_BYTE( 0 );
		//MESSAGE_END();

		pev->iuser1 = pev->iuser2 = 0; 
		m_hObserverTarget = NULL;
		m_iHideHUD = 0;
		
		//MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
	 	//	WRITE_BYTE( ENTINDEX(edict()) );
	 	//	WRITE_STRING( m_szTeamName );
		//MESSAGE_END();
	}
}

// Find the next client in the game for this player to spectate
void CBasePlayer::Observer_FindNextPlayer( bool bReverse )
{

	int		iStart;
	if ( m_hObserverTarget )
		iStart = ENTINDEX( m_hObserverTarget->edict() );
	else
		iStart = ENTINDEX( edict() );
	int	    iCurrent = iStart;
	m_hObserverTarget = NULL;
	int iDir = 1; 

	do
	{
		iCurrent += iDir;

		// Loop through the clients
		if (iCurrent > gpGlobals->maxClients)
			iCurrent = 1;
		if (iCurrent < 1)
			iCurrent = gpGlobals->maxClients;

		CBaseEntity *pEnt = UTIL_PlayerByIndex( iCurrent );
		if ( !pEnt )
			continue;
		if ( pEnt == this )
			continue;
		// Don't spec observers or invisible players or defunct players
		if ( ((CBasePlayer*)pEnt)->IsObserver() 
			|| (pEnt->pev->effects & EF_NODRAW) 
			|| ( ( STRING( pEnt->pev->netname ) )[0] == 0 ) 
			|| (!((CBasePlayer*)pEnt)->m_bIsConnected)
			)
			continue;

		// MOD AUTHORS: Add checks on target here.

		m_hObserverTarget = pEnt;
		break;

	} while ( iCurrent != iStart );

	// Did we find a target?
	if ( m_hObserverTarget )
	{
		// Store the target in pev so the physics DLL can get to it
		pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
		// Move to the target
		UTIL_SetOrigin( pev, m_hObserverTarget->pev->origin );
		
		// ClientPrint( pev, HUD_PRINTCENTER, UTIL_VarArgs ("Chase Camera: %s\n", STRING( m_hObserverTarget->pev->netname ) ) );
		ALERT( at_console, "Now Tracking %s\n", STRING( m_hObserverTarget->pev->netname ) );
	}
	else
	{
		ALERT( at_console, "No observer targets.\n" );
	}
}

// Handle buttons in observer mode
void CBasePlayer::Observer_HandleButtons()
{
	// Slow down mouse clicks
	if ( m_fMsgTimer <= gpGlobals->time )
	{
		// Show the observer mode instructions
		if (!m_bSentMsg)
		{
			m_bSentMsg = TRUE;
			PrintMessage( this, BMOD_CHAN_INFO, Vector (20,250,20), Vector (.5, 15, 2), "SPEC");
			PrintMessage( this, BMOD_CHAN_RUNE, Vector (20,250,20), Vector (.5, 86400, 2), 
				"-Spectator Mode-\n FIRE=Spawn   JUMP=Switch Modes   ALT FIRE=Switch Targets");
		}

		// Check to see if the observer wants to spray their logo
		if (pev->impulse == 201) {
			ImpulseCommands();
		}

		// Jump changes modes: Chase to Roaming
		if ( pev->button & IN_JUMP )
		{
			if ( pev->iuser1 == OBS_ROAMING )
				Observer_SetMode( OBS_CHASE_FREE );
			else
				Observer_SetMode( OBS_ROAMING );
			m_fMsgTimer = gpGlobals->time + 0.2f;
		}

		// Attack2 cycles player targets
		if ( pev->button & IN_ATTACK2 && pev->iuser1 != OBS_ROAMING )
		{
			Observer_FindNextPlayer( false );
			m_fMsgTimer = gpGlobals->time + 0.2f;
		}

		// Attack Spawns
		// if ( m_afButtonPressed & IN_ATTACK && pev->iuser1 != OBS_ROAMING )
		if ( pev->button & IN_ATTACK )
		{
//			g_engfuncs.pfnServerPrint( "Player spawned from Obs!\n" );
			StopObserver();
			m_fMsgTimer = gpGlobals->time + 0.2f;
		}		
	}

	// clear attack/use commands from player
	m_afButtonPressed = 0;
	pev->button = 0;
	m_afButtonReleased = 0;

	pev->impulse = 0;
	if ((m_hObserverTarget != 0) && (m_hObserverTarget->IsPlayer()) && (pev->iuser1 == OBS_CHASE_FREE))
	{
		pev->origin = m_hObserverTarget->pev->origin;
		pev->velocity = m_hObserverTarget->pev->velocity;
	}
}

// Attempt to change the observer mode
void CBasePlayer::Observer_SetMode( int iMode )
{
	// Just abort if we're changing to the mode we're already in
	if ( iMode == pev->iuser1 )
		return;

	// Changing to Roaming?
	if ( iMode == OBS_ROAMING )
	{
		// MOD AUTHORS: If you don't want to allow roaming observers at all in your mod, just abort here.
		pev->iuser1 = OBS_ROAMING;
		pev->iuser2 = 0;
		m_hObserverTarget = NULL;

		ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode3" );
		// pev->maxspeed = 320;
		pev->maxspeed = 1000;
		return;
	}

	// Changing to Chase Lock?
	if ( iMode == OBS_CHASE_LOCKED )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		if ( m_hObserverTarget == 0 )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_LOCKED;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_Mode1" );
			pev->maxspeed = 0;
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}

		return;
	}

	// Changing to Chase Freelook?
	if ( iMode == OBS_CHASE_FREE )
	{
		// If changing from Roaming, or starting observing, make sure there is a target
		// if ( m_hObserverTarget == NULL )
		if ( pev->iuser1 == OBS_ROAMING )
			Observer_FindNextPlayer( false );

		if (m_hObserverTarget)
		{
			pev->iuser1 = OBS_CHASE_FREE;
			pev->iuser2 = ENTINDEX( m_hObserverTarget->edict() );
			// ClientPrint( pev, HUD_PRINTCENTER, "Chase-Camera Mode" );
			pev->maxspeed = 0;
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Spec_NoTarget"  );
			Observer_SetMode(OBS_ROAMING);
		}
		
		return;
	}
}

void CBasePlayer::BMOD_PreThink(void)
{
	// Bubblemod freeze ray
	if (m_flFreezeTime > 0)
    {
      if (m_flFreezeTime <= gpGlobals->time)
      {
         EnableControl(TRUE);
		 // pev->movetype = MOVETYPE_WALK;
         pev->rendermode = kRenderNormal;
         pev->renderfx = kRenderFxNone;
         pev->renderamt = 0;
		 
         m_flFreezeTime = 0;
      }
      else
	  {
		pev->v_angle = m_vFreezeAngle;
        pev->fixangle = TRUE;
	  }
    }

	// Bubblemod spawn
	if (
		m_fSpawnTimeStamp &&
		m_fSpawnTimeStamp + bm_spawnkilltime.value >= gpGlobals->time &&
		m_iMessageFire
		)
	{
		PrintMessage( this, BMOD_CHAN_INFO, Vector (255,0,0), Vector (.1, bm_spawnkilltime.value, .1), 
			UTIL_VarArgs("Spawn Protection: %d seconds", (int)(m_fSpawnTimeStamp + bm_spawnkilltime.value - gpGlobals->time))
			);
	}

	if (m_fSpawnTimeStamp > 0 &&
		((m_fSpawnTimeStamp + bm_spawnkilltime.value) <= gpGlobals->time) ) {

		if (m_RuneFlags == RUNE_NONE &&
			m_flFreezeTime == 0) {
			 pev->rendermode = kRenderNormal;
			 pev->renderfx = kRenderFxNone;
			 pev->renderamt = 0;
		}
		 BMOD_ResetSpawnKill();
    }

	// Bubblemod runes
	if (m_RuneFlags && (m_RuneTime < gpGlobals->time) )
	{
		m_RuneFlags = RUNE_NONE;
		pev->rendermode = kRenderNormal;
	    pev->renderfx = kRenderFxNone;
		pev->renderamt = 0;	
	}

	// BubbleMod Type detection
	if (pev->button & ~IN_SCORE ) {
		BMOD_ResetTypeKill();
	}

	// Color the typer
	if (BMOD_IsTyping() &&
		m_iMessageFire ) {
		if (!(m_iMessageCounter % 2)) {
			//MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			//	WRITE_BYTE( TE_SMOKE );
			//	WRITE_COORD( pev->origin.x );
			//	WRITE_COORD( pev->origin.y );
			//	WRITE_COORD( pev->origin.z - 36 );
			//	WRITE_SHORT( g_sModelIndexSmoke );
			//	WRITE_BYTE( 10 ); // scale * 10
			//	WRITE_BYTE( 10  ); // framerate
			//MESSAGE_END();
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_SPRITE );
				WRITE_COORD( pev->origin.x + RANDOM_LONG( -10, 10) );
				WRITE_COORD( pev->origin.y + RANDOM_LONG( -10, 10) );
				WRITE_COORD( pev->origin.z + RANDOM_LONG( -30, 30) );
				WRITE_SHORT( g_sModelIndexFlare );
				WRITE_BYTE( 3 ); // scale * 10
				WRITE_BYTE( 255  ); // alpha
			MESSAGE_END();
		}
		float fCos = cos( gpGlobals->time * 4.0f) * 8;
		float fSin = sin( gpGlobals->time * 4.0f) * 8;
        MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
                WRITE_BYTE( TE_BUBBLES );
                WRITE_COORD( pev->origin.x + fCos);  
                WRITE_COORD( pev->origin.y + fSin);
                WRITE_COORD( pev->origin.z - 36 );
                WRITE_COORD( pev->origin.x + fCos);  // mins
                WRITE_COORD( pev->origin.y + fSin);
                WRITE_COORD( pev->origin.z - 36 );
                WRITE_COORD( 75 );  // height
                WRITE_SHORT( g_sModelIndexBubbles );
                WRITE_BYTE( 3 ); // count
                WRITE_COORD( -1 ); // speed
        MESSAGE_END();

		// Away put your weapons!
		//if (m_pActiveItem) 	{
		//	ResetAutoaim( );
		//	m_pActiveItem->Holster( );
		//	m_pActiveItem = NULL;
		//}
		m_pLastItem = m_pActiveItem;

		if (!m_bTypeMode) {
			m_bTypeMode = TRUE;
			// EnableControl(FALSE);
			if (bm_typecam.value) {
				// spawn a camera above us and use that for our view.
				CCamPoint *pCam = (CCamPoint*)CBaseEntity::Create( "campoint", 
					pev->origin + Vector(0,0,40), 
					Vector(pev->angles.x + 22, pev->angles.y + 180, 0), 
					edict() );
				if (pCam) {
					pCam->m_pOwner = this;
					pCam->Think();
					// UTIL_ClientPrintAll( HUD_PRINTTALK, "<SERVER> Camera point created.\n");	
					SET_VIEW(edict(), pCam->edict());
					SET_MODEL(ENT(pCam->pev), STRING(pev->model) );
					pev->fov = m_iFOV = 140;
				}	
			}
		}
	}
}

void CBasePlayer::BMOD_Think( void )
{
	// BMOD End - Message delayer.
	// Message delayer. USE THIS for general messages to players. 
	// counts tenths of seconds. 
	if (m_fMessageTimer < gpGlobals->time) 
	{
		m_fMessageTimer = gpGlobals->time + 0.1f;
		m_iMessageCounter++;
		m_iMessageFire = TRUE;
	}

	// Locator
	if (m_LocateMode && m_iMessageFire) {
		int yang = (int)pev->angles.y;
		if (yang < 0)
			yang += 360;

		ClientPrint( pev, HUD_PRINTCENTER, UTIL_VarArgs ("Location: %dx %dy %dz, facing %d", 
			(int)pev->origin.x,
			(int)pev->origin.y,
			(int)pev->origin.z,
			yang
			) );
	}

	// Llamas!
	if (m_IsLlama && m_iMessageFire && !(m_iMessageCounter%5)) {
		CLIENT_COMMAND(edict(), "drop\n");
	}

	// Autoban
	if (m_bBanMe && m_iMessageFire) 
	{
		char cmd[81] = "";
		sprintf( cmd, "banid %d %s kick\n", (int)bm_bantime.value, GETPLAYERAUTHID( edict() ));
		SERVER_COMMAND(cmd);
	}

	// BMOD Begin - Spam Kick
	if (m_iMessageFire && !(m_iMessageCounter%10) && m_iSpamSay)
	{
		m_iSpamSay--;
	}

	if (bm_antispam.value && m_iSpamSay > bm_spamlimit.value)
	{
		char cmd[81] = "";
		sprintf( cmd, "kick # %u\n", GETPLAYERUSERID( edict() ));
		SERVER_COMMAND(cmd);
		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "<SERVER> %s was kicked for spamming.\n",
			STRING( pev->netname )));
		UTIL_LogPrintf( "\"SERVER<-1><-1><>\" say \"%s was was kicked for spamming.\"\n",  
			STRING( pev->netname ));
		UTIL_LogPrintf( "// \"%s<%i><%s><%s>\" was kicked for spamming.\n",  
			STRING( pev->netname ),
			GETPLAYERUSERID( edict() ),
			GETPLAYERAUTHID( edict() ),
			g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( edict() ), "model" )
			);
		m_iSpamSay = 0;
	}
	// BMOD End - Spam Kick


}

void CBasePlayer::BMOD_PostThink( void )
{

}

void CBasePlayer::BMOD_Identify( void )
{
	static float timer = gpGlobals->time;
	static CBasePlayer *LastIdentPlayer = NULL;

	TraceResult tr;

	CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pev);

	Vector anglesAim = pPlayer->pev->v_angle;
	UTIL_MakeVectors( anglesAim );
	anglesAim.x              = -anglesAim.x;
	Vector vecSrc = pPlayer->GetGunPosition( ) - gpGlobals->v_up * 1;
	Vector vecDir = gpGlobals->v_forward;

	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine(vecSrc, vecSrc + vecDir * 8192, dont_ignore_monsters, ENT(pPlayer->pev), &tr);

	CBaseEntity *pOther = CBaseEntity::Instance( tr.pHit );

	// Do we have our nose in a wall?
	if (((tr.vecEndPos - vecSrc).Length() > 40) || pOther->IsPlayer()) {
		BMOD_ResetTypeKill();
	}

	if ( pOther->IsPlayer() && 
			(
				(CBasePlayer *)pOther != LastIdentPlayer ||
				timer <= gpGlobals->time
			)
		)
	{
		char szExtra[81] = "\n";
		if (((CBasePlayer *)pOther)->BMOD_IsASpawn()) {
			strcat(szExtra, "\nFresh Spawn! DO NOT SHOOT!");
		}
		if (((CBasePlayer *)pOther)->BMOD_IsTyping()) {
			strcat(szExtra, "\nTyping! DO NOT SHOOT!");
		}

		CBasePlayer *pOtherPlayer = (CBasePlayer *)pOther;
		if (IsObserver())
				ClientPrint( pev, HUD_PRINTCENTER, UTIL_VarArgs ("-%s-\n(%s) %i / %i%s", 
					STRING( pOtherPlayer->pev->netname ), 
					g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pOtherPlayer->edict() ), "model" ),
					(int)pOtherPlayer->pev->health, 
					(int)pOtherPlayer->pev->armorvalue,
					szExtra
					) );
		else if ( g_pGameRules->PlayerRelationship( pOther, this ) == GR_TEAMMATE )
				ClientPrint( pev, HUD_PRINTCENTER, UTIL_VarArgs ("-%s-\n%i / %i", 
					STRING( pOtherPlayer->pev->netname ), 
					(int)pOtherPlayer->pev->health, 
					(int)pOtherPlayer->pev->armorvalue) 
					);
		else
				ClientPrint( pev, HUD_PRINTCENTER, UTIL_VarArgs ("-%s-\n(%s)%s", 
					STRING( pOtherPlayer->pev->netname ),
					g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pOtherPlayer->edict() ), "model" ),
					szExtra
					) );

		timer = gpGlobals->time + 0.5f;
		LastIdentPlayer = (CBasePlayer *)pOther;
	}
	else if (!pOther->IsPlayer()) {
		LastIdentPlayer = NULL;
	}
}

int CBasePlayer::BMOD_WasSpawnKilled( void )
{
	if ( !IsAlive() && 
		m_fSpawnTimeStamp &&
		((m_fSpawnTimeStamp + bm_spawnkilltime.value) > gpGlobals->time) &&
		bm_spawnkilltime.value)
		return TRUE;
	return FALSE;
}

int CBasePlayer::BMOD_IsASpawn( void )
{
	if ( IsAlive() && 
		m_fSpawnTimeStamp &&
		((m_fSpawnTimeStamp + bm_spawnkilltime.value) > gpGlobals->time) &&
		bm_spawnkilltime.value)
		return TRUE;
	return FALSE;
}

void CBasePlayer::BMOD_ResetSpawnKill( void )
{


	if (m_RuneFlags == RUNE_NONE &&
		m_flFreezeTime == 0) {
		pev->rendermode = kRenderNormal;
		pev->renderfx = kRenderFxNone;
		pev->renderamt = 0;	
	}
	m_fSpawnTimeStamp = 0;
	PrintMessage( this, BMOD_CHAN_INFO, Vector (20,250,20), Vector (.1, 1, .1), "");
}


int CBasePlayer::BMOD_WasTypeKilled( void )
{
	if ( !IsAlive() && 
		m_flTypeKillStamp > 0 &&
		m_flTypeKillStamp < gpGlobals->time &&
		!bm_typekills.value )
		return TRUE;
	return FALSE;
}

int CBasePlayer::BMOD_IsTyping( void )
{
	if ( IsAlive() &&
		m_flTypeKillStamp &&
		m_flTypeKillStamp < gpGlobals->time &&
		!bm_typekills.value ) {

			// Do we have a crowbar?
			if (m_pActiveItem != NULL &&
				strcmp("weapon_crowbar", STRING(m_pActiveItem->pev->classname))
				) 
			{
				BMOD_ResetTypeKill();
				return FALSE;
			}

			return TRUE;
	}
	return FALSE;
}

void CBasePlayer::BMOD_ResetTypeKill( void )
{
	if ( IsAlive() ) {
		// make a sound if we are coming out of type mode.
		if (m_bTypeMode) 
		{
			EMIT_SOUND(ENT(pev), CHAN_VOICE, "fvox/alert.wav", 1.0, ATTN_NORM);
			SET_VIEW(edict(), edict());
			pev->fov = m_iFOV = 0;
		}
		m_bTypeMode = FALSE;
		// EnableControl(TRUE);
		m_flTypeKillStamp = gpGlobals->time + 4.0f;
	}
	return;
}

