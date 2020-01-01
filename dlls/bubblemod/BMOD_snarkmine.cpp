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
// Snark Mines

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "effects.h"
#include "gamerules.h"
#include "BMOD_snarkmine.h"

#define	TRIPMINE_PRIMARY_VOLUME		450
#define	TRIPSNARK_FLARE			"sprites/xspark3.spr"

extern cvar_t bm_spawnmines;

enum tripmine_e {
	TRIPMINE_IDLE1 = 0,
	TRIPMINE_IDLE2,
	TRIPMINE_ARM1,
	TRIPMINE_ARM2,
	TRIPMINE_FIDGET,
	TRIPMINE_HOLSTER,
	TRIPMINE_DRAW,
	TRIPMINE_WORLD,
	TRIPMINE_GROUND
};

LINK_ENTITY_TO_CLASS( monster_tripsnark, CTripSnarkGrenade );

TYPEDESCRIPTION	CTripSnarkGrenade::m_SaveData[] = 
{
	DEFINE_FIELD( CTripSnarkGrenade, m_flPowerUp, FIELD_TIME ),
	DEFINE_FIELD( CTripSnarkGrenade, m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( CTripSnarkGrenade, m_vecEnd, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTripSnarkGrenade, m_flBeamLength, FIELD_FLOAT ),
	DEFINE_FIELD( CTripSnarkGrenade, m_hOwner, FIELD_EHANDLE ),
	DEFINE_FIELD( CTripSnarkGrenade, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CTripSnarkGrenade, m_posOwner, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CTripSnarkGrenade, m_angleOwner, FIELD_VECTOR ),
	DEFINE_FIELD( CTripSnarkGrenade, m_pRealOwner, FIELD_EDICT ),
};

IMPLEMENT_SAVERESTORE(CTripSnarkGrenade,CGrenade);


void CTripSnarkGrenade :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;

	SET_MODEL(ENT(pev), "models/v_tripmine.mdl");
	pev->frame = 0;
	pev->body = 3;
	pev->sequence = TRIPMINE_WORLD;
	ResetSequenceInfo( );
	pev->framerate = 0;
	
	UTIL_SetSize(pev, Vector( -8, -8, -8), Vector(8, 8, 8));
	UTIL_SetOrigin( pev, pev->origin );

	if (pev->spawnflags & 1)
	{
		// power up quickly
		m_flPowerUp = gpGlobals->time + 1.0f;
	}
	else
	{
		// power up in 2.5 seconds
		m_flPowerUp = gpGlobals->time + 2.5f;
	}

	SetThink( &CTripSnarkGrenade::PowerupThink );
	pev->nextthink = gpGlobals->time + 0.2f;

	pev->takedamage = DAMAGE_YES;
	// pev->dmg = gSkillData.plrDmgTripmine;
	pev->health = 1; // don't let die normally

	if (pev->owner != NULL)
	{
		// play deploy sound
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav", 1.0, ATTN_NORM );
		EMIT_SOUND( ENT(pev), CHAN_BODY, "weapons/mine_charge.wav", 0.2, ATTN_NORM ); // chargeup

		m_pRealOwner = pev->owner;// see CTripSnarkGrenade for why.
	}

	UTIL_MakeAimVectors( pev->angles );

	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = pev->origin + m_vecDir * 2048;
}


void CTripSnarkGrenade :: Precache( void )
{
	PRECACHE_MODEL("models/v_tripmine.mdl");
	PRECACHE_SOUND("weapons/mine_deploy.wav");
	PRECACHE_SOUND("squeek/sqk_deploy1.wav");
	PRECACHE_SOUND("weapons/mine_charge.wav");
	PRECACHE_SOUND("debris/beamstart2.wav");
	PRECACHE_MODEL( TRIPSNARK_FLARE );
	m_LaserSprite = PRECACHE_MODEL( "sprites/laserbeam.spr" );
}


