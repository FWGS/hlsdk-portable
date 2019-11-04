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
#ifndef OEM_BUILD

//=========================================================
// TRex
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"nodes.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"customentity.h"
#include	"weapons.h"
#include	"effects.h"
#include	"soundent.h"
#include	"decals.h"
#include	"explode.h"
#include	"func_break.h"
#include	"scripted.h"

#include "spiral.h"


#include "TRex.h"

//=========================================================
// TRex Monster
//=========================================================
const float TREX_ATTACKDIST = 320.0;

// TRex animation events
#define TREX_AE_SLASH_LEFT			1
//#define TREX_AE_BEAM_ATTACK_RIGHT	2		// No longer used
#define TREX_AE_LEFT_FOOT			3
#define TREX_AE_RIGHT_FOOT			4
#define TREX_AE_STOMP				5
#define TREX_AE_BREATHE				6
#define TREX_AE_ROAR				7


// TRex is immune to any damage but this
#define TREX_BEAM_SPRITE_NAME		"sprites/xbeam3.spr"
#define TREX_BEAM_SPRITE2			"sprites/xbeam3.spr"
#define TREX_STOMP_SPRITE_NAME		"sprites/gargeye1.spr"
#define TREX_STOMP_BUZZ_SOUND		"weapons/mine_charge.wav"
#define TREX_FLAME_LENGTH			330
//#define TREX_GIB_MODEL				"models/metalplategibs.mdl"

#define ATTN_TREX					(ATTN_NORM)

#define STOMP_SPRITE_COUNT			10

int tStompSprite = 0;
//int gTRexGibModel = 0;

LINK_ENTITY_TO_CLASS( trex_stomp, CTRexStomp );
CTRexStomp *CTRexStomp::StompCreate( const Vector &origin, const Vector &end, float speed )
{
	CTRexStomp *pStomp = GetClassPtr( (CTRexStomp *)NULL );
	
	pStomp->pev->origin = origin;
	Vector dir = (end - origin);
	pStomp->pev->scale = dir.Length();
	pStomp->pev->movedir = dir.Normalize();
	pStomp->pev->speed = speed;
	pStomp->Spawn();
	
	return pStomp;
}

void CTRexStomp::Spawn( void )
{
	SetNextThink( 0 );
	pev->classname = MAKE_STRING("trex_stomp");
	pev->dmgtime = gpGlobals->time;

	pev->framerate = 30;
	pev->model = MAKE_STRING(TREX_STOMP_SPRITE_NAME);
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	EMIT_SOUND_DYN( edict(), CHAN_BODY, TREX_STOMP_BUZZ_SOUND, 1, ATTN_NORM, 0, PITCH_NORM * 0.55);
}


#define	STOMP_INTERVAL		0.025

void CTRexStomp::Think( void )
{
	TraceResult tr;

	SetNextThink( 0.1 );

	// Do damage for this frame
	Vector vecStart = pev->origin;
	vecStart.z += 30;
	Vector vecEnd = vecStart + (pev->movedir * pev->speed * gpGlobals->frametime);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit && tr.pHit != pev->owner )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		entvars_t *pevOwner = pev;
		if ( pev->owner )
			pevOwner = VARS(pev->owner);

		if ( pEntity )
			pEntity->TakeDamage( pev, pevOwner, gSkillData.gargantuaDmgStomp, DMG_SONIC );
	}
	
	// Accelerate the effect
	pev->speed = pev->speed + (gpGlobals->frametime) * pev->framerate;
	pev->framerate = pev->framerate + (gpGlobals->frametime) * 1500;
	
	// Move and spawn trails
	while ( gpGlobals->time - pev->dmgtime > STOMP_INTERVAL )
	{
		pev->origin = pev->origin + pev->movedir * pev->speed * STOMP_INTERVAL;
		for ( int i = 0; i < 2; i++ )
		{
			CSprite *pSprite = CSprite::SpriteCreate( TREX_STOMP_SPRITE_NAME, pev->origin, TRUE );
			if ( pSprite )
			{
				UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,500), ignore_monsters, edict(), &tr );
				pSprite->pev->origin = tr.vecEndPos;
				pSprite->pev->velocity = Vector(RANDOM_FLOAT(-200,200),RANDOM_FLOAT(-200,200),175);
				// pSprite->AnimateAndDie( RANDOM_FLOAT( 8.0, 12.0 ) );
				pSprite->SetNextThink( 0.3 );
				pSprite->SetThink( SUB_Remove );
				pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxFadeFast );
			}
		}
		pev->dmgtime += STOMP_INTERVAL;
		// Scale has the "life" of this effect
		pev->scale -= STOMP_INTERVAL * pev->speed;
		if ( pev->scale <= 0 )
		{
			// Life has run out
			UTIL_Remove(this);
			STOP_SOUND( edict(), CHAN_BODY, TREX_STOMP_BUZZ_SOUND );
		}

	}
}


