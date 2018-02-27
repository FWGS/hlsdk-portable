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
// Bad David Monster
//=========================================================

#include	"zombie.h"
#include	"shake.h"
#include	"effects.h"
#include	"player.h"

#define BADDAVID_AE_ATTACK			1
#define BADDAVID_AE_FLINCH			1013

#define BADDAVID_FLINCH_DELAY			15.0
#define BADDAVID_HEALTH				1800

#define SF_NOELECTROCUTE			( 1 << 5 )

class CDavidMonster : public CZombie
{
public:
	void Spawn( void );
	void Precache( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void MonsterThink();
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	int IgnoreConditions( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL StartFlame;
	BOOL HasTurnedOffFlameSound;	
	float m_flNextFlinchForget;
	float NextFlame;
	float NextFlameBurn;
	float TurnOffFlames;

	void PainSound( void );
	void AlertSound( void );
	void AttackSound( void );
	void ThunderAttackSound();
	void DavidHurtSound();

	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist );

	static const char *pAttackSounds[];
	static const char *pFireSounds[];
	static const char *pAxeHitSounds[];
	static const char *pAxeGrabSounds[];
	static const char *pThunderAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pHurtSounds[];
};

LINK_ENTITY_TO_CLASS( monster_david, CDavidMonster );

TYPEDESCRIPTION CDavidMonster::m_SaveData[] =
{
	DEFINE_FIELD( CDavidMonster, StartFlame, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDavidMonster, HasTurnedOffFlameSound, FIELD_BOOLEAN ),
	DEFINE_FIELD( CDavidMonster, m_flNextFlinchForget, FIELD_TIME ),
	DEFINE_FIELD( CDavidMonster, NextFlame, FIELD_TIME ),
	DEFINE_FIELD( CDavidMonster, NextFlameBurn, FIELD_TIME ),
	DEFINE_FIELD( CDavidMonster, TurnOffFlames, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CDavidMonster, CZombie )

const char *CDavidMonster::pAttackSounds[] = 
{
	"davidbad/david_attack.wav"
};

const char *CDavidMonster::pFireSounds[] =
{
	"davidbad/fire_ignite.wav",
	"davidbad/fire_loop.wav",
	"davidbad/fire_off.wav"
};

const char *CDavidMonster::pAxeHitSounds[] =
{
	"davidbad/axe_hit.wav",
	"davidbad/axe_hitbody.wav",
	"davidbad/axe_swing.wav"
};

const char *CDavidMonster::pAxeGrabSounds[] =
{
	"davidbad/david_axegrab.wav"
};      

const char *CDavidMonster::pThunderAttackSounds[] =
{
	"davidbad/thunder_attack1.wav",
	"davidbad/thunder_attack2.wav",
	"davidbad/thunder_attack3.wav"
};

const char *CDavidMonster::pAlertSounds[] = 
{
	"davidbad/db_alert10.wav",
	"davidbad/db_alert20.wav",
	"davidbad/db_alert30.wav"
};

const char *CDavidMonster::pPainSounds[] = 
{
	"davidbad/db_pain1.wav",
	"davidbad/db_pain2.wav"
};

const char *CDavidMonster::pHurtSounds[] =
{
	"davidbad/david_hurt.wav",
	"davidbad/david_hurt2.wav",
	"davidbad/david_hurt3.wav"
};

int CDavidMonster :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if( bitsDamageType == DMG_SPEAR )
	{
		if( m_flNextFlinchForget < gpGlobals->time)
		{			
			if( RANDOM_LONG( 0, 1 ) )
				ClearBits( m_afMemory, bits_MEMORY_FLINCHED );
			m_flNextFlinchForget = gpGlobals->time + BADDAVID_FLINCH_DELAY;
		}

		// HACK HACK -- until we fix this.
		if( IsAlive() )
			PainSound();
		return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	}

	if( pevAttacker->flags & FL_CLIENT )
	{
		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
			if( pPlayer )
			{
				UTIL_ScreenFade( pPlayer, Vector( 255, 0, 0 ), 0.5, 0.0, 100, FFADE_IN );
				pPlayer->TakeDamage( pev, pev, flDamage / 4, bitsDamageType );
			}
		}
	}
	return 0;
}

void CDavidMonster :: PainSound( void )
{
	int pitch = PITCH_LOW + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CDavidMonster :: AlertSound( void )
{
	int pitch = PITCH_LOW + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CDavidMonster :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackSounds[0], 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5) );
}

void CDavidMonster :: ThunderAttackSound( void )
{
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
                if( pPlayer )
			// Play a random thunder attack sound
			EMIT_SOUND_DYN ( ENT(pPlayer->pev), CHAN_AUTO, pThunderAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pThunderAttackSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5) );
	}
	CBaseEntity::Create( "lightning_effect_boss", g_vecZero, g_vecZero, NULL );
}

