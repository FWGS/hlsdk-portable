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
// Hellhound
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"soundent.h"
#include	"scripted.h"
#include	"decals.h"


#include "hellhound.h"

int			   iHHFireballSprite;

/////////////////////////////////////////////////////////////////////////////////////////////

LINK_ENTITY_TO_CLASS( hhfireball, CHHFireball );

TYPEDESCRIPTION	CHHFireball::m_SaveData[] = 
{
	DEFINE_FIELD( CHHFireball, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHHFireball, CBaseEntity );

void CHHFireball:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "hhfireball" );
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	//pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;
	pev->framerate = 10.0;	// cthulhu

	SET_MODEL(ENT(pev), "sprites/rjet1.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}

void CHHFireball::Animate( void )
{
	SetNextThink( 0.1 );

	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

void CHHFireball::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CHHFireball *pFireball = GetClassPtr( (CHHFireball *)NULL );
	pFireball->Spawn();
	
	UTIL_SetOrigin( pFireball, vecStart );
	pFireball->pev->velocity = vecVelocity;
	pFireball->pev->owner = ENT(pevOwner);

	pFireball->SetThink ( Animate );
	pFireball->SetNextThink( 0.1 );
}

void CHHFireball :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}

	if ( !pOther->pev->takedamage )
	{

		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2));

		// make some flecks
		/*
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_SPRITE_SPRAY );
			WRITE_COORD( tr.vecEndPos.x);	// pos
			WRITE_COORD( tr.vecEndPos.y);	
			WRITE_COORD( tr.vecEndPos.z);	
			WRITE_COORD( tr.vecPlaneNormal.x);	// dir
			WRITE_COORD( tr.vecPlaneNormal.y);	
			WRITE_COORD( tr.vecPlaneNormal.z);	
			WRITE_SHORT( iHHFireballSprite );	// model
			WRITE_BYTE ( 1 );			// count
			WRITE_BYTE ( 30 );			// speed
			WRITE_BYTE ( 8 );			// noise ( client will divide by 100 )
		MESSAGE_END();
		*/
	}
	else
	{
		pOther->TakeDamage ( pev, pev, gSkillData.bullsquidDmgSpit * 2, DMG_BURN );
	}

	SetThink ( SUB_Remove );
	SetNextThink( 0 );
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	HELLHOUND_AE_ATTACK			0x01
#define	HELLHOUND_AE_FIREBALL		0x02

#define HELLHOUND_FLINCH_DELAY			8		// at most one flinch every n secs
#define HELLHOUND_FIREBALL_DELAY		5		// at most one fireball every n secs


LINK_ENTITY_TO_CLASS( monster_hellhound, CHellhound );

TYPEDESCRIPTION	CHellhound::m_SaveData[] = 
{
	DEFINE_FIELD( CHellhound, m_flNextFireballTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CHellhound, CBaseMonster );

const char *CHellhound::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CHellhound::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CHellhound::pAttackSounds[] = 
{
	"ghoul/gh_attack1.wav",
//	"ghoul/gh_attack2.wav",
};

const char *CHellhound::pIdleSounds[] = 
{
	"ghoul/gh_idle1.wav",
	"ghoul/gh_idle2.wav",
	"ghoul/gh_idle3.wav",
//	"ghoul/gh_idle4.wav",
};

const char *CHellhound::pAlertSounds[] = 
{
	"ghoul/gh_alert1.wav",
//	"ghoul/gh_alert20.wav",
//	"ghoul/gh_alert30.wav",
};

const char *CHellhound::pPainSounds[] = 
{
	"ghoul/gh_pain1.wav",
//	"ghoul/gh_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHellhound :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_PREDATOR;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CHellhound :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		// hound will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if ( flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextFireballTime )
	{
		if ( m_hEnemy != NULL )
		{
			if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 256 )
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if ( IsMoving() )
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextFireballTime = gpGlobals->time + (HELLHOUND_FIREBALL_DELAY*2);
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextFireballTime = gpGlobals->time + HELLHOUND_FIREBALL_DELAY;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CHellhound :: ISoundMask ( void )
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
void CHellhound :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

	pev->yaw_speed = ys;
}

int CHellhound :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if (bitsDamageType & DMG_BURN)
	{
		return 0;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CHellhound :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CHellhound :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CHellhound :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 0.2, ATTN_NORM, 0, pitch );
}

void CHellhound :: AttackSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 0.5, ATTN_NORM, 0, pitch );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHellhound :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case HELLHOUND_AE_ATTACK:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.bullsquidDmgBite * 1.5, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -150;
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

		case HELLHOUND_AE_FIREBALL:
		{
			// do stuff for this event.
			Vector	vecFireballOffset;
			Vector	vecFireballDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecFireballOffset = ( gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 23 );		
			vecFireballOffset = ( pev->origin + vecFireballOffset );
			if (m_pCine) // LRC- are we being told to do this by a scripted_action?
			{
				if (m_hTargetEnt != NULL && m_pCine->PreciseAttack())
					vecFireballDir = ( ( m_hTargetEnt->pev->origin ) - vecFireballOffset ).Normalize();
				else
					vecFireballDir = gpGlobals->v_forward;
			}
			else
				vecFireballDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecFireballOffset ).Normalize();

			vecFireballDir.x += RANDOM_FLOAT( -0.02, 0.02 );
			vecFireballDir.y += RANDOM_FLOAT( -0.02, 0.02 );
			vecFireballDir.z += RANDOM_FLOAT( -0.02, 0 );


			// do stuff for this event.
			AttackSound();

			// spew the spittle temporary ents.
			/*
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecFireballOffset );
				WRITE_BYTE( TE_SPRITE_SPRAY );
				WRITE_COORD( vecFireballOffset.x);	// pos
				WRITE_COORD( vecFireballOffset.y);	
				WRITE_COORD( vecFireballOffset.z);	
				WRITE_COORD( vecFireballDir.x);	// dir
				WRITE_COORD( vecFireballDir.y);	
				WRITE_COORD( vecFireballDir.z);	
				WRITE_SHORT( iHHFireballSprite );	// model
				WRITE_BYTE ( 1 );			// count
				WRITE_BYTE ( 210 );			// speed
				WRITE_BYTE ( 8 );			// noise ( client will divide by 100 )
			MESSAGE_END();
			*/

			CHHFireball::Shoot( pev, vecFireballOffset, vecFireballDir * 900 );
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
void CHellhound :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/hellhound.mdl");
	UTIL_SetSize( pev, Vector( -24, -24, 0 ), Vector( 24, 24, 36 ) );

	// Hellhounds are immune to fire
	SetBits( pev->flags, FL_IMMUNE_LAVA );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	if (pev->health == 0)
		pev->health		= gSkillData.houndeyeHealth * 4;	// use this one
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHellhound :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/hellhound.mdl");

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

	iHHFireballSprite = PRECACHE_MODEL("sprites/rjet1.spr");// client side spittle.
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CHellhound::IgnoreConditions ( void )
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
			m_flNextFlinch = gpGlobals->time + HELLHOUND_FLINCH_DELAY;
	}

	return iIgnore;
	
}

