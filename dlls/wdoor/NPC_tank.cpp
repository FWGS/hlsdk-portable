/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
/*

===== turret.cpp ========================================================

*/

//
// TODO: 
//		Take advantage of new monster fields like m_hEnemy and get rid of that OFFSET() stuff
//		Revisit enemy validation stuff, maybe it's not necessary with the newest monster code
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "animation.h"
#include "player.h"

typedef enum
{
	TANK_ANIM_IDLE = 0,
	TANK_ANIM_FIRE
} TANK_ANIM;

class CBaseTurret_Tank : public CBaseMonster
{
public:
	void Spawn(void);
	virtual void Precache(void);

	virtual void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	virtual int	 TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	virtual int	 Classify(void);

	int BloodColor( void ) { return DONT_BLEED; }
	void GibMonster( void ) {}	// UNDONE: Throw turret gibs?

	// Think functions

	void EXPORT ActiveThink(void);
	void EXPORT SearchThink(void);
	void EXPORT TurretDeath(void);

	void EXPORT Initialize(void);
	
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// other functions
	void SetTurretAnim(TANK_ANIM anim);
	int MoveTurret(void);
	virtual void Shoot(Vector &vecSrc, Vector &vecDirToEnemy) { };

	int m_iBaseTurnRate;	// angles per second
	float m_fTurnRate;		// actual turn rate

	Vector m_vecLastSight;
	float m_flLastSight;	// Last time we saw a target
	float m_flMaxWait;		// Max time to seach w/o a target
	
	int m_ion;
	int m_ifuck;
	int m_search_player;

	// movement
	float	m_flStartYaw;
	Vector	m_vecCurAngles;
	Vector	m_vecGoalAngles;
};


TYPEDESCRIPTION	CBaseTurret_Tank::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseTurret_Tank, m_ion, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret_Tank, m_ifuck, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret_Tank, m_search_player, FIELD_INTEGER ),

	DEFINE_FIELD( CBaseTurret_Tank, m_iBaseTurnRate, FIELD_INTEGER ),
	DEFINE_FIELD( CBaseTurret_Tank, m_fTurnRate, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseTurret_Tank, m_vecLastSight, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CBaseTurret_Tank, m_flLastSight, FIELD_TIME ),
	DEFINE_FIELD( CBaseTurret_Tank, m_flMaxWait, FIELD_FLOAT ),

	DEFINE_FIELD( CBaseTurret_Tank, m_flStartYaw, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseTurret_Tank, m_vecCurAngles, FIELD_VECTOR ),
	DEFINE_FIELD( CBaseTurret_Tank, m_vecGoalAngles, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CBaseTurret_Tank, CBaseMonster );

class CTank_Monster : public CBaseTurret_Tank
{
public:
	void Spawn(void);
	void Precache(void);
	// Think functions
	void SpinUpCall(void);
	void SpinDownCall(void);
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	
	static	TYPEDESCRIPTION m_SaveData[];

	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);

private:
	int m_iStartSpin;

};


void CTank_Monster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 1:
		{
		UTIL_MakeAimVectors(m_vecCurAngles);
		Vector vecSrc, vecAng;
		GetAttachment( 0, vecSrc, vecAng );
		Shoot(vecSrc, gpGlobals->v_forward );
		}
		break;

	default:
		ALERT( at_aiconsole, "Unhandled animation event %d for %s\n", pEvent->event, STRING(pev->classname) );
		break;

	}
}

TYPEDESCRIPTION	CTank_Monster::m_SaveData[] = 
{
	DEFINE_FIELD( CTank_Monster, m_iStartSpin, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CTank_Monster, CBaseTurret_Tank );

LINK_ENTITY_TO_CLASS( monster_tank, CTank_Monster );

void CBaseTurret_Tank::Spawn()
{ 
	Precache( );
	pev->nextthink		= gpGlobals->time + 1;
	pev->movetype		= MOVETYPE_FLY;
	pev->sequence		= 0;
	pev->frame			= 0;
	pev->solid			= SOLID_SLIDEBOX;
	pev->takedamage		= DAMAGE_AIM;

	SetBits (pev->flags, FL_MONSTER);

	ResetSequenceInfo( );
	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );
	m_flFieldOfView = VIEW_FIELD_FULL;
	// m_flSightRange = TURRET_RANGE;
}