void CTripSnarkGrenade :: WarningThink( void  )
{
	// play warning sound
	// EMIT_SOUND( ENT(pev), CHAN_VOICE, "buttons/Blip2.wav", 1.0, ATTN_NORM );

	// set to power up
	SetThink( &CTripSnarkGrenade::PowerupThink );
	pev->nextthink = gpGlobals->time + 1.0f;
}


void CTripSnarkGrenade :: PowerupThink( void  )
{
	TraceResult tr;

	if( m_hOwner == 0 )
	{
		// find an owner
		edict_t *oldowner = pev->owner;
		pev->owner = NULL;
		UTIL_TraceLine( pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 32, dont_ignore_monsters, ENT( pev ), &tr );
		if (tr.fStartSolid || (oldowner && tr.pHit == oldowner))
		{
			pev->owner = oldowner;
			m_flPowerUp += 0.1f;
			pev->nextthink = gpGlobals->time + 0.1f;
			return;
		}
		if (tr.flFraction < 1.0f)
		{
			pev->owner = tr.pHit;
			m_hOwner = CBaseEntity::Instance( pev->owner );
			m_posOwner = m_hOwner->pev->origin;
			m_angleOwner = m_hOwner->pev->angles;
		}
		else
		{
			STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav" );
			STOP_SOUND( ENT(pev), CHAN_BODY, "weapons/mine_charge.wav" );
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1f;
			ALERT( at_console, "WARNING:Tripmine at %.0f, %.0f, %.0f removed\n", (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z );
			KillBeam();
			return;
		}
	}
	else if (m_posOwner != m_hOwner->pev->origin || m_angleOwner != m_hOwner->pev->angles)
	{
		// disable
		STOP_SOUND( ENT(pev), CHAN_VOICE, "weapons/mine_deploy.wav" );
		STOP_SOUND( ENT(pev), CHAN_BODY, "weapons/mine_charge.wav" );
		CBaseEntity *pMine = Create( "weapon_snark", pev->origin + m_vecDir * 24, pev->angles );
		pMine->pev->spawnflags |= SF_NORESPAWN;
		SetThink( &CBaseEntity::SUB_Remove );
		KillBeam();
		pev->nextthink = gpGlobals->time + 0.1f;
		return;

		/*
		// Just detonate
		pev->owner = m_pRealOwner;
                pev->health = 0;
                Killed( VARS( pev->owner ), GIB_NORMAL );
                return;
		*/
	}
	// ALERT( at_console, "%d %.0f %.0f %0.f\n", pev->owner, m_pOwner->pev->origin.x, m_pOwner->pev->origin.y, m_pOwner->pev->origin.z );
 
	if (gpGlobals->time > m_flPowerUp)
	{
		// make solid
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );

		MakeBeam( );

		// play enabled sound
        	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "squeek/sqk_deploy1.wav", 0.5, ATTN_NORM, 1.0, 75 );
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}


void CTripSnarkGrenade :: KillBeam( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
}


void CTripSnarkGrenade :: MakeBeam( void )
{
	TraceResult tr;

	// ALERT( at_console, "serverflags %f\n", gpGlobals->serverflags );

	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	m_flBeamLength = tr.flFraction;

	// set to follow laser spot
	SetThink( &CTripSnarkGrenade::BeamBreakThink );
	pev->nextthink = gpGlobals->time + 0.1f;

	Vector vecTmpEnd = pev->origin + m_vecDir * 2048 * m_flBeamLength;

	m_pBeam = CBeam::BeamCreate( g_pModelNameLaser, 10 );
	m_pBeam->PointEntInit( vecTmpEnd, entindex() );
	m_pBeam->SetColor( 255, 20, 20 );
	m_pBeam->SetScrollRate( 255 );
	m_pBeam->SetBrightness( 40 );

	if (IsSpawnMine())
	{
		pev->owner = m_pRealOwner;
		pev->health = 0;
		Killed( VARS( pev->owner ), GIB_NORMAL );

		UTIL_SpeakBadWeapon();

		UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "%s tried to place a spawn snark mine!\n",
			STRING( VARS( pev->owner )->netname ) ) );
	}
}