void TRexStreakSplash( const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
		WRITE_BYTE( TE_STREAK_SPLASH );
		WRITE_COORD( origin.x );		// origin
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( direction.x );	// direction
		WRITE_COORD( direction.y );
		WRITE_COORD( direction.z );
		WRITE_BYTE( color );	// Streak color 6
		WRITE_SHORT( count );	// count
		WRITE_SHORT( speed );
		WRITE_SHORT( velocityRange );	// Random velocity modifier
	MESSAGE_END();
}



LINK_ENTITY_TO_CLASS( monster_trex, CTRex );

TYPEDESCRIPTION	CTRex::m_SaveData[] = 
{
	DEFINE_FIELD( CTRex, m_seeTime, FIELD_TIME ),
	DEFINE_FIELD( CTRex, m_flameTime, FIELD_TIME ),
	DEFINE_FIELD( CTRex, m_streakTime, FIELD_TIME ),
	DEFINE_FIELD( CTRex, m_painSoundTime, FIELD_TIME ),
	DEFINE_ARRAY( CTRex, m_pFlame, FIELD_CLASSPTR, 2 ),
	DEFINE_FIELD( CTRex, m_flameX, FIELD_FLOAT ),
	DEFINE_FIELD( CTRex, m_flameY, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CTRex, CBaseMonster );

const char *CTRex::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CTRex::pBeamAttackSounds[] = 
{
	"garg/gar_flameoff1.wav",
	"garg/gar_flameon1.wav",
	"garg/gar_flamerun1.wav",
};


const char *CTRex::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CTRex::pRicSounds[] = 
{
#if 0
	"weapons/ric1.wav",
	"weapons/ric2.wav",
	"weapons/ric3.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
#else
	"debris/metal4.wav",
	"debris/metal6.wav",
	"weapons/ric4.wav",
	"weapons/ric5.wav",
#endif
};

const char *CTRex::pFootSounds[] = 
{
	"garg/gar_step1.wav",
	"garg/gar_step2.wav",
};


const char *CTRex::pIdleSounds[] = 
{
	"garg/gar_idle1.wav",
	"garg/gar_idle2.wav",
	"garg/gar_idle3.wav",
	"garg/gar_idle4.wav",
	"garg/gar_idle5.wav",
};


const char *CTRex::pAttackSounds[] = 
{
	"garg/gar_attack1.wav",
	"garg/gar_attack2.wav",
	"garg/gar_attack3.wav",
};

const char *CTRex::pAlertSounds[] = 
{
	"garg/gar_alert1.wav",
	"garg/gar_alert2.wav",
	"garg/gar_alert3.wav",
};

const char *CTRex::pPainSounds[] = 
{
	"garg/gar_pain1.wav",
	"garg/gar_pain2.wav",
	"garg/gar_pain3.wav",
};

const char *CTRex::pStompSounds[] = 
{
	"garg/gar_stomp1.wav",
};

const char *CTRex::pBreatheSounds[] = 
{
	"garg/gar_breathe1.wav",
	"garg/gar_breathe2.wav",
	"garg/gar_breathe3.wav",
};

const char *CTRex::pRoarSounds[] = 
{
	"trex/trex_roar1.wav",
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
#if 0
enum
{
	SCHED_ = LAST_COMMON_SCHEDULE + 1,
};
#endif

enum
{
	TASK_SOUND_ATTACK = LAST_COMMON_TASK + 1,
	TASK_FLAME_SWEEP,
};

Task_t	tlTRexFlame[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOUND_ATTACK,		(float)0		},
	// { TASK_PLAY_SEQUENCE,		(float)ACT_SIGNAL1	},
	{ TASK_SET_ACTIVITY,		(float)ACT_MELEE_ATTACK2 },
	{ TASK_FLAME_SWEEP,			(float)4.5		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slTRexFlame[] =
{
	{ 
		tlTRexFlame,
		ARRAYSIZE ( tlTRexFlame ), 
		0,
		0,
		"TRexFlame"
	},
};


// primary melee attack
Task_t	tlTRexSwipe[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slTRexSwipe[] =
{
	{ 
		tlTRexSwipe,
		ARRAYSIZE ( tlTRexSwipe ), 
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"TRexSwipe"
	},
};


DEFINE_CUSTOM_SCHEDULES( CTRex )
{
	slTRexFlame,
	slTRexSwipe,
};

IMPLEMENT_CUSTOM_SCHEDULES( CTRex, CBaseMonster );


void CTRex::StompAttack( void )
{
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin + Vector(0,0,60) + 35 * gpGlobals->v_forward;
	Vector vecAim = ShootAtEnemy( vecStart );
	Vector vecEnd = (vecAim * 1024) + vecStart;

	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &trace );
	CTRexStomp::StompCreate( vecStart, trace.vecEndPos, 0 );
	UTIL_ScreenShake( pev->origin, 12.0, 100.0, 2.0, 1000 );
	EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pStompSounds[ RANDOM_LONG(0,ARRAYSIZE(pStompSounds)-1) ], 1.0, ATTN_TREX, 0, PITCH_NORM + RANDOM_LONG(-10,10) );

	UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,20), ignore_monsters, edict(), &trace );
	if ( trace.flFraction < 1.0 )
		UTIL_DecalTrace( &trace, DECAL_GARGSTOMP1 );
}


