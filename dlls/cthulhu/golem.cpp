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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

#define GOLEM_DAMAGE			(DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST|DMG_SONIC|DMG_ACID|DMG_FALL)

#define GOLEM_SHOCK_RADIUS		512

// todo: 
// clean up other animations
// gibs


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	GOLEM_AE_ATTACK_SMASH		0x01
#define	GOLEM_AE_ATTACK_UPPERCUT	0x02
#define	GOLEM_AE_LEFT_FOOT			0x03
#define	GOLEM_AE_RIGHT_FOOT			0x04
#define	GOLEM_AE_ATTACK_SHOCK		0x05

#define GOLEM_FLINCH_DELAY			4		// at most one flinch every n secs

class CGolem : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );

	void Shockwave ( void );

	float m_flNextFlinch;
	int m_iSpriteTexture;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];
	static const char *pFootSounds[];

	// No range attacks
	virtual BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; };
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int HasCustomGibs( void ) { return m_iszGibModel; }
	
	int m_iszGibModel;

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( monster_golem, CGolem );

TYPEDESCRIPTION	CGolem::m_SaveData[] = 
{
	DEFINE_FIELD( CGolem, m_iSpriteTexture, FIELD_INTEGER ),
	DEFINE_FIELD( CGolem, m_iszGibModel, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CGolem, CBaseMonster );

const char *CGolem::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CGolem::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CGolem::pAttackSounds[] = 
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CGolem::pIdleSounds[] = 
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CGolem::pAlertSounds[] = 
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CGolem::pPainSounds[] = 
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

const char *CGolem::pFootSounds[] = 
{
	"garg/gar_step1.wav",
	"garg/gar_step2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGolem :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGolem :: SetYawSpeed ( void )
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

BOOL CGolem :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	//int iGround = FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND );

	// Cthulhu: but this stops the monster from hitting flying monsters that are
	// melee attacking it (e.g. nightgaunt)
	// Solution: explicitly check for monster types that we cannot hit

	BOOL bHit = TRUE;	// we can hit by default
	if (m_hEnemy)
	{
		if (FClassnameIs( m_hEnemy->pev, "hornet"))			bHit = FALSE;
		if (FClassnameIs( m_hEnemy->pev, "monster_snark"))	bHit = FALSE;
	}

	//if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL && iGround )
	if ( flDist <= 96 && flDot >= 0.7 && m_hEnemy != NULL && bHit )
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CGolem :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= GOLEM_SHOCK_RADIUS && flDot >= 0.5 )
	{
		if (gpGlobals->time >= m_flNextAttack)
		{
			return TRUE;
		}
	}

	// we always return false, otherwise it will try and do an animation that does not exist
	return FALSE;
}

int CGolem :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 300% damage from SONIC
	if ( bitsDamageType & DMG_SONIC )
	{
		flDamage *= 3.0;
	}

	if (!(bitsDamageType & GOLEM_DAMAGE))
	{
		flDamage = 0.0;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CGolem :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 3)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CGolem :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CGolem :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CGolem :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGolem :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case GOLEM_AE_RIGHT_FOOT:
		case GOLEM_AE_LEFT_FOOT:
			UTIL_ScreenShake( pev->origin, 2.0, 3.0, 1.0, 500 );
			EMIT_SOUND_DYN ( edict(), CHAN_BODY, pFootSounds[ RANDOM_LONG(0,ARRAYSIZE(pFootSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
			break;

		case GOLEM_AE_ATTACK_UPPERCUT:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 96, gSkillData.zombieDmgOneSlash * 2, DMG_CLUB );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -20;
					pHurt->pev->punchangle.x = 20;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 200;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 500;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case GOLEM_AE_ATTACK_SMASH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 96, gSkillData.zombieDmgOneSlash * 3, DMG_CLUB );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -20;
					pHurt->pev->punchangle.x = 20;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 500;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 200;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case GOLEM_AE_ATTACK_SHOCK:
		{
			// we do the shockwave here
			Shockwave();

			m_flNextAttack = gpGlobals->time + 20.0;
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
void CGolem :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/golem.mdl");
	UTIL_SetSize( pev, Vector( -24, -24, 0 ), Vector( 24, 24, 144 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health		= gSkillData.zombieHealth * 3;
	pev->view_ofs		= Vector( 0, 0, 48 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_bloodColor = DONT_BLEED;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGolem :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/golem.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pFootSounds ); i++ )
		PRECACHE_SOUND((char *)pFootSounds[i]);

	PRECACHE_SOUND("houndeye/he_blast1.wav");
	PRECACHE_SOUND("houndeye/he_blast2.wav");
	PRECACHE_SOUND("houndeye/he_blast3.wav");

	m_iszGibModel = ALLOC_STRING("models/golem_gibs.mdl");
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
	PRECACHE_MODEL("models/golem_gibs.mdl");
}	

void CGolem::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	ALERT( at_aiconsole, "CGolem::TraceAttack\n");

	if ( !IsAlive() )
	{
		CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
		return;
	}

	bitsDamageType &= GOLEM_DAMAGE;

	if ( bitsDamageType == 0)
	{
		if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20) )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT(0.5,1.5) );
			pev->dmgtime = gpGlobals->time;
//			if ( RANDOM_LONG(0,100) < 25 )
//				EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, pRicSounds[ RANDOM_LONG(0,ARRAYSIZE(pRicSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM );
		}
		flDamage = 0;
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );

}

void CGolem::Shockwave()
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
		WRITE_COORD( pev->origin.z + 16 + GOLEM_SHOCK_RADIUS / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 16 ); // life
		WRITE_BYTE( 96 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 64  );	// RED
		WRITE_BYTE( 64 );	// GREEN
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
		WRITE_COORD( pev->origin.z + 16 + (GOLEM_SHOCK_RADIUS / 2 ) / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iSpriteTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 12 ); // life
		WRITE_BYTE( 96 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 64  );	// RED
		WRITE_BYTE( 64 );	// GREEN
		WRITE_BYTE( 64  );	// BLUE
		
		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();


	CBaseEntity *pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, GOLEM_SHOCK_RADIUS )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			if ( !FClassnameIs(pEntity->pev, "monster_golem") )
			{

				// golems do FULL damage if the ent in question is visible. Half damage otherwise.
				// This means that you must get out of the kingpin's attack range entirely to avoid damage.
				// Calculate full damage first

				flAdjustedDamage = gSkillData.zombieDmgBothSlash * 2;

				flDist = (pEntity->Center() - pev->origin).Length();

				flAdjustedDamage *= (1.0 - ( flDist / GOLEM_SHOCK_RADIUS ));

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

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CGolem::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + GOLEM_FLINCH_DELAY;
	}

	return iIgnore;
	
}