void CTripSnarkGrenade :: BeamBreakThink( void  )
{
	BOOL bBlowup = 0;

	TraceResult tr;

	// HACKHACK Set simple box using this really nice global!
	gpGlobals->trace_flags = FTRACE_SIMPLEBOX;
	UTIL_TraceLine( pev->origin, m_vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	// ALERT( at_console, "%f : %f\n", tr.flFraction, m_flBeamLength );

	// respawn detect. 
	if ( !m_pBeam )
	{
		MakeBeam( );
		if ( tr.pHit )
			m_hOwner = CBaseEntity::Instance( tr.pHit );	// reset owner too
	}

	if (fabs( m_flBeamLength - tr.flFraction ) > 0.001f)
	{
		bBlowup = 1;
	}
	else
	{
		if( m_hOwner == 0 )
			bBlowup = 1;
		else if( m_posOwner != m_hOwner->pev->origin )
			bBlowup = 1;
		else if( m_angleOwner != m_hOwner->pev->angles )
			bBlowup = 1;
	}
	
	if (bBlowup)
	{
		// a bit of a hack, but all CGrenade code passes pev->owner along to make sure the proper player gets credit for the kill
		// so we have to restore pev->owner from pRealOwner, because an entity's tracelines don't strike it's pev->owner which meant
		// that a player couldn't trigger his own tripmine. Now that the mine is exploding, it's safe the restore the owner so the 
		// CGrenade code knows who the explosive really belongs to.
		pev->owner = m_pRealOwner;
		pev->health = 0;
		Killed( VARS( pev->owner ), GIB_NORMAL );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1f;
}

int CTripSnarkGrenade :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (gpGlobals->time < m_flPowerUp && flDamage < pev->health)
	{
		// disable
		// Create( "weapon_tripsnark", pev->origin + m_vecDir * 24, pev->angles );
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1f;
		KillBeam();
		return FALSE;
	}
	return CGrenade::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CTripSnarkGrenade::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->takedamage = DAMAGE_NO;
	
	if ( pevAttacker && ( pevAttacker->flags & FL_CLIENT ) )
	{
		// some client has destroyed this mine, he'll get credit for any kills
		pev->owner = ENT( pevAttacker );
	}

	SetThink( &CTripSnarkGrenade::DelayDeathThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1f, 0.3f );

	EMIT_SOUND( ENT(pev), CHAN_BODY, "common/null.wav", 0.5, ATTN_NORM ); // shut off chargeup
}


void CTripSnarkGrenade::Explode( TraceResult *pTrace )
{
	// Make mine invisible
        pev->model = iStringNull;//invisible
        pev->solid = SOLID_NOT;// intangible
	pev->effects |= EF_NODRAW;
        pev->takedamage = DAMAGE_NO;

        // Pull out of the wall a bit
        if ( pTrace->flFraction != 1.0f )
        {
                pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * 20);
        }

	// Make teleport sound effect
	EMIT_SOUND( ENT(pev), CHAN_BODY, "debris/beamstart2.wav", 0.5, ATTN_NORM );

	// Create Animated Sprite
	m_pSprite = CSprite::SpriteCreate( TRIPSNARK_FLARE, pev->origin, TRUE );
	m_pSprite->Animate( 1 );
        m_pSprite->pev->scale = 1.4;
        m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;

	// Spawn snarks 
	CBaseEntity *pSqueak = CBaseEntity::Create( "monster_snark", pev->origin, m_vecDir , pev->owner );
        pSqueak->pev->velocity = m_vecDir * 200;
	pSqueak = CBaseEntity::Create( "monster_snark", pev->origin + Vector(8, 8, 0), m_vecDir , pev->owner );
        pSqueak->pev->velocity = m_vecDir * 200;
	pSqueak = CBaseEntity::Create( "monster_snark", pev->origin + Vector(-8, 8, 0), m_vecDir , pev->owner );
        pSqueak->pev->velocity = m_vecDir * 200;
	pSqueak = CBaseEntity::Create( "monster_snark", pev->origin + Vector(8, -8, 0), m_vecDir , pev->owner );
        pSqueak->pev->velocity = m_vecDir * 200;
	pSqueak = CBaseEntity::Create( "monster_snark", pev->origin + Vector(-8, -8, 0), m_vecDir , pev->owner );
        pSqueak->pev->velocity = m_vecDir * 200;

	// Tell the mine what do do for the next little while.
	SetThink( &CTripSnarkGrenade::RiftThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_RiftTime = gpGlobals->time + 3.0f;
}

void CTripSnarkGrenade::RiftThink( void )
{
	// Setup next think time
	pev->nextthink = gpGlobals->time + 0.1f;

	// Make a lightning strike
	Vector vecEnd;
	TraceResult tr;
	vecEnd.x = RANDOM_FLOAT(-1.0,1.0);	// Pick a random direction
	vecEnd.y = RANDOM_FLOAT(-1.0,1.0);
	vecEnd.z = RANDOM_FLOAT(-1.0,1.0);
	// vecEnd = vecEnd.Normalize();
	vecEnd = pev->origin + vecEnd.Normalize() * 128;

	UTIL_TraceLine( pev->origin, vecEnd, ignore_monsters, ENT(pev), &tr);

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_BEAMPOINTS );
                WRITE_COORD(pev->origin.x);
                WRITE_COORD(pev->origin.y);
                WRITE_COORD(pev->origin.z);
                WRITE_COORD( tr.vecEndPos.x );
                WRITE_COORD( tr.vecEndPos.y );
                WRITE_COORD( tr.vecEndPos.z );
                WRITE_SHORT( m_LaserSprite );
                WRITE_BYTE( 0 ); // Starting frame
                WRITE_BYTE( 0  ); // framerate * 0.1
                WRITE_BYTE( 5 ); // life * 0.1
                WRITE_BYTE( 3 ); // width
                WRITE_BYTE( 64 ); // noise
                WRITE_BYTE( 10 ); // color r,g,b
                WRITE_BYTE( 200 ); // color r,g,b
                WRITE_BYTE( 255 ); // color r,g,b
                WRITE_BYTE( 255 ); // brightness
                WRITE_BYTE( 0 ); // scroll speed
        MESSAGE_END();

	// Animate Sprite
	m_pSprite->Animate(1);

	// Check to see if the rift should fade away
	if (m_RiftTime <= gpGlobals->time) {
		m_pSprite->Expand( 10, 500 );	
		m_pSprite = NULL;
		UTIL_Remove( this );
	}
}

