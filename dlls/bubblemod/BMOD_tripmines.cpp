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
#include	"game.h"
#include	"tripmine.h"
#include	"shake.h"

BOOL CTripmineGrenade::BMOD_IsSpawnMine()
{
	if (bm_spawnmines.value)
		return FALSE;

	BOOL result = FALSE;
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	Vector vecSpot;
	Vector vecSrc = pev->origin;
	float flRadius = pev->dmg * 2.5f;

	int bInWater = (UTIL_PointContents ( vecSrc ) == CONTENTS_WATER);

	vecSrc.z += 1;// in case grenade is lying on the ground

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecSrc, flRadius )) != NULL)
	{
		// Only look for deathmatch spawn points
		if ( FClassnameIs( pEntity->pev, "info_player_deathmatch" ) )
		{
			// blast's don't tavel into or out of water,
			// so ignore spawn points that lie on the other side.
			if (bInWater && pEntity->pev->waterlevel == 0)
				continue;
			if (!bInWater && pEntity->pev->waterlevel == 3)
				continue;

			// Trace a small line from the trip out to the potential damage radius.
			UTIL_TraceLine ( vecSrc, vecSrc + m_vecDir * flRadius, ignore_monsters, ENT(pev), &tr );
			vecSpot = tr.vecEndPos;

			UTIL_TraceLine( pEntity->pev->origin, pEntity->pev->origin - Vector(0,0,1024), ignore_monsters, ENT(pev), &tr);
			Vector vecTop = pEntity->pev->origin + Vector(0,0,36);
			float height = fabs(vecTop.z - tr.vecEndPos.z) * 0.5f;

			if (UTIL_OBB_LineTest(vecSrc, vecSpot, Vector(vecTop.x, vecTop.y, (vecTop.z + tr.vecEndPos.z) * 0.5f), Vector(16,16,height) ))
				result = TRUE;
		}
	}
	return result;
}

void CTripmineGrenade::FlashBang( void )
{
#define FLASH_RANGE 600
	pev->nextthink = gpGlobals->time + 0.3f;
	SetThink( &CTripmineGrenade::Smoke );

	// Find all players in range
	CBaseEntity *pEntity = NULL;
	TraceResult tr;
	Vector vecSpot;
	float amount, range;

	EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/pl_gun2.wav", 1, ATTN_NORM);

	while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, FLASH_RANGE )) != NULL)
	{
		// Only look for players
		if ( FClassnameIs( pEntity->pev, "player" ) )
		{
			vecSpot = pEntity->BodyTarget( pev->origin );

			// Trace a small line from the trip out to the player.
			UTIL_TraceLine ( pev->origin, vecSpot, dont_ignore_monsters, ENT(pev), &tr );
			if (tr.flFraction == 1.0f || tr.pHit == pEntity->edict())
			{
				range = (vecSpot - pev->origin).Length();
				amount = range 
					* range  
					* -1
					/ FLASH_RANGE 
					+ 255;
				UTIL_ScreenFade( pEntity, Vector(255,255,255), 5.0, 1.0, amount, FFADE_IN);

			}

		}
	}

}

void CTripmineGrenade::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	KillBeam();
	UTIL_Remove( this );
}

//=========================================================
// DeactivateTrips - removes all trips owned by
// the provided player.
//
// Made this global on purpose.
//=========================================================
void DeactivateTrips( CBasePlayer *pOwner )
{
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_tripmine" );

	while ( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CTripmineGrenade *pTrip = (CTripmineGrenade *)pEnt;

		if ( pTrip )
		{
			if ( pTrip->Owner() == pOwner->edict() )
			{
				pTrip->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_tripmine" );
	}
}
