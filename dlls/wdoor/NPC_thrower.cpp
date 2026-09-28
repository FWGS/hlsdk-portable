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
//=========================================================
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"game.h"
#include	"player.h"
#include	"soundent.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CThrower : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void PainSound( void );
	void DeathSound( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	BOOL FCanCheckAttacks ( void );

	static const char *pDeathSounds[];
	static const char *pPainSounds[];
};

LINK_ENTITY_TO_CLASS( monster_thrower, CThrower );

const char *CThrower::pDeathSounds[] = 
{
	"gonome/gonome_death2.wav",
	"gonome/gonome_death3.wav",
};

const char *CThrower::pPainSounds[] = 
{
	"gonome/gonome_pain1.wav",
	"gonome/gonome_pain2.wav",
	"gonome/gonome_pain3.wav",
};

int CThrower :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Don't take any acid damage -- BigMomma's mortar is acid
	if ( bitsDamageType & DMG_ACID ){
	return 0;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CThrower :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CThrower :: SetYawSpeed ( void )
{
	pev->yaw_speed = 90;
}

BOOL CThrower :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


Vector VecCheckSplatToss2( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float maxHeight )
{
	TraceResult		tr;
	Vector			vecMidPoint;// halfway point between Spot1 and Spot2
	Vector			vecApex;// highest point 
	Vector			vecScale;
	Vector			vecGrenadeVel;
	Vector			vecTemp;
	float			flGravity = g_psv_gravity->value;

	// calculate the midpoint and apex of the 'triangle'
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0,0,maxHeight), ignore_monsters, ENT(pev), &tr);
	vecApex = tr.vecEndPos;

	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, ENT(pev), &tr);
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}

	// Don't worry about actually hitting the target, this won't hurt us!

	// How high should the grenade travel (subtract 15 so the grenade doesn't hit the ceiling)?
	float height = (vecApex.z - vecSpot1.z) - 15;
	// How fast does the grenade need to travel to reach that height given gravity?
	float speed = sqrt( 2 * flGravity * height );
	
	// How much time does it take to get there?
	float time = speed / flGravity;
	vecGrenadeVel = (vecSpot2 - vecSpot1);
	vecGrenadeVel.z = 0;
	float distance = vecGrenadeVel.Length();
	
	// Travel half the distance to the target in that time (apex is at the midpoint)
	vecGrenadeVel = vecGrenadeVel * ( 0.5 / time );
	// Speed to offset gravity at the desired height
	vecGrenadeVel.z = speed;

	return vecGrenadeVel;
}

void CThrower :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 1){
	flDamage *= 0.5;//头部超硬化，伤害只有1.5倍
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

BOOL CThrower :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 2000 )
	{
		CBaseEntity *pEnemy = m_hEnemy;

		if ( pEnemy )
		{
			Vector startPos = pev->origin;
			startPos.z += 96;
			pev->movedir = VecCheckSplatToss2( pev, startPos, pEnemy->BodyTarget_o( pev->origin ), 400);
			return TRUE;
		}
	}
	return FALSE;
}

void CThrower :: PainSound( void )
{
	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 0.75;

	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CThrower :: DeathSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pDeathSounds[ RANDOM_LONG(0,ARRAYSIZE(pDeathSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CThrower :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	
		case 1://炮弹发射
		{
			Vector	vecSpitOffset;
			vecSpitOffset = pev->origin + Vector(0,0,96);

			CBaseEntity *pMortar = CBaseEntity::Create( "bmortar", vecSpitOffset, pev->angles, edict() );
			pMortar->pev->velocity = pev->movedir;
		}
		break;


		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CThrower :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/thrower.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 128 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 720;
	}
	else{
	pev->health			= 600;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	pev->body			= 0;
	pev->gravity		= 1.5;

	m_ignoredamage		= 1;

	MonsterInit();
	m_killed_exp = 100;
	m_rpgms_level = 60;
	pev->netname = MAKE_STRING( "Thrower" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CThrower :: Precache()
{
	int i;

	PRECACHE_MODEL("models/thrower.mdl");

	UTIL_PrecacheOther( "bmortar" );

	PRECACHE_MODEL("sprites/mommaspit.spr");// spit projectile.
	PRECACHE_MODEL("sprites/mommaspout.spr");// client side spittle.
	PRECACHE_MODEL("sprites/mommablob.spr" );

	PRECACHE_SOUND( "weapons/cluster_explode.wav" );
	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );

	for ( i = 0; i < ARRAYSIZE( pDeathSounds ); i++ )
		PRECACHE_SOUND((char *)pDeathSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	