void CBaseTurret_Tank::Precache( )
{
	PRECACHE_SOUND ("tank/tank_explode.wav");
	PRECACHE_SOUND ("tank/tank_fire.wav");
	PRECACHE_SOUND ("tank/tank_idle.wav");
}

void CTank_Monster::Spawn()
{ 
	Precache( );
	SET_MODEL(ENT(pev), "models/tank_m1a1.mdl");
	pev->health			= 1800;
	pev->max_health			= pev->health;
	m_HackedGunPos		= Vector( 0, 0, 100 );

	pev->view_ofs.z = 90;

	CBaseTurret_Tank::Spawn( );
	
	DROP_TO_FLOOR ( ENT(pev) );
	pev->movetype		= MOVETYPE_FLY;
	pev->gravity        = 3.0;

	SetThink(&CTank_Monster::Initialize);	
	m_canbarnacle_mode  = 0;

	pev->nextthink = gpGlobals->time + 0.3; 
	m_killed_exp = 600;
	pev->netname = MAKE_STRING( "Tank" );
}

void CTank_Monster::Precache()
{
	CBaseTurret_Tank::Precache( );
	PRECACHE_MODEL ("models/tank_m1a1.mdl");
	PRECACHE_MODEL ("models/tank_gibs.mdl");
//	PRECACHE_MODEL( "models/tank_gibs2.mdl" );
}

void CBaseTurret_Tank::Initialize(void)
{
				Vector angle_old = pev->angles;
				TraceResult	tr;
				UTIL_TraceLine( pev->origin + Vector(0,0,8), pev->origin - Vector(0,0,8), dont_ignore_monsters, ENT( pev ), &tr );
				if (tr.flFraction < 1.0)
				{
					CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
					if ( !(pEntity->pev->flags & FL_CONVEYOR) )
					{
						pev->angles = UTIL_VecToAngles( tr.vecPlaneNormal );
						pev->angles.x -= 90;
						if(pev->angles.x == 0 && pev->angles.z == 0){
						pev->angles.y = angle_old.y;
						}
					}
				}

	if(pev->angles.y >= 0 && pev->angles.y < 90){
	UTIL_SetSize(pev, Vector(-155, -75, 0), Vector(155, 75, 110));
	}
	else if(pev->angles.y >= 180 && pev->angles.y < 270){
	UTIL_SetSize(pev, Vector(-155, -75, 0), Vector(155, 75, 110));
	}
	else{
	UTIL_SetSize(pev, Vector(-75, -155, 0), Vector(75, 155, 110));
	}

	SetBoneController( 0, 0 );
	SetBoneController( 1, 0 );

	m_iBaseTurnRate = 10;

	m_flStartYaw = pev->angles.y;

	m_vecGoalAngles.x = 0;
	m_vecGoalAngles.y = m_flStartYaw;

	m_flLastSight = gpGlobals->time + 1;
	SetThink(&CBaseTurret_Tank::SearchThink);		
 	pev->nextthink = gpGlobals->time + 0.5;
}


