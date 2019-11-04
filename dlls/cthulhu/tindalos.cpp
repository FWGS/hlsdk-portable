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
// Tindalos
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"effects.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	TINDALOS_AE_ATTACK			0x01

#define TINDALOS_FLINCH_DELAY		4		// at most one flinch every n secs

#define TINDALOS_SPRITE_NAME		"sprites/ballsmoke.spr"


#include "tindalos.h"


LINK_ENTITY_TO_CLASS( monster_tindalos, CTindalos );

TYPEDESCRIPTION	CTindalos::m_SaveData[] = 
{
	DEFINE_ARRAY( CTindalos, m_pSmoke, FIELD_CLASSPTR, 4 ),
};

IMPLEMENT_SAVERESTORE( CTindalos, CBaseMonster );

const char *CTindalos::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CTindalos::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CTindalos::pAttackSounds[] = 
{
	"ghoul/gh_attack1.wav",
//	"ghoul/gh_attack2.wav",
};

const char *CTindalos::pIdleSounds[] = 
{
	"ghoul/gh_idle1.wav",
	"ghoul/gh_idle2.wav",
	"ghoul/gh_idle3.wav",
//	"ghoul/gh_idle4.wav",
};

const char *CTindalos::pAlertSounds[] = 
{
	"ghoul/gh_alert1.wav",
//	"ghoul/gh_alert20.wav",
//	"ghoul/gh_alert30.wav",
};

const char *CTindalos::pPainSounds[] = 
{
	"ghoul/gh_pain1.wav",
//	"ghoul/gh_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CTindalos :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CTindalos :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CTindalos :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

void CTindalos::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	if (bitsDamageType & DMG_BULLET)	return;
	if (bitsDamageType & DMG_CRUSH)		return;
	if (bitsDamageType & DMG_SLASH)		return;
	if (bitsDamageType & DMG_FALL)		return;
	if (bitsDamageType & DMG_CLUB)		return;

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CTindalos :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (bitsDamageType & DMG_BULLET)	return 0;
	if (bitsDamageType & DMG_CRUSH)		return 0;
	if (bitsDamageType & DMG_SLASH)		return 0;
	if (bitsDamageType & DMG_FALL)		return 0;
	if (bitsDamageType & DMG_CLUB)		return 0;

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CTindalos :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CTindalos :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CTindalos :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 0.2, ATTN_NORM, 0, pitch );
}

void CTindalos :: AttackSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 0.5, ATTN_NORM, 0, pitch );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTindalos :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case TINDALOS_AE_ATTACK:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.bullsquidDmgBite, DMG_POISON );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
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

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// Spawn
//=========================================================
void CTindalos :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/tindalos.mdl");
	UTIL_SetSize( pev, Vector( -24, -24, 0 ), Vector( 24, 24, 36 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	if (pev->health == 0)
		pev->health		= gSkillData.houndeyeHealth * 2.5;	// use this one
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= 0;

	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 48;

	SetBits(pev->spawnflags, SF_MONSTER_FADECORPSE);

	MonsterInit();

	Vector vecPosition;
	Vector vecJunk;

	for (int i = 0; i < 4; i++)
	{
		GetAttachment( i, vecPosition, vecJunk );

		m_pSmoke[i] = CSprite::SpriteCreate( TINDALOS_SPRITE_NAME, vecPosition, TRUE );
		m_pSmoke[i]->SetTransparency( kRenderTransAdd, 255, 255, 255, 24, kRenderFxNone );
		m_pSmoke[i]->SetAttachment( edict(), i+1 );
		m_pSmoke[i]->pev->scale = 1.0;
		m_pSmoke[i]->pev->frame = RANDOM_FLOAT(0,10);
		m_pSmoke[i]->pev->framerate = RANDOM_FLOAT(5,15);
		m_pSmoke[i]->TurnOn();
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTindalos :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/tindalos.mdl");

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
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CTindalos::IgnoreConditions ( void )
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
			m_flNextFlinch = gpGlobals->time + TINDALOS_FLINCH_DELAY;
	}

	return iIgnore;
	
}

void CTindalos :: Killed( entvars_t *pevAttacker, int iGib )
{
	for (int i = 0; i < 4; i++)
	{
		UTIL_Remove( m_pSmoke[i] );
		m_pSmoke[i] = NULL;
	}

	CBaseMonster::Killed(pevAttacker, iGib);
}