void CTRex :: FlameCreate( void )
{
	int			i;
	Vector		posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	
	for ( i = 0; i < 2; i++ )
	{
		if ( i < 2 )
			m_pFlame[i] = CBeam::BeamCreate( TREX_BEAM_SPRITE_NAME, 240 );
		else
			m_pFlame[i] = CBeam::BeamCreate( TREX_BEAM_SPRITE2, 140 );
		if ( m_pFlame[i] )
		{
			int attach = i%2;
			// attachment is 0 based in GetAttachment
			GetAttachment( attach+1, posGun, angleGun );

			Vector vecEnd = (gpGlobals->v_forward * TREX_FLAME_LENGTH) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->PointEntInit( trace.vecEndPos, entindex() );
			if ( i < 2 )
				m_pFlame[i]->SetColor( 255, 130, 90 );
			else
				m_pFlame[i]->SetColor( 0, 120, 255 );
			m_pFlame[i]->SetBrightness( 190 );
			m_pFlame[i]->SetFlags( BEAM_FSHADEIN );
			m_pFlame[i]->SetScrollRate( 20 );
			// attachment is 1 based in SetEndAttachment
			m_pFlame[i]->SetEndAttachment( attach + 2 );
			CSoundEnt::InsertSound( bits_SOUND_COMBAT, posGun, 384, 0.3 );
		}
	}
	EMIT_SOUND_DYN ( edict(), CHAN_BODY, pBeamAttackSounds[ 1 ], 1.0, ATTN_NORM, 0, PITCH_NORM );
	EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pBeamAttackSounds[ 2 ], 1.0, ATTN_NORM, 0, PITCH_NORM );
}


void CTRex :: FlameControls( float angleX, float angleY )
{
	if ( angleY < -180 )
		angleY += 360;
	else if ( angleY > 180 )
		angleY -= 360;

	if ( angleY < -45 )
		angleY = -45;
	else if ( angleY > 45 )
		angleY = 45;

	m_flameX = UTIL_ApproachAngle( angleX, m_flameX, 4 );
	m_flameY = UTIL_ApproachAngle( angleY, m_flameY, 8 );
	SetBoneController( 0, m_flameY );
	SetBoneController( 1, m_flameX );
}


void CTRex :: FlameUpdate( void )
{
	int				i;
	static float	offset[2] = { 60, -60 };
	TraceResult		trace;
	Vector			vecStart, angleGun;
	BOOL			streaks = FALSE;

	for ( i = 0; i < 1; i++ )
	{
		if ( m_pFlame[i] )
		{
			Vector vecAim = pev->angles;
			vecAim.x += m_flameX;
			vecAim.y += m_flameY;

			UTIL_MakeVectors( vecAim );

			GetAttachment( i+1, vecStart, angleGun );
			Vector vecEnd = vecStart + (gpGlobals->v_forward * TREX_FLAME_LENGTH); //  - offset[i] * gpGlobals->v_right;

			UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->SetStartPos( trace.vecEndPos );

			if ( trace.flFraction != 1.0 && gpGlobals->time > m_streakTime )
			{
				TRexStreakSplash( trace.vecEndPos, trace.vecPlaneNormal, 6, 20, 50, 400 );
				streaks = TRUE;
				UTIL_DecalTrace( &trace, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2) );
			}
			// RadiusDamage( trace.vecEndPos, pev, pev, gSkillData.gargantuaDmgFire, CLASS_ALIEN_MONSTER, DMG_BURN );
			FlameDamage( vecStart, trace.vecEndPos, pev, pev, gSkillData.gargantuaDmgFire, CLASS_ALIEN_MONSTER, DMG_BURN );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x1000 * (i + 2) );		// entity, attachment
				WRITE_COORD( vecStart.x );		// origin
				WRITE_COORD( vecStart.y );
				WRITE_COORD( vecStart.z );
				WRITE_COORD( RANDOM_FLOAT( 32, 48 ) );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 255 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 2 );	// life * 10
				WRITE_COORD( 0 ); // decay
			MESSAGE_END();
		}
	}
	if ( streaks )
		m_streakTime = gpGlobals->time;
}