void CBaseTurret_Tank::ActiveThink(void)
{
	int fAttack = 0;
	Vector vecDirToEnemy;

	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance( );
	DispatchAnimEvents( StudioFrameAdvance() );

	if(m_ion > 0){
		m_ion--;
		UTIL_MakeVectors ( pev->angles );
		TraceResult tr;
		Vector vecspot = pev->origin + Vector(0,0,15);
		if(m_ifuck == 0){
			UTIL_TraceLine( vecspot, vecspot + gpGlobals->v_forward * 180, dont_ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
			m_ifuck = 1;
			m_ion = 0;
			pev->velocity = g_vecZero;
			DROP_TO_FLOOR ( ENT(pev) );
			}
			else{
			pev->velocity = gpGlobals->v_forward * 120;
			DROP_TO_FLOOR ( ENT(pev) );
			}
		}
		else{
			UTIL_TraceLine( vecspot, vecspot + gpGlobals->v_forward * -180, dont_ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
			m_ifuck = 0;
			m_ion = 0;
			pev->velocity = g_vecZero;
			DROP_TO_FLOOR ( ENT(pev) );
			}
			else{
			pev->velocity = gpGlobals->v_forward * -120;
			DROP_TO_FLOOR ( ENT(pev) );
			}
		}
	}
	else{
		pev->velocity = g_vecZero;
		DROP_TO_FLOOR ( ENT(pev) );
	}

	if (m_hEnemy == NULL)
	{
		m_ion = 100;
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + 4;
		SetThink(&CBaseTurret_Tank::SearchThink);
		return;
	}
	
	// if it's dead, look for something new
	if ( !m_hEnemy->IsAlive() )
	{
		if (!m_flLastSight)
		{
			m_flLastSight = gpGlobals->time + 2; // continue-shooting timeout
		}
		else
		{
			if (gpGlobals->time > m_flLastSight)
			{	
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + 4;
				SetThink(&CBaseTurret_Tank::SearchThink);
				return;
			}
		}
	}
	Vector oor,oc;
	GetAttachment( 0, oor, oc );
	Vector vecMid = oor;
	Vector vecMidEnemy = m_hEnemy->BodyTarget_e( vecMid );

	// Look for our current enemy
	int fEnemyVisible = FBoxVisible(pev, m_hEnemy->pev, vecMidEnemy );	

	vecDirToEnemy = vecMidEnemy - vecMid;	// calculate dir and dist to enemy
	float flDistToEnemy = vecDirToEnemy.Length();

	Vector vec = UTIL_VecToAngles(vecMidEnemy - vecMid);	

	// Current enmey is not visible.
	if (!fEnemyVisible || (flDistToEnemy > 4096))
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 4;
		else
		{
			// Should we look for a new target?
			if (gpGlobals->time > m_flLastSight)
			{
				m_ion = 100;
				m_hEnemy = NULL;
				m_flLastSight = gpGlobals->time + 4;
				SetThink(&CBaseTurret_Tank::SearchThink);
				return;
			}
		}
		fEnemyVisible = 0;
	}
	else
	{
		m_vecLastSight = vecMidEnemy;
	}

	UTIL_MakeAimVectors(m_vecCurAngles);	

	/*
	ALERT( at_console, "%.0f %.0f : %.2f %.2f %.2f\n", 
		m_vecCurAngles.x, m_vecCurAngles.y,
		gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_forward.z );
	*/
	
	Vector vecLOS = vecDirToEnemy; //vecMid - m_vecLastSight;
	vecLOS = vecLOS.Normalize();

	// Is the Gun looking at the target
	if (DotProduct(vecLOS, gpGlobals->v_forward) <= 0.9){ // 30 degree slop{
		fAttack = FALSE;
	}
	else{
		fAttack = TRUE;
	}

	// fire the gun
	if (fAttack)
	{
		SetTurretAnim(TANK_ANIM_FIRE);
	} 
	else
	{
		SetTurretAnim(TANK_ANIM_IDLE);
	}

	//move the gun

	if (fEnemyVisible)
	{
		if (vec.y > 360)
			vec.y -= 360;

		if (vec.y < 0)
			vec.y += 360;

		//ALERT(at_console, "[%.2f]", vec.x);
		
		if (vec.x < -180)
			vec.x += 360;

		if (vec.x > 180)
			vec.x -= 360;

		// now all numbers should be in [1...360]
		// pin to turret limitations to [-90...15]

			if (vec.x > 50)
				vec.x = 50;
			else if (vec.x < -5)
				vec.x = -5;

		// ALERT(at_console, "->[%.2f]\n", vec.x);

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;

	}
	if ( fabs(m_vecGoalAngles.x - m_vecCurAngles.x) >= 3 ){
	MoveTurret();
	}
	else if ( fabs(m_vecGoalAngles.y - m_vecCurAngles.y) >= 3 ){
	MoveTurret();
	}
}


