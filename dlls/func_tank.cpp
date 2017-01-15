/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"

#include "player.h"

#include "func_tank.h"

//=========================================================
// CFuncTank
//=========================================================
/*
#define SF_TANK_ACTIVE			0x0001
#define SF_TANK_PLAYER			0x0002
#define SF_TANK_HUMANS			0x0004
#define SF_TANK_ALIENS			0x0008
#define SF_TANK_LINEOFSIGHT		0x0010
#define SF_TANK_CANCONTROL		0x0020
#define SF_TANK_SOUNDON			0x8000

enum TANKBULLET
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3
};
*/

TYPEDESCRIPTION	CFuncTank::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTank, m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_fireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_persist, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteSmoke, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteFlash, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_bulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_sightOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spread, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_pController, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTank, m_vecControllerUsePos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_iBulletDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_iszMaster, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncTank, CBaseEntity )

static Vector gTankSpread[] =
{
	Vector( 0, 0, 0 ),		// perfect
	Vector( 0.025, 0.025, 0.025 ),	// small cone
	Vector( 0.05, 0.05, 0.05 ),  // medium cone
	Vector( 0.1, 0.1, 0.1 ),	// large cone
	Vector( 0.25, 0.25, 0.25 ),	// extra-large cone
};

#define MAX_FIRING_SPREADS ARRAYSIZE( gTankSpread )

void CFuncTank::Spawn( void )
{
	Precache();

	pev->movetype = MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid = SOLID_BSP;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;

	if( IsActive() )
		pev->nextthink = pev->ltime + 1.0;

	m_sightOrigin = BarrelPosition(); // Point at the end of the barrel

	if( m_fireRate <= 0 )
		m_fireRate = 1;
	if( m_spread > MAX_FIRING_SPREADS )
		m_spread = 0;

	pev->oldorigin = pev->origin;
}

void CFuncTank::Precache( void )
{
	if( m_iszSpriteSmoke )
		PRECACHE_MODEL( (char *)STRING( m_iszSpriteSmoke ) );

	if( m_iszSpriteFlash )
		PRECACHE_MODEL( (char *)STRING( m_iszSpriteFlash ) );

	if( pev->noise )
		PRECACHE_SOUND( (char *)STRING( pev->noise ) );
}