void CTripSnarkGrenade::DelayDeathThink( void )
{
	KillBeam();
	TraceResult tr;
	UTIL_TraceLine ( pev->origin + m_vecDir * 8, pev->origin - m_vecDir * 64,  dont_ignore_monsters, ENT(pev), & tr);

	Explode( &tr );
}

BOOL CTripSnarkGrenade::IsSpawnMine()
{
	if (bm_spawnmines.value)
		return FALSE;

	BOOL result = FALSE;

	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	Vector vecSpot;
	Vector vecSrc = pev->origin;
	float flRadius = 375;

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

void CTripSnarkGrenade::Deactivate( void )
{
	pev->solid = SOLID_NOT;
	KillBeam();
	UTIL_Remove( this );
}

//=========================================================
// DeactivateSnarkTrips - removes all snark trips owned by
// the provided player.
//
// Made this global on purpose.
//=========================================================
void DeactivateSnarkTrips( CBasePlayer *pOwner )
{
	edict_t *pFind; 

	pFind = FIND_ENTITY_BY_CLASSNAME( NULL, "monster_tripsnark" );

	while ( !FNullEnt( pFind ) )
	{
		CBaseEntity *pEnt = CBaseEntity::Instance( pFind );
		CTripSnarkGrenade *pTrip = (CTripSnarkGrenade *)pEnt;

		if ( pTrip )
		{
			if ( pTrip->Owner() == pOwner->edict() )
			{
				pTrip->Deactivate();
			}
		}

		pFind = FIND_ENTITY_BY_CLASSNAME( pFind, "monster_tripsnark" );
	}
}
