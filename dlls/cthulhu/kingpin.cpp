/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// Kingpin
//=========================================================

#define KINGPIN_MAX_ATTACK_RADIUS  1024

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CKingpin : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	int IgnoreConditions ( void );

	float m_flNextFlinch;

	void Shockwave ( void );

	void PainSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	int m_iSpriteTexture;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( monster_kingpin, CKingpin );

TYPEDESCRIPTION	CKingpin::m_SaveData[] = 
{
	DEFINE_FIELD( CKingpin, m_iSpriteTexture, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CKingpin, CBaseMonster );

const char *CKingpin::pAttackSounds[] = 
{
	"x/x_attack1.wav",
	"x/x_attack2.wav",
	"x/x_attack3.wav",
};

const char *CKingpin::pIdleSounds[] = 
{
	"x/x_attack1.wav",
	"x/x_attack2.wav",
	"x/x_attack3.wav",
	"x/x_pain1.wav",
	"x/x_pain2.wav",
	"x/x_pain3.wav",
};

const char *CKingpin::pPainSounds[] = 
{
	"x/x_pain1.wav",
	"x/x_pain2.wav",
	"x/x_pain3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CKingpin :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CKingpin :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CKingpin :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( IsAlive() )
		PainSound();

	// kingpin does not take damage, but we want to use this logic, so pass a small number
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, 0.001, bitsDamageType );
}

void CKingpin :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 3)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CKingpin :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CKingpin :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// Spawn
//=========================================================
void CKingpin :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/kingpin.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= 1000;	// always 1000
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	// PhilG: hack to allow CheckRangeAttack1 to be used
	m_afCapability |= bits_CAP_RANGE_ATTACK1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CKingpin :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/kingpin.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	PRECACHE_SOUND("houndeye/he_blast1.wav");
	PRECACHE_SOUND("houndeye/he_blast2.wav");
	PRECACHE_SOUND("houndeye/he_blast3.wav");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CKingpin::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + 2.0;
	}

	return iIgnore;
	
}

BOOL CKingpin :: CheckRangeAttack1 ( float flDot, float flDist )
{
	//if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 1024 && flDot >= 0.5 )
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if (gpGlobals->time >= m_flNextAttack)
		{
			// we do the shockwave here
			Shockwave();

			m_flNextAttack = gpGlobals->time + 2.0;
		}
	}

	// we always return false, otherwise it will try and do an animation that does not exist
	return FALSE;
}

void CKingpin::Shockwave()
{
	float		flAdjustedDamage;
	float		flDist;

	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "houndeye/he_blast1.wav", 1, ATTN_NORM);	break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "houndeye/he_blast2.wav", 1, ATTN_NORM);	break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "houndeye/he_blast3.wav", 1, ATTN_NORM);	break;
	}

	// blast circles
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + KINGPIN_MAX_ATTACK_RADIUS / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 16 ); // life
		WRITE_BYTE( 96 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 64  );	// RED
		WRITE_BYTE( 255 );	// GREEN
		WRITE_BYTE( 64  );	// BLUE

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + (KINGPIN_MAX_ATTACK_RADIUS / 2 ) / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 12 ); // life
		WRITE_BYTE( 96 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 64  );	// RED
		WRITE_BYTE( 255 );	// GREEN
		WRITE_BYTE( 64  );	// BLUE
		
		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();


	CBaseEntity *pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, KINGPIN_MAX_ATTACK_RADIUS )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			if ( !FClassnameIs(pEntity->pev, "monster_kingpin") )
			{

				// kingpins do FULL damage if the ent in question is visible. Half damage otherwise.
				// This means that you must get out of the kingpin's attack range entirely to avoid damage.
				// Calculate full damage first

				flAdjustedDamage = 200.0;

				flDist = (pEntity->Center() - pev->origin).Length();

				flAdjustedDamage -= ( flDist / KINGPIN_MAX_ATTACK_RADIUS ) * flAdjustedDamage;

				if ( !FVisible( pEntity ) )
				{
					if ( pEntity->IsPlayer() )
					{
						// if this entity is a client, and is not in full view, inflict half damage. We do this so that players still 
						// take the residual damage if they don't totally leave the houndeye's effective radius. We restrict it to clients
						// so that monsters in other parts of the level don't take the damage and get pissed.
						flAdjustedDamage *= 0.5;
					}
					else if ( !FClassnameIs( pEntity->pev, "func_breakable" ) && !FClassnameIs( pEntity->pev, "func_pushable" ) ) 
					{
						// do not hurt nonclients through walls, but allow damage to be done to breakables
						flAdjustedDamage = 0;
					}
				}

				//ALERT ( at_aiconsole, "Damage: %f\n", flAdjustedDamage );

				if (flAdjustedDamage > 0 )
				{
					pEntity->TakeDamage ( pev, pev, flAdjustedDamage, DMG_SONIC | DMG_ALWAYSGIB );
				}
			}
		}
	}
}