void CFuncTank::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "yawrate" ) )
	{
		m_yawRate = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "yawrange" ) )
	{
		m_yawRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "yawtolerance" ) )
	{
		m_yawTolerance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchrange" ) )
	{
		m_pitchRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchrate" ) )
	{
		m_pitchRate = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "pitchtolerance" ) )
	{
		m_pitchTolerance = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "firerate" ) )
	{
		m_fireRate = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "barrel" ) )
	{
		m_barrelPos.x = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "barrely" ) )
	{
		m_barrelPos.y = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "barrelz" ) )
	{
		m_barrelPos.z = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spritescale" ) )
	{
		m_spriteScale = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spritesmoke" ) )
	{
		m_iszSpriteSmoke = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "spriteflash" ) )
	{
		m_iszSpriteFlash = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "rotatesound" ) )
	{
		pev->noise = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "persistence" ) )
	{
		m_persist = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bullet" ) )
	{
		m_bulletType = (TANKBULLET)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bullet_damage" ) )
	{
		m_iBulletDamage = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq(pkvd->szKeyName, "firespread" ) )
	{
		m_spread = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "minRange" ) )
	{
		m_minRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxRange" ) )
	{
		m_maxRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "master" ) )
	{
		m_iszMaster = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

////////////// START NEW STUFF //////////////

//==================================================================================
// TANK CONTROLLING
BOOL CFuncTank::OnControls( entvars_t *pevTest )
{
	if( !( pev->spawnflags & SF_TANK_CANCONTROL ) )
		return FALSE;

	Vector offset = pevTest->origin - pev->origin;

	if( ( m_vecControllerUsePos - pevTest->origin ).Length() < 30 )
		return TRUE;

	return FALSE;
}

BOOL CFuncTank::StartControl( CBasePlayer *pController )
{
	if( m_pController != NULL )
		return FALSE;

	// Team only or disabled?
	if( m_iszMaster )
	{
		if( !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
			return FALSE;
	}

	ALERT( at_console, "using TANK!\n");

	m_pController = pController;
	if( m_pController->m_pActiveItem )
	{
		m_pController->m_pActiveItem->Holster();
		m_pController->pev->weaponmodel = 0;
		m_pController->pev->viewmodel = 0;
	}

	m_pController->m_iHideHUD |= HIDEHUD_WEAPONS;
	m_vecControllerUsePos = m_pController->pev->origin;

	pev->nextthink = pev->ltime + 0.1;

	return TRUE;
}

void CFuncTank::StopControl()
{
	// TODO: bring back the controllers current weapon
	if( !m_pController )
		return;

	if( m_pController->m_pActiveItem )
		m_pController->m_pActiveItem->Deploy();

	ALERT( at_console, "stopped using TANK\n");

	m_pController->m_iHideHUD &= ~HIDEHUD_WEAPONS;

	pev->nextthink = 0;
	m_pController = NULL;

	if( IsActive() )
		pev->nextthink = pev->ltime + 1.0;
}

// Called each frame by the player's ItemPostFrame
void CFuncTank::ControllerPostFrame( void )
{
	ASSERT( m_pController != NULL );

	if( gpGlobals->time < m_flNextAttack )
		return;

	if( m_pController->pev->button & IN_ATTACK )
	{
		Vector vecForward;
		UTIL_MakeVectorsPrivate( pev->angles, vecForward, NULL, NULL );

		m_fireLast = gpGlobals->time - ( 1 / m_fireRate ) - 0.01;  // to make sure the gun doesn't fire too many bullets

		Fire( BarrelPosition(), vecForward, m_pController->pev );
		
		// HACKHACK -- make some noise (that the AI can hear)
		if( m_pController && m_pController->IsPlayer() )
			( (CBasePlayer *)m_pController )->m_iWeaponVolume = LOUD_GUN_VOLUME;

		m_flNextAttack = gpGlobals->time + ( 1 / m_fireRate );
	}
}
////////////// END NEW STUFF //////////////

void CFuncTank::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( pev->spawnflags & SF_TANK_CANCONTROL )
	{
		// player controlled turret
		if( pActivator->Classify() != CLASS_PLAYER )
			return;

		if( value == 2 && useType == USE_SET )
		{
			ControllerPostFrame();
		}
		else if( !m_pController && useType != USE_OFF )
		{
			( (CBasePlayer*)pActivator )->m_pTank = this;
			StartControl( (CBasePlayer*)pActivator );
		}
		else
		{
			StopControl();
		}
	}
	else
	{
		if( !ShouldToggle( useType, IsActive() ) )
			return;

		if( IsActive() )
			TankDeactivate();
		else
			TankActivate();
	}
}

edict_t *CFuncTank::FindTarget( edict_t *pPlayer )
{
	return pPlayer;
}

BOOL CFuncTank::InRange( float range )
{
	if( range < m_minRange )
		return FALSE;
	if( m_maxRange > 0 && range > m_maxRange )
		return FALSE;

	return TRUE;
}

void CFuncTank::Think( void )
{
	pev->avelocity = g_vecZero;
	TrackTarget();

	if( fabs( pev->avelocity.x ) > 1 || fabs( pev->avelocity.y ) > 1 )
		StartRotSound();
	else
		StopRotSound();
}

void CFuncTank::TrackTarget( void )
{
	TraceResult tr;
	edict_t *pPlayer = FIND_CLIENT_IN_PVS( edict() );
	BOOL updateTime = FALSE, lineOfSight;
	Vector angles, direction, targetPosition, barrelEnd;
	edict_t *pTarget;

	// Get a position to aim for
	if( m_pController )
	{
		// Tanks attempt to mirror the player's angles
		angles = m_pController->pev->v_angle;
		angles[0] = 0 - angles[0];
		pev->nextthink = pev->ltime + 0.05;
	}
	else
	{
		if( IsActive() )
			pev->nextthink = pev->ltime + 0.1;
		else
			return;

		if( FNullEnt( pPlayer ) )
		{
			if( IsActive() )
				pev->nextthink = pev->ltime + 2;	// Wait 2 secs
			return;
		}
		pTarget = FindTarget( pPlayer );
		if( !pTarget )
			return;

		// Calculate angle needed to aim at target
		barrelEnd = BarrelPosition();
		targetPosition = pTarget->v.origin + pTarget->v.view_ofs;
		float range = ( targetPosition - barrelEnd ).Length();

		if( !InRange( range ) )
			return;

		UTIL_TraceLine( barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr );

		lineOfSight = FALSE;
		// No line of sight, don't track
		if( tr.flFraction == 1.0 || tr.pHit == pTarget )
		{
			lineOfSight = TRUE;

			CBaseEntity *pInstance = CBaseEntity::Instance(pTarget);
			if( InRange( range ) && pInstance && pInstance->IsAlive() )
			{
				updateTime = TRUE;
				m_sightOrigin = UpdateTargetPosition( pInstance );
			}
		}

		// Track sight origin
		// !!! I'm not sure what i changed
		direction = m_sightOrigin - pev->origin;
		//direction = m_sightOrigin - barrelEnd;
		angles = UTIL_VecToAngles( direction );

		// Calculate the additional rotation to point the end of the barrel at the target (not the gun's center) 
		AdjustAnglesForBarrel( angles, direction.Length() );
	}

	angles.x = -angles.x;

	// Force the angles to be relative to the center position
	angles.y = m_yawCenter + UTIL_AngleDistance( angles.y, m_yawCenter );
	angles.x = m_pitchCenter + UTIL_AngleDistance( angles.x, m_pitchCenter );

	// Limit against range in y
	if( angles.y > m_yawCenter + m_yawRange )
	{
		angles.y = m_yawCenter + m_yawRange;
		updateTime = FALSE;	// Don't update if you saw the player, but out of range
	}
	else if( angles.y < ( m_yawCenter - m_yawRange ) )
	{
		angles.y = ( m_yawCenter - m_yawRange );
		updateTime = FALSE; // Don't update if you saw the player, but out of range
	}

	if( updateTime )
		m_lastSightTime = gpGlobals->time;

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( angles.y, pev->angles.y );
	pev->avelocity.y = distY * 10;
	if( pev->avelocity.y > m_yawRate )
		pev->avelocity.y = m_yawRate;
	else if( pev->avelocity.y < -m_yawRate )
		pev->avelocity.y = -m_yawRate;

	// Limit against range in x
	if( angles.x > m_pitchCenter + m_pitchRange )
		angles.x = m_pitchCenter + m_pitchRange;
	else if( angles.x < m_pitchCenter - m_pitchRange )
		angles.x = m_pitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( angles.x, pev->angles.x );
	pev->avelocity.x = distX  * 10;

	if( pev->avelocity.x > m_pitchRate )
		pev->avelocity.x = m_pitchRate;
	else if( pev->avelocity.x < -m_pitchRate )
		pev->avelocity.x = -m_pitchRate;

	if( m_pController )
		return;

	if( CanFire() && ( ( fabs( distX ) < m_pitchTolerance && fabs( distY ) < m_yawTolerance ) || ( pev->spawnflags & SF_TANK_LINEOFSIGHT ) ) )
	{
		BOOL fire = FALSE;
		Vector forward;
		UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

		if( pev->spawnflags & SF_TANK_LINEOFSIGHT )
		{
			float length = direction.Length();
			UTIL_TraceLine( barrelEnd, barrelEnd + forward * length, dont_ignore_monsters, edict(), &tr );
			if( tr.pHit == pTarget )
				fire = TRUE;
		}
		else
			fire = TRUE;

		if( fire )
		{
			Fire( BarrelPosition(), forward, pev );
		}
		else
			m_fireLast = 0;
	}
	else
		m_fireLast = 0;
}

// If barrel is offset, add in additional rotation
void CFuncTank::AdjustAnglesForBarrel( Vector &angles, float distance )
{
	float r2, d2;

	if( m_barrelPos.y != 0 || m_barrelPos.z != 0 )
	{
		distance -= m_barrelPos.z;
		d2 = distance * distance;
		if( m_barrelPos.y )
		{
			r2 = m_barrelPos.y * m_barrelPos.y;
			angles.y += ( 180.0 / M_PI ) * atan2( m_barrelPos.y, sqrt( d2 - r2 ) );
		}
		if( m_barrelPos.z )
		{
			r2 = m_barrelPos.z * m_barrelPos.z;
			angles.x += ( 180.0 / M_PI ) * atan2( -m_barrelPos.z, sqrt( d2 - r2 ) );
		}
	}
}

// Fire targets and spawn sprites
void CFuncTank::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if( m_fireLast != 0 )
	{
		if( m_iszSpriteSmoke )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteSmoke ), barrelEnd, TRUE );
			pSprite->AnimateAndDie( RANDOM_FLOAT( 15.0, 20.0 ) );
			pSprite->SetTransparency( kRenderTransAlpha, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, 255, kRenderFxNone );
			pSprite->pev->velocity.z = RANDOM_FLOAT( 40, 80 );
			pSprite->SetScale( m_spriteScale );
		}
		if( m_iszSpriteFlash )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING( m_iszSpriteFlash ), barrelEnd, TRUE );
			pSprite->AnimateAndDie( 60 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetScale( m_spriteScale );

			// Hack Hack, make it stick around for at least 100 ms.
			pSprite->pev->nextthink += 0.1;
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
	}
	m_fireLast = gpGlobals->time;
}

