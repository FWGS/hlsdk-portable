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
// Cthonian - big, spotty tentacle-mouthed meanie.
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"

#define		CTHONIAN_SPRINT_DIST	256 // how close the Cthonian has to get before starting to sprint and refusing to swerve

int			   iCthonianSpitSprite;
	

//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_CTHONIAN_HURTREAR = LAST_COMMON_SCHEDULE + 1,
	SCHED_CTHONIAN_SMELLFOOD,
	SCHED_CTHONIAN_EAT,
	SCHED_CTHONIAN_SNIFF_AND_EAT,
	SCHED_CTHONIAN_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum 
{
	TASK_CTHONIAN_REARTURN = LAST_COMMON_TASK + 1,
};

#include "Cthonian.h"

//=========================================================
// Cthonian's spit projectile
//=========================================================

LINK_ENTITY_TO_CLASS( cthonianspit, CCthonianSpit );

TYPEDESCRIPTION	CCthonianSpit::m_SaveData[] = 
{
	DEFINE_FIELD( CCthonianSpit, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CCthonianSpit, CBaseEntity );

void CCthonianSpit:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "cthonianspit" );
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/bigspit.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
}

void CCthonianSpit::Animate( void )
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

void CCthonianSpit::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CCthonianSpit *pSpit = GetClassPtr( (CCthonianSpit *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink ( Animate );
	pSpit->SetNextThink( 0.1 );
}

void CCthonianSpit :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	// use bullsquid sounds
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
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0,1));

		// make some flecks
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_SPRITE_SPRAY );
			WRITE_COORD( tr.vecEndPos.x);	// pos
			WRITE_COORD( tr.vecEndPos.y);	
			WRITE_COORD( tr.vecEndPos.z);	
			WRITE_COORD( tr.vecPlaneNormal.x);	// dir
			WRITE_COORD( tr.vecPlaneNormal.y);	
			WRITE_COORD( tr.vecPlaneNormal.z);	
			WRITE_SHORT( iCthonianSpitSprite );	// model
			WRITE_BYTE ( 5 );			// count
			WRITE_BYTE ( 30 );			// speed
			WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
		MESSAGE_END();
	}
	else
	{
		pOther->TakeDamage ( pev, pev, gSkillData.cthonianDmgSpit, DMG_ACID );
	}

	SetThink ( SUB_Remove );
	SetNextThink(0);
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		CTHONIAN_AE_SPIT		( 1 )
#define		CTHONIAN_AE_BITE		( 2 )
#define		CTHONIAN_AE_BLINK		( 3 )
#define		CTHONIAN_AE_TAILWHIP	( 4 )
#define		CTHONIAN_AE_REAR		( 5 )
#define		CTHONIAN_AE_THROW		( 6 )

LINK_ENTITY_TO_CLASS( monster_cthonian, CCthonian );