void CTRex :: FlameDamage( Vector vecStart, Vector vecEnd, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage;
	Vector		vecSpot;

	Vector vecMid = (vecStart + vecEnd) * 0.5;

	float searchRadius = (vecStart - vecMid).Length();

	Vector vecAim = (vecEnd - vecStart).Normalize( );

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecMid, searchRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}
			
			vecSpot = pEntity->BodyTarget( vecMid );
		
			float dist = DotProduct( vecAim, vecSpot - vecMid );
			if (dist > searchRadius)
				dist = searchRadius;
			else if (dist < -searchRadius)
				dist = searchRadius;
			
			Vector vecSrc = vecMid + dist * vecAim;

			UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pev), &tr );

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{// the explosion can 'see' this entity, so hurt them!
				// decrease damage for an ent that's farther from the flame.
				dist = ( vecSrc - tr.vecEndPos ).Length();

				if (dist > 64)
				{
					flAdjustedDamage = flDamage - (dist - 64) * 0.4;
					if (flAdjustedDamage <= 0)
						continue;
				}
				else
				{
					flAdjustedDamage = flDamage;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage( );
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
		}
	}
}


void CTRex :: FlameDestroy( void )
{
	int i;

	EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pBeamAttackSounds[ 0 ], 1.0, ATTN_NORM, 0, PITCH_NORM );
	for ( i = 0; i < 2; i++ )
	{
		if ( m_pFlame[i] )
		{
			UTIL_Remove( m_pFlame[i] );
			m_pFlame[i] = NULL;
		}
	}
}



//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CTRex :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_PREDATOR;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CTRex :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 60;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_WALK:
	case ACT_RUN:
		ys = 60;
		break;

	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// Spawn
//=========================================================
void CTRex :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/trex.mdl");
	UTIL_SetSize( pev, Vector( -64, -64, 0 ), Vector( 64, 64, 128 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= gSkillData.gargantuaHealth;
	//pev->view_ofs		= Vector ( 0, 0, 96 );// taken from mdl file
	m_flFieldOfView		= -0.2;// width of forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	m_seeTime = gpGlobals->time + 5;
	m_flameTime = gpGlobals->time + 2;
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTRex :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/trex.mdl");
	PRECACHE_MODEL( TREX_BEAM_SPRITE_NAME );
	PRECACHE_MODEL( TREX_BEAM_SPRITE2 );
	tStompSprite = PRECACHE_MODEL( TREX_STOMP_SPRITE_NAME );
	//gTRexGibModel = PRECACHE_MODEL( TREX_GIB_MODEL );
	PRECACHE_SOUND( TREX_STOMP_BUZZ_SOUND );

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pBeamAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pBeamAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pRicSounds ); i++ )
		PRECACHE_SOUND((char *)pRicSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pFootSounds ); i++ )
		PRECACHE_SOUND((char *)pFootSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pStompSounds ); i++ )
		PRECACHE_SOUND((char *)pStompSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pBreatheSounds ); i++ )
		PRECACHE_SOUND((char *)pBreatheSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pRoarSounds ); i++ )
		PRECACHE_SOUND((char *)pRoarSounds[i]);
}	


void CTRex::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	ALERT( at_aiconsole, "CTRex::TraceAttack\n");

	if ( !IsAlive() )
	{
		CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
		return;
	}

	if ( m_painSoundTime < gpGlobals->time )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_TREX, 0, PITCH_NORM );
		m_painSoundTime = gpGlobals->time + RANDOM_FLOAT( 2.5, 4 );
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );

}