void CFuncTank::TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr )
{
	// get circular gaussian spread
	float x, y, z;
	do
	{
		x = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		y = RANDOM_FLOAT( -0.5, 0.5 ) + RANDOM_FLOAT( -0.5, 0.5 );
		z = x * x + y * y;
	} while( z > 1 );
	Vector vecDir = vecForward +
		x * vecSpread.x * gpGlobals->v_right +
		y * vecSpread.y * gpGlobals->v_up;
	Vector vecEnd;

	vecEnd = vecStart + vecDir * 4096;
	UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &tr );
}
	
void CFuncTank::StartRotSound( void )
{
	if( !pev->noise || ( pev->spawnflags & SF_TANK_SOUNDON ) )
		return;
	pev->spawnflags |= SF_TANK_SOUNDON;
	EMIT_SOUND( edict(), CHAN_STATIC, (char*)STRING( pev->noise ), 0.85, ATTN_NORM );
}

void CFuncTank::StopRotSound( void )
{
	if( pev->spawnflags & SF_TANK_SOUNDON )
		STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING( pev->noise ) );
	pev->spawnflags &= ~SF_TANK_SOUNDON;
}

//=========================================================
// CFuncTankGun
//=========================================================
LINK_ENTITY_TO_CLASS( func_tank, CFuncTankGun )