void CDavidMonster :: DavidHurtSound( void )
{
	// Play a random hurt sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pHurtSounds[ RANDOM_LONG(0,ARRAYSIZE(pHurtSounds)-1) ], 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5) );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDavidMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case BADDAVID_AE_ATTACK:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
					if(pev->body)
					{
						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "davidbad/axe_hitbody.wav", 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5) );
					}
					else
					{
						DavidHurtSound();
						UTIL_ScreenFade( pHurt, Vector( 255, 0, 0 ), 0.5, 0.0, 100, FFADE_IN );
					}
				}
				else if( pev->body ) // Play attack hit sound
					EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "davidbad/axe_hit.wav", 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5) );
			}
			else if( pev->body ) // Play attack miss sound
				EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "davidbad/axe_swing.wav", 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
			break;
		case BADDAVID_AE_FLINCH:
			pev->body = pev->body ? 0 : 1;
			// EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "davidbad/david_axegrab.wav", 1.0, ATTN_NORM, 0, PITCH_NORM + RANDOM_LONG(-5,5) );
			break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 
//=========================================================
BOOL CDavidMonster :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if(!pev->body)
		return CBaseMonster::CheckMeleeAttack1( flDot, flDist );

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
BOOL CDavidMonster :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	return CBaseMonster::CheckMeleeAttack2( flDot, flDist );
}

//=========================================================
// Spawn
//=========================================================
void CDavidMonster :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/david_monster.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health			= BADDAVID_HEALTH;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDavidMonster :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL(STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/david_monster.mdl");

	PRECACHE_MODEL("sprites/xffloor.spr");

	for( i = 0; i < ARRAYSIZE( pAxeHitSounds ); i++ )
		PRECACHE_SOUND(pAxeHitSounds[i]);

	for( i = 0; i < ARRAYSIZE( pFireSounds ); i++ )
		PRECACHE_SOUND(pFireSounds[i]);

	for( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND(pAlertSounds[i]);
 
	for( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND(pPainSounds[i]);

	for( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
 		PRECACHE_SOUND(pAttackSounds[i]);

	for( i = 0; i < ARRAYSIZE( pAxeGrabSounds ); i++ )
		PRECACHE_SOUND(pAxeGrabSounds[i]);

	for( i = 0; i < ARRAYSIZE( pThunderAttackSounds ); i++ )
		PRECACHE_SOUND(pThunderAttackSounds[i]);

	for( i = 0; i < ARRAYSIZE( pHurtSounds ); i++ )
		PRECACHE_SOUND(pHurtSounds[i]);
}

void CDavidMonster::MonsterThink()
{
	if( !( pev->spawnflags & SF_NOELECTROCUTE ) && m_hEnemy != 0
		&& m_flNextAttack < gpGlobals->time && pev->health )
	{
		// thunder attack
		float flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();
		if( flDist > 128.0f )
		{
			ThunderAttackSound();
			m_flNextAttack = gpGlobals->time + 9.0f;
		}
	}
	if( pev->health <= 300 && !StartFlame)
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "davidbad/fire_ignite.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		EMIT_SOUND_DYN( ENT(pev), CHAN_AUTO, "davidbad/fire_loop.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		NextFlame = gpGlobals->time + 0.5f;
		StartFlame = TRUE;		
	}
	if( TurnOffFlames >= 15.0f && !HasTurnedOffFlameSound )
	{
		STOP_SOUND(ENT(pev), CHAN_AUTO, "davidbad/fire_loop.wav");
		EMIT_SOUND_DYN( ENT(pev), CHAN_ITEM, "davidbad/fire_off.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
		HasTurnedOffFlameSound = TRUE;
	}
	if( pev->health <= 300 && NextFlame < gpGlobals->time && TurnOffFlames < 15.0f )
	{
		CSprite *m_Fire = CSprite::SpriteCreate( "sprites/xffloor.spr", pev->origin + Vector( 0, 0, 72 ), TRUE );
		m_Fire->AnimateAndDie( 20.0f );
		m_Fire->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxReflection );
		
		if( NextFlameBurn < gpGlobals->time )
		{
			for( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBaseEntity *pPlayer = UTIL_PlayerByIndex( i );
				if( pPlayer && !FBitSet( pPlayer->pev->flags, FL_INWATER ) )
				{
					pPlayer->TakeDamage(pev, pev, DAMAGE_YES, DMG_BURN );
				}
			}
			NextFlameBurn = gpGlobals->time + 1.0f;
		}
		NextFlame = gpGlobals->time + 0.5f;
		TurnOffFlames = TurnOffFlames + 0.5f;
	}

	CBaseMonster::MonsterThink();
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
int CDavidMonster::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
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
			m_flNextFlinch = gpGlobals->time + BADDAVID_FLINCH_DELAY;
	}

	return iIgnore;
}

class CLightningEffect : public CBaseEntity
{
public:
	void Spawn();
	void EXPORT ElectricityAttack2();
};

LINK_ENTITY_TO_CLASS( lightning_effect_boss, CLightningEffect )

void CLightningEffect::Spawn()
{
	SetThink( &CLightningEffect::ElectricityAttack2 );
	pev->nextthink = gpGlobals->time + 1.0;
}

void CLightningEffect::ElectricityAttack2()
{
	CBasePlayer *pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( 0, "player" );
	if( pPlayer )
	{
		pPlayer->ThunderAttack();
		if( pPlayer->pev->flags & FL_ONGROUND )
		{
			pPlayer->TakeDamage( pev, pev, 10, DMG_SHOCK );
		}
	}
	REMOVE_ENTITY(ENT(pev));
}