int CTRex::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	ALERT( at_aiconsole, "CTRex::TakeDamage\n");

	if (bitsDamageType & DMG_SONIC)
	{
		return 0;
	}

	if ( IsAlive() )
	{
		// only take a bit of damage from melee weapons
		if ( !(bitsDamageType & DMG_BULLET|DMG_SLASH|DMG_CLUB) )
			flDamage *= 0.1;
		if ( bitsDamageType & DMG_BLAST )
			SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CTRex::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// CheckMeleeAttack1
// TRex swipe attack
// 
//=========================================================
BOOL CTRex::CheckMeleeAttack1( float flDot, float flDist )
{
//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	if (flDot >= 0.7)
	{
		if (flDist <= TREX_ATTACKDIST)
			return TRUE;
	}
	return FALSE;
}


// Flame thrower madness!
BOOL CTRex::CheckMeleeAttack2( float flDot, float flDist )
{
//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	//if ( gpGlobals->time > m_flameTime )
	//{
	//	if (flDot >= 0.8 && flDist > TREX_ATTACKDIST)
	//	{
	//		if ( flDist <= TREX_FLAME_LENGTH )
	//			return TRUE;
	//	}
	//}
	return FALSE;
}


//=========================================================
// CheckRangeAttack1
// flDot is the cos of the angle of the cone within which
// the attack can occur.
//=========================================================
//
// Stomp attack
//
//=========================================================
BOOL CTRex::CheckRangeAttack1( float flDot, float flDist )
{
	if ( gpGlobals->time > m_seeTime )
	{
		if (flDot >= 0.7 && flDist > TREX_ATTACKDIST)
		{
				return TRUE;
		}
	}
	return FALSE;
}




//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTRex::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch( pEvent->event )
	{
	case TREX_AE_SLASH_LEFT:
		{
			// HACKHACK!!!
			CBaseEntity *pHurt = TRexCheckTraceHullAttack( TREX_ATTACKDIST + 10.0, gSkillData.gargantuaDmgSlash, DMG_SLASH );
			if (pHurt)
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = -30; // pitch
					pHurt->pev->punchangle.y = -30;	// yaw
					pHurt->pev->punchangle.z = 30;	// roll
					//UTIL_MakeVectors(pev->angles);	// called by CheckTraceHullAttack
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 50 + RANDOM_LONG(0,15) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 50 + RANDOM_LONG(0,15) );

			Vector forward;
			UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );
		}
		break;

	case TREX_AE_RIGHT_FOOT:
	case TREX_AE_LEFT_FOOT:
		UTIL_ScreenShake( pev->origin, 4.0, 3.0, 1.0, 750 );
		EMIT_SOUND_DYN ( edict(), CHAN_BODY, pFootSounds[ RANDOM_LONG(0,ARRAYSIZE(pFootSounds)-1) ], 1.0, ATTN_TREX, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		break;

	case TREX_AE_STOMP:
		StompAttack();
		m_seeTime = gpGlobals->time + 12;
		break;

	case TREX_AE_BREATHE:
		if ( !(pev->spawnflags & SF_MONSTER_GAG) || m_MonsterState != MONSTERSTATE_IDLE)
			EMIT_SOUND_DYN ( edict(), CHAN_VOICE, pBreatheSounds[ RANDOM_LONG(0,ARRAYSIZE(pBreatheSounds)-1) ], 1.0, ATTN_TREX, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		break;

	case TREX_AE_ROAR:
		EMIT_SOUND_DYN ( edict(), CHAN_VOICE, pRoarSounds[ RANDOM_LONG(0,ARRAYSIZE(pRoarSounds)-1) ], 1.0, ATTN_TREX, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
// Used for many contact-range melee attacks. Bites, claws, etc.

// Overridden for TRex because his swing starts lower as
// a percentage of his height (otherwise he swings over the
// players head)
//=========================================================
CBaseEntity* CTRex::TRexCheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += 64;
//	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) - (gpGlobals->v_up * flDist * 0.3);
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) - (gpGlobals->v_up * 32);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}


Schedule_t *CTRex::GetScheduleOfType( int Type )
{
	// HACKHACK - turn off the flames if they are on and TRex goes scripted / dead
	if ( FlameIsOn() )
		FlameDestroy();

	switch( Type )
	{
		case SCHED_MELEE_ATTACK2:
			return slTRexFlame;
		case SCHED_MELEE_ATTACK1:
			return slTRexSwipe;
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}


void CTRex::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FLAME_SWEEP:
		FlameCreate();
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		m_flameTime = gpGlobals->time + 6;
		m_flameX = 0;
		m_flameY = 0;
		break;

	case TASK_SOUND_ATTACK:
		if ( RANDOM_LONG(0,100) < 30 )
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_TREX, 0, PITCH_NORM );
		TaskComplete();
		break;

	// allow a scripted_action to make TRex shoot flames.
	case TASK_PLAY_SCRIPT:
		if ( m_pCine->IsAction() && m_pCine->m_fAction == 3)
		{
			FlameCreate();
			m_flWaitFinished = gpGlobals->time + 4.5;
			m_flameTime = gpGlobals->time + 6;
			m_flameX = 0;
			m_flameY = 0;
		}
		CBaseMonster::RunTask( pTask );
		break;

	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + 1.6;
		// FALL THROUGH
	default: 
		CBaseMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CTRex::RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		if ( gpGlobals->time > m_flWaitFinished )
		{
			pev->renderfx = kRenderFxExplode;
			pev->rendercolor.x = 255;
			pev->rendercolor.y = 0;
			pev->rendercolor.z = 0;
			StopAnimation();
			SetNextThink( 0.15 );
			SetThink( SUB_Remove );
			/*
			int i;
			int parts = MODEL_FRAMES( gTRexGibModel );
			for ( i = 0; i < 10; i++ )
			{
				CGib *pGib = GetClassPtr( (CGib *)NULL );

				pGib->Spawn( TREX_GIB_MODEL );
				
				int bodyPart = 0;
				if ( parts > 1 )
					bodyPart = RANDOM_LONG( 0, pev->body-1 );

				pGib->pev->body = bodyPart;
				pGib->m_bloodColor = BLOOD_COLOR_YELLOW;
				pGib->m_material = matNone;
				pGib->pev->origin = pev->origin;
				pGib->pev->velocity = UTIL_RandomBloodVector() * RANDOM_FLOAT( 300, 500 );
				pGib->SetNextThink( 1.25 );
				pGib->SetThink( SUB_FadeOut );
			}
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );

				// size
				WRITE_COORD( 200 );
				WRITE_COORD( 200 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 ); 
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );

				// randomization
				WRITE_BYTE( 200 ); 

				// Model
				WRITE_SHORT( gTRexGibModel );	//model id#

				// # of shards
				WRITE_BYTE( 50 );

				// duration
				WRITE_BYTE( 20 );// 3.0 seconds

				// flags

				WRITE_BYTE( BREAK_FLESH );
			MESSAGE_END();
			*/

			return;
		}
		else
			CBaseMonster::RunTask(pTask);
		break;

	case TASK_PLAY_SCRIPT:
		if (!m_pCine->IsAction() || m_pCine->m_fAction != 3)
		{
			CBaseMonster::RunTask( pTask );
			break;
		}
		else
		{
			if (m_fSequenceFinished)
			{
				FlameDestroy();
				FlameControls( 0, 0 );
				SetBoneController( 0, 0 );
				SetBoneController( 1, 0 );
				m_pCine->SequenceDone( this );
				break;
			}
			//if not finished, carry on into task_flame_sweep!
		}
	case TASK_FLAME_SWEEP:
		if ( gpGlobals->time > m_flWaitFinished )
		{
			FlameDestroy();
			TaskComplete();
			FlameControls( 0, 0 );
			SetBoneController( 0, 0 );
			SetBoneController( 1, 0 );
		}
		else
		{
			BOOL cancel = FALSE;

			Vector angles = g_vecZero;

			FlameUpdate();
			CBaseEntity *pEnemy;
			if (m_pCine) // LRC- are we obeying a scripted_action?
				pEnemy = m_hTargetEnt;
			else
				pEnemy = m_hEnemy;
			if ( pEnemy )
			{
				Vector org = pev->origin;
				org.z += 64;
				Vector dir = pEnemy->BodyTarget(org) - org;
				angles = UTIL_VecToAngles( dir );
				angles.x = -angles.x;
				angles.y -= pev->angles.y;
				if ( dir.Length() > 400 )
					cancel = TRUE;
			}
			if ( fabs(angles.y) > 60 )
				cancel = TRUE;
			
			if ( cancel )
			{
				m_flWaitFinished -= 0.5;
				m_flameTime -= 0.5;
			}
			// FlameControls( angles.x + 2 * sin(gpGlobals->time*8), angles.y + 28 * sin(gpGlobals->time*8.5) );
			FlameControls( angles.x, angles.y );
		}
		break;

	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}


#endif