TYPEDESCRIPTION	CCthonian::m_SaveData[] = 
{
	DEFINE_FIELD( CCthonian, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CCthonian, m_flNextSpitTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CCthonian, CBaseMonster );

//=========================================================
// IgnoreConditions 
//=========================================================
int CCthonian::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ( gpGlobals->time - m_flLastHurtTime <= 20 )
	{
		// haven't been hurt in 20 seconds, so let the Cthonian care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	return iIgnore;
}

//=========================================================
// IRelationship - overridden for Cthonian.
//=========================================================
int CCthonian::IRelationship ( CBaseEntity *pTarget )
{
	return CBaseMonster :: IRelationship ( pTarget );
}

//=========================================================
// TakeDamage - overridden for Cthonian so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CCthonian :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	float flDist;
	Vector vecApex;

	// if the Cthonian is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if ( m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3 )
	{
		flDist = ( pev->origin - m_hEnemy->pev->origin ).Length2D();
		
		if ( flDist > CTHONIAN_SPRINT_DIST )
		{
			flDist = ( pev->origin - m_Route[ m_iRouteIndex ].vecLocation ).Length2D();// reusing flDist. 

			if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist * 0.5, m_hEnemy, &vecApex ) )
			{
				InsertWaypoint( vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY );
			}
		}
	}

	return CBaseMonster :: TakeDamage ( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CCthonian :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
	{
		// Cthonian will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if ( flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime )
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
			m_flNextSpitTime = gpGlobals->time + 2.5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - Cthonian is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CCthonian :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if ( m_hEnemy->pev->health <= gSkillData.cthonianDmgWhip && flDist <= 85 && flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - Cthonian is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CCthonian :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	if ( flDist <= 85 && flDot >= 0.7 && !HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )		// The player & Cthonian can be as much as their bboxes 
	{										// apart (48 * sqrt(3)) and he can still attack (85 is a little more than 48*sqrt(3))
		return TRUE;
	}
	return FALSE;
}  

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CCthonian :: FValidateHintType ( short sHint )
{
	int i;

	static short sCthonianHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for ( i = 0 ; i < ARRAYSIZE ( sCthonianHints ) ; i++ )
	{
		if ( sCthonianHints[ i ] == sHint )
		{
			return TRUE;
		}
	}

	ALERT ( at_aiconsole, "Couldn't validate hint type" );
	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CCthonian :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCthonian :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_PREDATOR;
}

//=========================================================
// IdleSound
//=========================================================
#define CTHONIAN_ATTN_IDLE	(float)1.5
void CCthonian :: IdleSound ( void )
{
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "cthonian/cth_idle1.wav", 1, CTHONIAN_ATTN_IDLE );	
		break;
	case 1:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "cthonian/cth_idle2.wav", 1, CTHONIAN_ATTN_IDLE );	
		break;
	case 2:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "cthonian/cth_idle3.wav", 1, CTHONIAN_ATTN_IDLE );	
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CCthonian :: PainSound ( void )
{
	int iPitch = RANDOM_LONG( 85, 120 );

	switch ( RANDOM_LONG(0,3) )
	{
	case 0:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cthonian/cth_pain1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cthonian/cth_pain2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 2:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cthonian/cth_pain3.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 3:	
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cthonian/cth_pain4.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CCthonian :: AlertSound ( void )
{
	int iPitch = RANDOM_LONG( 140, 160 );

	switch ( RANDOM_LONG ( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cthonian/cth_idle1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cthonian/cth_idle2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 2:
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "cthonian/cth_idle3.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCthonian :: SetYawSpeed ( void )
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case	ACT_WALK:			ys = 90;	break;
	case	ACT_RUN:			ys = 90;	break;
	case	ACT_IDLE:			ys = 90;	break;
	case	ACT_RANGE_ATTACK1:	ys = 90;	break;
	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCthonian :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case CTHONIAN_AE_SPIT:
		{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = ( gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 96 );		
			vecSpitOffset = ( pev->origin + vecSpitOffset );
			vecSpitDir = ( ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs ) - vecSpitOffset ).Normalize();

			vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );


			// do stuff for this event.
			AttackSound();

			// spew the spittle temporary ents.
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpitOffset );
				WRITE_BYTE( TE_SPRITE_SPRAY );
				WRITE_COORD( vecSpitOffset.x);	// pos
				WRITE_COORD( vecSpitOffset.y);	
				WRITE_COORD( vecSpitOffset.z);	
				WRITE_COORD( vecSpitDir.x);	// dir
				WRITE_COORD( vecSpitDir.y);	
				WRITE_COORD( vecSpitDir.z);	
				WRITE_SHORT( iCthonianSpitSprite );	// model
				WRITE_BYTE ( 15 );			// count
				WRITE_BYTE ( 210 );			// speed
				WRITE_BYTE ( 25 );			// noise ( client will divide by 100 )
			MESSAGE_END();

			CCthonianSpit::Shoot( pev, vecSpitOffset, vecSpitDir * 900 );
		}
		break;

		case CTHONIAN_AE_BITE:
		{
			// SOUND HERE!
			pev->origin.z -= 32;
			CBaseEntity *pHurt = CheckTraceHullAttack( 90, gSkillData.cthonianDmgBite, DMG_SLASH );
			pev->origin.z += 32;
			
			if ( pHurt )
			{
				//pHurt->pev->punchangle.z = -15;
				//pHurt->pev->punchangle.x = -45;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;

		case CTHONIAN_AE_TAILWHIP:
		{
			CBaseEntity *pHurt = CheckTraceHullAttack( 90, gSkillData.cthonianDmgWhip, DMG_CLUB | DMG_ALWAYSGIB );
			if ( pHurt ) 
			{
				pHurt->pev->punchangle.z = -20;
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;

		case CTHONIAN_AE_BLINK:
		{
			// close eye. 
			pev->skin = 1;
		}
		break;

		case CTHONIAN_AE_REAR:
		{
			// the Cthonian does not jump, but rears...hence, no need to throw into the air

			//float flGravity = CVAR_GET_FLOAT( "sv_gravity" );

			// throw the cthonian up into the air on this frame.
			//if ( FBitSet ( pev->flags, FL_ONGROUND ) )
			//{
			//	pev->flags -= FL_ONGROUND;
			//}

			// jump into air for 0.8 (24/30) seconds
//			pev->velocity.z += (0.875 * flGravity) * 0.5;
			//pev->velocity.z += (0.625 * flGravity) * 0.5;
		}
		break;

		case CTHONIAN_AE_THROW:
			{
				int iPitch;

				// cthonian throws its prey IF the prey is a client. 
				CBaseEntity *pHurt = CheckTraceHullAttack( 90, 0, 0 );


				if ( pHurt )
				{
					// croonchy bite sound : use bullsquid sounds
					iPitch = RANDOM_FLOAT( 90, 110 );
					switch ( RANDOM_LONG( 0, 1 ) )
					{
					case 0:
						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "cthonian/cth_bite1.wav", 1, ATTN_NORM, 0, iPitch );	
						break;
					case 1:
						EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "cthonian/cth_bite2.wav", 1, ATTN_NORM, 0, iPitch );	
						break;
					}

					
					//pHurt->pev->punchangle.x = RANDOM_LONG(0,34) - 5;
					//pHurt->pev->punchangle.z = RANDOM_LONG(0,49) - 25;
					//pHurt->pev->punchangle.y = RANDOM_LONG(0,89) - 45;
		
					// screeshake transforms the viewmodel as well as the viewangle. No problems with seeing the ends of the viewmodels.
					UTIL_ScreenShake( pHurt->pev->origin, 25.0, 1.5, 0.7, 2 );

					if ( pHurt->IsPlayer() )
					{
						UTIL_MakeVectors( pev->angles );
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 300 + gpGlobals->v_up * 300;
					}
				}
			}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CCthonian :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/cthonian.mdl");
	UTIL_SetSize( pev, Vector( -36, -36, 0 ), Vector( 36, 36, 100 ) );

	pev->solid			= SOLID_BBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->effects		= 0;
	if (pev->health == 0)
		pev->health			= gSkillData.cthonianHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	m_flNextSpitTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCthonian :: Precache()
{
	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/cthonian.mdl");
	
	PRECACHE_MODEL("sprites/bigspit.spr");// spit projectile.
	
	iCthonianSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("bullchicken/bc_attack2.wav");
	PRECACHE_SOUND("bullchicken/bc_attack3.wav");
	
	PRECACHE_SOUND("cthonian/cth_die1.wav");
	PRECACHE_SOUND("cthonian/cth_die2.wav");
	
	PRECACHE_SOUND("cthonian/cth_idle1.wav");
	PRECACHE_SOUND("cthonian/cth_idle2.wav");
	PRECACHE_SOUND("cthonian/cth_idle3.wav");
	
	PRECACHE_SOUND("cthonian/cth_pain1.wav");
	PRECACHE_SOUND("cthonian/cth_pain2.wav");
	PRECACHE_SOUND("cthonian/cth_pain3.wav");
	PRECACHE_SOUND("cthonian/cth_pain4.wav");
	
	PRECACHE_SOUND("cthonian/cth_attackgrowl.wav");
	//PRECACHE_SOUND("cthonian/cthonian_attackgrowl2.wav");
	//PRECACHE_SOUND("cthonian/cthonian_attackgrowl3.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");

	PRECACHE_SOUND("cthonian/cth_bite1.wav");
	PRECACHE_SOUND("cthonian/cth_bite2.wav");

	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");

}	

//=========================================================
// DeathSound
//=========================================================
void CCthonian :: DeathSound ( void )
{
	switch ( RANDOM_LONG(0,1) )
	{
	case 0:	
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "cthonian/cth_die1.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_VOICE, "cthonian/cth_die2.wav", 1, ATTN_NORM );	
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CCthonian :: AttackSound ( void )
{
	// use bullsquid sounds for spitting
	switch ( RANDOM_LONG(0,1) )
	{
	case 0:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack2.wav", 1, ATTN_NORM );	
		break;
	case 1:
		EMIT_SOUND( ENT(pev), CHAN_WEAPON, "bullchicken/bc_attack3.wav", 1, ATTN_NORM );	
		break;
	}
}


//========================================================
// RunAI - overridden for cthonian because there are things
// that need to be checked every think.
//========================================================
void CCthonian :: RunAI ( void )
{
	// first, do base class stuff
	CBaseMonster :: RunAI();

	if ( pev->skin != 0 )
	{
		// close eye if it was open.
		pev->skin = 0; 
	}

	if ( RANDOM_LONG(0,39) == 0 )
	{
		pev->skin = 1;
	}

	if ( m_hEnemy != NULL && m_Activity == ACT_RUN )
	{
		// chasing enemy. Sprint for last bit
		if ( (pev->origin - m_hEnemy->pev->origin).Length2D() < CTHONIAN_SPRINT_DIST )
		{
			pev->framerate = 1.25;
		}
	}

}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlCthonianRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slCthonianRangeAttack1[] =
{
	{ 
		tlCthonianRangeAttack1,
		ARRAYSIZE ( tlCthonianRangeAttack1 ), 
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED,
		0,
		"Cthonian Range Attack1"
	},
};

// Chase enemy schedule
Task_t tlCthonianChaseEnemy1[] = 
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_RANGE_ATTACK1	},// !!!OEM - this will stop nasty Cthonian oscillation.
	{ TASK_GET_PATH_TO_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,			(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0					},
};

Schedule_t slCthonianChaseEnemy[] =
{
	{ 
		tlCthonianChaseEnemy1,
		ARRAYSIZE ( tlCthonianChaseEnemy1 ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_SMELL_FOOD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_TASK_FAILED		|
		bits_COND_HEAR_SOUND,
		
		bits_SOUND_DANGER			|
		bits_SOUND_MEAT,
		"Cthonian Chase Enemy"
	},
};

Task_t tlCthonianHurtRear[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_SOUND_WAKE,			(float)0		},
	{ TASK_CTHONIAN_REARTURN,	(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},// in case Cthonian didn't turn all the way in the air.
};

Schedule_t slCthonianHurtRear[] =
{
	{
		tlCthonianHurtRear,
		ARRAYSIZE ( tlCthonianHurtRear ),
		0,
		0,
		"CthonianHurtRear"
	}
};

// Cthonian walks to something tasty and eats it.
Task_t tlCthonianEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the Cthonian can't get to the food
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slCthonianEat[] =
{
	{
		tlCthonianEat,
		ARRAYSIZE( tlCthonianEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"CthonianEat"
	}
};

// this is a bit different than just Eat. We use this schedule when the food is far away, occluded, or behind
// the Cthonian. This schedule plays a sniff animation before going to the source of food.
Task_t tlCthonianSniffAndEat[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the Cthonian can't get to the food
	{ TASK_PLAY_SEQUENCE,			(float)ACT_DETECT_SCENT },
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_EAT			},
	{ TASK_EAT,						(float)50				},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slCthonianSniffAndEat[] =
{
	{
		tlCthonianSniffAndEat,
		ARRAYSIZE( tlCthonianSniffAndEat ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_MEAT			|
		bits_SOUND_CARCASS,
		"CthonianSniffAndEat"
	}
};

// Cthonian does this to stinky things. 
Task_t tlCthonianWallow[] =
{
	{ TASK_STOP_MOVING,				(float)0				},
	{ TASK_EAT,						(float)10				},// this is in case the Cthonian can't get to the stinkiness
	{ TASK_STORE_LASTPOSITION,		(float)0				},
	{ TASK_GET_PATH_TO_BESTSCENT,	(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_INSPECT_FLOOR},
	{ TASK_EAT,						(float)50				},// keeps Cthonian from eating or sniffing anything else for a while.
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_WALK_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t slCthonianWallow[] =
{
	{
		tlCthonianWallow,
		ARRAYSIZE( tlCthonianWallow ),
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_NEW_ENEMY	,
		
		// even though HEAR_SOUND/SMELL FOOD doesn't break this schedule, we need this mask
		// here or the monster won't detect these sounds at ALL while running this schedule.
		bits_SOUND_GARBAGE,

		"CthonianWallow"
	}
};

DEFINE_CUSTOM_SCHEDULES( CCthonian ) 
{
	slCthonianRangeAttack1,
	slCthonianChaseEnemy,
	slCthonianHurtRear,
	slCthonianEat,
	slCthonianSniffAndEat,
	slCthonianWallow
};

IMPLEMENT_CUSTOM_SCHEDULES( CCthonian, CBaseMonster );

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CCthonian :: GetSchedule( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
			{
				return GetScheduleOfType ( SCHED_CTHONIAN_HURTREAR );
			}

			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound		*pSound;

				pSound = PBestScent();
				
				if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_CTHONIAN_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_CTHONIAN_EAT );
			}

			if ( HasConditions(bits_COND_SMELL) )
			{
				// there's something stinky. 
				CSound		*pSound;

				pSound = PBestScent();
				if ( pSound )
					return GetScheduleOfType( SCHED_CTHONIAN_WALLOW);
			}

			break;
		}
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}

			if ( HasConditions(bits_COND_SMELL_FOOD) )
			{
				CSound		*pSound;

				pSound = PBestScent();
				
				if ( pSound && (!FInViewCone ( &pSound->m_vecOrigin ) || !FVisible ( pSound->m_vecOrigin )) )
				{
					// scent is behind or occluded
					return GetScheduleOfType( SCHED_CTHONIAN_SNIFF_AND_EAT );
				}

				// food is right out in the open. Just go get it.
				return GetScheduleOfType( SCHED_CTHONIAN_EAT );
			}

			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_RANGE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}

			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK2 );
			}
			
			return GetScheduleOfType ( SCHED_CHASE_ENEMY );

			break;
		}
	}

	return CBaseMonster :: GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CCthonian :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_RANGE_ATTACK1:
		return &slCthonianRangeAttack1[ 0 ];
		break;
	case SCHED_CTHONIAN_HURTREAR:
		return &slCthonianHurtRear[ 0 ];
		break;
	case SCHED_CTHONIAN_EAT:
		return &slCthonianEat[ 0 ];
		break;
	case SCHED_CTHONIAN_SNIFF_AND_EAT:
		return &slCthonianSniffAndEat[ 0 ];
		break;
	case SCHED_CTHONIAN_WALLOW:
		return &slCthonianWallow[ 0 ];
		break;
	case SCHED_CHASE_ENEMY:
		return &slCthonianChaseEnemy[ 0 ];
		break;
	}

	return CBaseMonster :: GetScheduleOfType ( Type );
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for Cthonian because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CCthonian :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_MELEE_ATTACK2:
		{
			//switch ( RANDOM_LONG ( 0, 2 ) )
			//{
			//case 0:	
				EMIT_SOUND( ENT(pev), CHAN_VOICE, "cthonian/cth_attackgrowl.wav", 1, ATTN_NORM );		
			//	break;
			//}

			CBaseMonster :: StartTask ( pTask );
			break;
		}
	case TASK_CTHONIAN_REARTURN:
		{
			SetActivity ( ACT_HOP );  // this is not actually a hop, but a rear up...
			MakeIdealYaw ( m_vecEnemyLKP );
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			if ( BuildRoute ( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy ) )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			else
			{
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
	default:
		{
			CBaseMonster :: StartTask ( pTask );
			break;
		}
	}
}

//=========================================================
// RunTask
//=========================================================
void CCthonian :: RunTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_CTHONIAN_REARTURN:
		{
			MakeIdealYaw( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed );

			if ( m_fSequenceFinished )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CBaseMonster :: RunTask( pTask );
			break;
		}
	}
}


//=========================================================
// GetIdealState - Overridden for Cthonian. 
//=========================================================
MONSTERSTATE CCthonian :: GetIdealState ( void )
{
	int	iConditions;

	iConditions = IScheduleFlags();
	
	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
		{
			m_hEnemy = NULL;
			m_IdealMonsterState = MONSTERSTATE_ALERT;
			break;
		}
	}

	m_IdealMonsterState = CBaseMonster :: GetIdealState();

	return m_IdealMonsterState;
}