void CFuncTankGun::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;

	if( m_fireLast != 0 )
	{
		// FireBullets needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors( pev->angles );

		int bulletCount = ( gpGlobals->time - m_fireLast ) * m_fireRate;
		if( bulletCount > 0 )
		{
			for( i = 0; i < bulletCount; i++ )
			{
				switch( m_bulletType )
				{
				case TANK_BULLET_9MM:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_9MM, 1, m_iBulletDamage, pevAttacker );
					break;
				case TANK_BULLET_MP5:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_MP5, 1, m_iBulletDamage, pevAttacker );
					break;
				case TANK_BULLET_12MM:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_12MM, 1, m_iBulletDamage, pevAttacker );
					break;
				default:
				case TANK_BULLET_NONE:
					break;
				}
			}
			CFuncTank::Fire( barrelEnd, forward, pevAttacker );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pevAttacker );
}

//=========================================================
// CFuncTankLaser
//=========================================================
LINK_ENTITY_TO_CLASS( func_tanklaser, CFuncTankLaser )

TYPEDESCRIPTION	CFuncTankLaser::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTankLaser, m_pLaser, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankLaser, m_laserTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFuncTankLaser, CFuncTank )

void CFuncTankLaser::Activate( void )
{
	if( !GetLaser() )
	{
		UTIL_Remove( this );
		ALERT( at_error, "Laser tank with no env_laser!\n" );
	}
	else
	{
		m_pLaser->TurnOff();
	}
}

void CFuncTankLaser::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "laserentity" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

CLaser *CFuncTankLaser::GetLaser( void )
{
	if( m_pLaser )
		return m_pLaser;

	edict_t	*pentLaser;

	pentLaser = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->message ) );
	while( !FNullEnt( pentLaser ) )
	{
		// Found the landmark
		if( FClassnameIs( pentLaser, "env_laser" ) )
		{
			m_pLaser = (CLaser *)CBaseEntity::Instance( pentLaser );
			break;
		}
		else
			pentLaser = FIND_ENTITY_BY_TARGETNAME( pentLaser, STRING( pev->message ) );
	}

	return m_pLaser;
}