void CTank_Monster::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "tank/tank_fire.wav", 1, 0.4);
	FireBullets( 1, vecSrc, vecDirToEnemy, Vector(0.03,0.03,0.03), 8192, 810, 0 );
}

void CBaseTurret_Tank::SetTurretAnim(TANK_ANIM anim)
{
	if (pev->sequence != anim)
	{
		pev->frame = 0;
		pev->sequence = anim;
		ResetSequenceInfo( );
		//ALERT(at_console, "Turret anim #%d\n", anim);
	}
}


//
// This search function will sit with the turret deployed and look for a new target. 
// After a set amount of time, the barrel will spin down. After m_flMaxWait, the turret will
// retact.
//
void CBaseTurret_Tank::SearchThink(void)
{
	// ensure rethink
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	if(m_search_player == 0){
		if (!FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) ){
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "player" );
			if ( pEntity )//���
			{
				CBasePlayer *pPlayer = GetClassPtr((CBasePlayer *)pEntity->pev);
				pPlayer->BOSS_Find();
				m_search_player = 1;
			}
		}
	}

	if(m_ion > 0){
		m_ion--;
		UTIL_MakeVectors ( pev->angles );
		TraceResult tr;
		Vector vecspot = pev->origin + Vector(0,0,15);
		if(m_ifuck == 0){
			UTIL_TraceLine( vecspot, vecspot + gpGlobals->v_forward * 180, dont_ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
			m_ifuck = 1;
			m_ion = 0;
			pev->velocity = g_vecZero;
			DROP_TO_FLOOR ( ENT(pev) );
			}
			else{
			pev->velocity = gpGlobals->v_forward * 120;
			DROP_TO_FLOOR ( ENT(pev) );
			}
		}
		else{
			UTIL_TraceLine( vecspot, vecspot + gpGlobals->v_forward * -180, dont_ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction != 1.0 )
			{
			m_ifuck = 0;
			m_ion = 0;
			pev->velocity = g_vecZero;
			DROP_TO_FLOOR ( ENT(pev) );
			}
			else{
			pev->velocity = gpGlobals->v_forward * -120;
			DROP_TO_FLOOR ( ENT(pev) );
			}
		}
	}
	else{
		pev->velocity = g_vecZero;
		DROP_TO_FLOOR ( ENT(pev) );
	}

	// If we have a target and we're still healthy
	if (m_hEnemy != NULL)
	{
		if (!m_hEnemy->IsAlive() )
			m_hEnemy = NULL;// Dead enemy forces a search for new one
	}


	// Acquire Target
	if (m_hEnemy == NULL)
	{
		Look(4096);
		m_hEnemy = BestVisibleEnemy();
	}

	// If we've found a target, spin up the barrel and start to attack
	if (m_hEnemy != NULL)
	{
		m_flLastSight = 0;
		if(pev->frags <= 0){
		SetThink(&CBaseTurret_Tank::ActiveThink);
		}
		else{
		pev->frags -= 1;
		}
	}
	else
	{
		// stop moving
		SetTurretAnim(TANK_ANIM_IDLE);
	}
}

void CBaseTurret_Tank ::	TurretDeath( void )
{
	Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	FX_Explosion( vecSpot, EXPLOSION_C4 );

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 32);

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 200 );

			// velocity
			WRITE_COORD( 0 ); 
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 30 ); 
			int m_iBodyGibs;
			m_iBodyGibs = PRECACHE_MODEL( "models/tank_gibs.mdl" );
			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 200 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags
			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 0 ); // no sprite
			WRITE_BYTE( 15  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		EMIT_SOUND(ENT(pev), CHAN_STATIC, "tank/tank_explode.wav", 1.0, 0.3);

		RadiusDamage( pev->origin, pev, pev, 250, CLASS_NONE, DMG_BLAST );

		UTIL_Remove( this );
		pev->framerate = 0;
		SetThink( NULL );
}



