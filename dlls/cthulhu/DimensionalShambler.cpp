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
// SHAMBLER
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"effects.h"

const float PI = 3.14159;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	SHAMBLER_AE_ATTACK_RIGHT		0x01
#define	SHAMBLER_AE_ATTACK_LEFT		0x02
#define	SHAMBLER_AE_ATTACK_BOTH		0x03

#define SHAMBLER_FLINCH_DELAY			2		// at most one flinch every n secs


#include "DimensionalShambler.h"


LINK_ENTITY_TO_CLASS( monster_dimensionalshambler, CDimensionalShambler );

const char *CDimensionalShambler::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CDimensionalShambler::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CDimensionalShambler::pAttackSounds[] = 
{
	"shambler/ds_attack1.wav",
	"shambler/ds_attack2.wav",
};

const char *CDimensionalShambler::pIdleSounds[] = 
{
	"shambler/ds_idle1.wav",
	"shambler/ds_idle2.wav",
};

const char *CDimensionalShambler::pAlertSounds[] = 
{
	"shambler/ds_alert1.wav",
	"shambler/ds_alert2.wav",
};

const char *CDimensionalShambler::pPainSounds[] = 
{
	"shambler/ds_pain1.wav",
	"shambler/ds_pain2.wav",
	"shambler/ds_pain3.wav",
	"shambler/ds_pain4.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDimensionalShambler :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDimensionalShambler :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

int CDimensionalShambler :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if ( bitsDamageType & DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CDimensionalShambler :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CDimensionalShambler :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CDimensionalShambler :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CDimensionalShambler :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CDimensionalShambler :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SHAMBLER_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.shamblerDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
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

		case SHAMBLER_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.shamblerDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case SHAMBLER_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.shamblerDmgBothSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
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
void CDimensionalShambler :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/shambler.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= gSkillData.shamblerHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	m_flNextTeleport	= gpGlobals->time + 5.0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDimensionalShambler :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/shambler.mdl");

	PRECACHE_SOUND("debris/beamstart6.wav");

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

	// sprites
	PRECACHE_MODEL("sprites/c-tele1.spr");
	PRECACHE_MODEL("sprites/d-tele1.spr");
	// for the warpball
	PRECACHE_MODEL( "sprites/Fexplo1.spr" );
	PRECACHE_MODEL( "sprites/XFlare1.spr" );
	PRECACHE_SOUND( "debris/beamstart2.wav" );
	PRECACHE_SOUND( "debris/beamstart7.wav" );
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================



int CDimensionalShambler::IgnoreConditions ( void )
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
			m_flNextFlinch = gpGlobals->time + SHAMBLER_FLINCH_DELAY;
	}

	return iIgnore;
	
}

//=========================================================
// Look
//=========================================================

void CDimensionalShambler :: Look ( int iDistance )
{
	CBaseMonster::Look ( iDistance );

	float yaw, y;
	BOOL bDown, bUp;
	Vector vOffset;

	// if we are within teleport time
	if (m_flNextTeleport < gpGlobals->time)
	{
		// if we can see the enemy
		if (HasConditions(bits_COND_SEE_ENEMY))
		{
			CBaseEntity* pEnemy = m_hEnemy;

			// and we can find a place to teleport to
			yaw = RANDOM_FLOAT(-PI,PI);

			Vector vecTele;

			// Store original origin
			Vector myOrig = pev->origin;

			for (y =-PI; y < PI; y += PI/6)
			{
				// if vecGoal+yaw*36 (close enough) is ok.
				vOffset = Vector(36*cos(y+yaw),36*sin(y+yaw),pev->mins.z - pEnemy->pev->mins.z);

				// check that we are over some solid floor - i.e. if we move down 8 bits, we are in a brush!
				// now check we are not stuck in a brush anyway
				pev->origin = m_hEnemy->pev->origin + vOffset;

				pev->origin.z += 1;
				DROP_TO_FLOOR ( ENT(pev) );

				bDown = (fabs((pev->origin.z + pev->mins.z) - (pEnemy->pev->origin.z + pEnemy->pev->mins.z)) < 16.0);
				bUp = WALK_MOVE ( ENT(pev), 0,1, WALKMOVE_NORMAL );

				// this (hopefully) ensures that the dimensional shambler will not teleport to empty air (e.g. if the player is on a ledge)
				// and will account for slight slopes or steps.
				//if ( bDown && WALK_MOVE ( ENT(pev), 0,1, WALKMOVE_NORMAL ) )
				if ( bUp && bDown )
				{
					// yes, we can teleport there
					CEnvWarpBall* pWarpOut = (CEnvWarpBall*)CBaseEntity::Create("env_warpball",myOrig+Vector(0,0,36),Vector(0,0,0),edict());
					pWarpOut->pev->frags = 8;	// num of beams
					pWarpOut->pev->health = 128;	// max length of beam
					pWarpOut->Use(NULL,NULL,USE_TOGGLE,0);
					// make a flash here
					//CSprite *pHereSprite = CSprite::SpriteCreate( "sprites/d-tele1.spr", myOrig+Vector(0,0,36), TRUE );
					//pHereSprite->pev->scale = 1.0;
					//pHereSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
					//pHereSprite->AnimateAndDie( 20 );
					CEnvWarpBall* pWarpIn = (CEnvWarpBall*)CBaseEntity::Create("env_warpball",pev->origin+Vector(0,0,36),Vector(0,0,0),edict());
					pWarpIn->pev->frags = 8;	// num of beams
					pWarpIn->pev->health = 128;	// max length of beam
					pWarpIn->Use(NULL,NULL,USE_TOGGLE,0);
					// make a flash there
					//CSprite *pThereSprite = CSprite::SpriteCreate( "sprites/c-tele1.spr", pev->origin+Vector(0,0,36), TRUE );
					//pThereSprite->pev->scale = 1.0;
					//pThereSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
					//pThereSprite->AnimateAndDie( 20 );
					// now the sound
					//EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "debris/beamstart6.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					// set ideal facing
					MakeIdealYaw(m_hEnemy->pev->origin);
					pev->angles.y = pev->ideal_yaw;
					// clear the route
					RouteClear();
					// set the next teleport time (10 secs)
					m_flNextTeleport = gpGlobals->time + RANDOM_LONG(8.0,12.0);
					
					return;
				}
			}

			// we failed to find a teleport spot
			// restore the old origin
			pev->origin = myOrig;
		}
	}
}