void CFuncTankLaser::Think( void )
{
	if( m_pLaser && (gpGlobals->time > m_laserTime) )
		m_pLaser->TurnOff();

	CFuncTank::Think();
}

void CFuncTankLaser::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;
	TraceResult tr;

	if( m_fireLast != 0 && GetLaser() )
	{
		// TankTrace needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors( pev->angles );

		int bulletCount = ( gpGlobals->time - m_fireLast ) * m_fireRate;
		if( bulletCount )
		{
			for( i = 0; i < bulletCount; i++ )
			{
				m_pLaser->pev->origin = barrelEnd;
				TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );

				m_laserTime = gpGlobals->time;
				m_pLaser->TurnOn();
				m_pLaser->pev->dmgtime = gpGlobals->time - 1.0;
				m_pLaser->FireAtPoint( tr );
				m_pLaser->pev->nextthink = 0;
			}
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
	{
		CFuncTank::Fire( barrelEnd, forward, pev );
	}
}

//=========================================================
// CFuncTankRocket
//=========================================================
LINK_ENTITY_TO_CLASS( func_tankrocket, CFuncTankRocket )

void CFuncTankRocket::Precache( void )
{
	UTIL_PrecacheOther( "rpg_rocket" );
	CFuncTank::Precache();
}

void CFuncTankRocket::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;

	if( m_fireLast != 0 )
	{
		int bulletCount = ( gpGlobals->time - m_fireLast ) * m_fireRate;
		if( bulletCount > 0 )
		{
			for( i = 0; i < bulletCount; i++ )
			{
				CBaseEntity *pRocket = CBaseEntity::Create( "rpg_rocket", barrelEnd, pev->angles, edict() );
			}
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}

//=========================================================
// CFuncTankMortar
//=========================================================
LINK_ENTITY_TO_CLASS( func_tankmortar, CFuncTankMortar )

void CFuncTankMortar::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "iMagnitude" ) )
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}

void CFuncTankMortar::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if( m_fireLast != 0 )
	{
		int bulletCount = ( gpGlobals->time - m_fireLast ) * m_fireRate;
		// Only create 1 explosion
		if( bulletCount > 0 )
		{
			TraceResult tr;

			// TankTrace needs gpGlobals->v_up, etc.
			UTIL_MakeAimVectors( pev->angles );

			TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );

			ExplosionCreate( tr.vecEndPos, pev->angles, edict(), pev->impulse, TRUE );

			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}

//============================================================================
// FUNC TANK CONTROLS
//============================================================================
LINK_ENTITY_TO_CLASS( func_tankcontrols, CFuncTankControls )

TYPEDESCRIPTION	CFuncTankControls::m_SaveData[] =
{
	DEFINE_FIELD( CFuncTankControls, m_pTank, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CFuncTankControls, CBaseEntity )

int CFuncTankControls::ObjectCaps( void ) 
{ 
	return ( CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION ) | FCAP_IMPULSE_USE; 
}

void CFuncTankControls::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// pass the Use command onto the controls
	if( m_pTank )
		m_pTank->Use( pActivator, pCaller, useType, value );

	ASSERT( m_pTank != NULL );	// if this fails,  most likely means save/restore hasn't worked properly
}

void CFuncTankControls::Think( void )
{
	edict_t *pTarget = NULL;

	do
	{
		pTarget = FIND_ENTITY_BY_TARGETNAME( pTarget, STRING( pev->target ) );
	} while( !FNullEnt( pTarget ) && strncmp( STRING( pTarget->v.classname ), "func_tank", 9 ) );

	if( FNullEnt( pTarget ) )
	{
		ALERT( at_console, "No tank %s\n", STRING( pev->target ) );
		return;
	}

	m_pTank = (CFuncTank*)Instance( pTarget );
}

void CFuncTankControls::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );

	pev->nextthink = gpGlobals->time + 0.3;	// After all the func_tank's have spawned

	CBaseEntity::Spawn();
}