void CBaseTurret_Tank :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( (bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB)) ){
		flDamage -= 60;//�������ܸߵĻ���
	}

		if(flDamage < 1){
			if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
			{
				pev->dmgtime = gpGlobals->time;

				if (RANDOM_LONG(0, 1))
					EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-1.wav", 1, ATTN_NORM);
				else
					EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/ric_metal-2.wav", 1, ATTN_NORM);

				UTIL_Sparks(ptr->vecEndPos);
			}

			return;
		}
		else{
		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 0, 5, 500, 20 );//chispas
		UTIL_Sparks( ptr->vecEndPos );

		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
		}
}

// take damage. bitsDamageType indicates type of damage sustained, ie: DMG_BULLET

int CBaseTurret_Tank::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	if ( !pev->takedamage )
		return 0;

	if (m_hEnemy == NULL && !m_ion)
	{
		m_ion = 100;
	}

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2.0;
	}
	else if (bitsDamageType & DMG_ENERGYBLAST)
	{
		flDamage *= 1.5;
	}

	if( bitsDamageType & (DMG_BULLET|DMG_CLUB|DMG_ENERGYBEAM) ){
		if(flDamage <= 60){
		flDamage = 0;
		}
		else{
		flDamage -= 60;
		}
	}

			if (pevAttacker)
			{
				CBaseEntity *pEntity = GetClassPtr((CBaseEntity *)pevAttacker);
				if(pEntity == this){
					if(!m_ion){
					m_ion = 100;//Fa!? ը���Լ���!!!
					}
				}
			}
	pev->health -= flDamage;

	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;

		ClearBits (pev->flags, FL_MONSTER); // why are they set in the first place???

		SetUse(NULL);
		SetThink(&CBaseTurret_Tank::TurretDeath);
		SUB_UseTargets( this, USE_ON, 0 ); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;

		return 0;
	}

	return 1;
}

int CBaseTurret_Tank::MoveTurret(void)
{
	int state = 0;
	// any x movement?
	
	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1 : -1 ;

		m_vecCurAngles.x += 0.05 * m_fTurnRate * flDir;

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		} 
		else
		{
			if (m_vecCurAngles.x < m_vecGoalAngles.x)
				m_vecCurAngles.x = m_vecGoalAngles.x;
		}

		SetBoneController(1, -m_vecCurAngles.x);

		state = 1;
	}

	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1 : -1 ;
		float flDist = fabs(m_vecGoalAngles.y - m_vecCurAngles.y);
		
		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}

		if (m_fTurnRate > 30)
		{
			m_fTurnRate -= m_iBaseTurnRate;
		}
		else
		{
			m_fTurnRate += m_iBaseTurnRate;
		}

		m_vecCurAngles.y += 0.1 * m_fTurnRate * flDir;

		if (m_vecCurAngles.y < 0)
			m_vecCurAngles.y += 360;
		else if (m_vecCurAngles.y >= 360)
			m_vecCurAngles.y -= 360;

		if (flDist < (0.05 * m_iBaseTurnRate))
			m_vecCurAngles.y = m_vecGoalAngles.y;


		SetBoneController(0, m_vecCurAngles.y - pev->angles.y );

		state = 1;
	}

	if (!state)
		m_fTurnRate = m_iBaseTurnRate;

	//ALERT(at_console, "(%.2f, %.2f)->(%.2f, %.2f)\n", m_vecCurAngles.x, 
	//	m_vecCurAngles.y, m_vecGoalAngles.x, m_vecGoalAngles.y);
	return state;
}

//
// ID as a machine
//
int	CBaseTurret_Tank::Classify ( void )
{
	return	CLASS_MACHINE;
}
