/***
*
*	Copyright (c) 2007, Vyacheslav Dzhura
*	
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "effects.h"
#include "trains.h"

extern DLL_GLOBAL int		g_iSkillLevel;

#define SF_WAITFORTRIGGER	(0x04 | 0x40) // UNDONE: Fix
#define SF_NOWRECKAGE		0x08
#define SF_PATHCORNER_USELASER	8
#define SF_PATHCORNER_ATTACK	16

class CAlienFlyer : public CBaseMonster
{
	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	int  Classify( void ) { return CLASS_ALIEN_MONSTER; };
	int  BloodColor( void ) { return DONT_BLEED; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );
	//void (CAlienFlyer::*m_pfnCallWhenMoveDone)(void);
	void KeyValue(KeyValueData *pkvd);

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -300, -300, -172);
		pev->absmax = pev->origin + Vector(300, 300, 8);
	}

	void EXPORT HuntThink( void );
	void EXPORT FlyTouch( CBaseEntity *pOther );
	
	void EXPORT CrashTouch( CBaseEntity *pOther );
	void EXPORT DyingThink( void );
	void EXPORT StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT NullThink( void );

	void ShowDamage( void );
	void Flight( void );
	void UseDamagingBeam( bool bTurnOn );
	void CheckAttack( void );
	
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	//void EXPORT Wait( void );
	//void EXPORT Next( void );

	void Activate( void );

	float m_flForce;
	int m_iszDeathTarget;

	// from func_train
	entvars_t	*m_pevCurrentTarget;
	BOOL m_activated;
	// end of

	Vector m_vecTarget;
	Vector m_posTarget;

	Vector m_vecDesired;
	Vector m_posDesired;

	Vector m_vecGoal;

	float m_flLastSeen;
	float m_flPrevSeen;
	float m_flDieCounter;
	float m_flLaserCounter;
	float m_flAttackCounter;

	//int m_iSoundState; // don't save this

	int m_iSpriteTexture;
	int m_iExplode;
	int m_iBodyGibs;

	float m_flGoalSpeed;

	CBeam *m_pBeam[2];
};
//#define SetMoveDone( a ) m_pfnCallWhenMoveDone = static_cast <void (CAlienFlyer::*)(void)> (a)

LINK_ENTITY_TO_CLASS( monster_alienflyer, CAlienFlyer );

TYPEDESCRIPTION	CAlienFlyer::m_SaveData[] = 
{
	DEFINE_FIELD( CAlienFlyer, m_pevCurrentTarget, FIELD_EVARS ),
	DEFINE_FIELD( CAlienFlyer, m_activated, FIELD_BOOLEAN ),

	DEFINE_FIELD( CAlienFlyer, m_flForce, FIELD_FLOAT ),
	DEFINE_FIELD( CAlienFlyer, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CAlienFlyer, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CAlienFlyer, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( CAlienFlyer, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CAlienFlyer, m_vecGoal, FIELD_VECTOR ),
	DEFINE_FIELD( CAlienFlyer, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( CAlienFlyer, m_flPrevSeen, FIELD_TIME ),
	DEFINE_FIELD( CAlienFlyer, m_flGoalSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CAlienFlyer, m_iszDeathTarget, FIELD_STRING ),
	DEFINE_FIELD( CAlienFlyer, m_flLaserCounter, FIELD_FLOAT ),
	DEFINE_FIELD( CAlienFlyer, m_flDieCounter, FIELD_FLOAT ),
	DEFINE_FIELD( CAlienFlyer, m_flAttackCounter, FIELD_FLOAT ),
	DEFINE_ARRAY( CAlienFlyer, m_pBeam, FIELD_CLASSPTR , 2 ),
};
IMPLEMENT_SAVERESTORE( CAlienFlyer, CBaseMonster );

void CAlienFlyer::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "death_target"))
	{
		     m_iszDeathTarget = ALLOC_STRING(pkvd->szValue);
			 pkvd->fHandled = TRUE;
	}
	else
       CBaseEntity::KeyValue( pkvd );
}

void CAlienFlyer :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/flyer.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, -64 ), Vector( 32, 32, 0 ) );
	UTIL_SetOrigin( pev, pev->origin );

	pev->flags |= FL_MONSTER;
	pev->takedamage		= DAMAGE_AIM;
	pev->health			= 150*10; // gSkillData.apacheHealth;

	m_flFieldOfView = -0.707; // 270 degrees

	pev->sequence = 0;
	ResetSequenceInfo( );
	pev->frame = RANDOM_LONG(0, 0xFF);

	InitBoneControllers();

	m_pBeam[0] = CBeam::BeamCreate( "sprites/xenobeam.spr", 200 );
	m_pBeam[0]->SetFlags( SF_BEAM_SHADEIN );
	m_pBeam[0]->EntsInit( entindex( ), entindex( ) );
	m_pBeam[0]->SetStartAttachment( 0 );
	m_pBeam[0]->SetEndAttachment( 1 );
	m_pBeam[0]->SetColor( 254, 244, 233 );
	m_pBeam[0]->SetNoise( 3 );
	m_pBeam[0]->SetBrightness( 150 );
	m_pBeam[0]->SetWidth( 255 );
	m_pBeam[0]->SetScrollRate( 40 );

	m_pBeam[1] = CBeam::BeamCreate( "sprites/laserbeam.spr", 200 );
	m_pBeam[1]->SetFlags( SF_BEAM_SHADEIN );
	m_pBeam[1]->EntsInit( entindex( ), entindex( ) );
	m_pBeam[1]->SetStartAttachment( 0 );
	m_pBeam[1]->SetEndAttachment( 1 );
	m_pBeam[1]->SetColor( 255, 255, 255 );
	m_pBeam[1]->SetNoise( 100 );
	m_pBeam[1]->SetBrightness( 125 );
	m_pBeam[1]->SetWidth( 30 );
	m_pBeam[1]->SetScrollRate( 35 );

	UseDamagingBeam( false );

	if (pev->spawnflags & SF_WAITFORTRIGGER)
	{
		SetUse( StartupUse );
		m_activated = false;
		pev->effects |= EF_NODRAW;
	}
	else
	{
		SetThink( HuntThink );
		SetTouch( FlyTouch );
		pev->nextthink = gpGlobals->time + 1.0;
		m_activated = true;
	}
}

void CAlienFlyer::UseDamagingBeam( bool bTurnOn )
{
	if ( bTurnOn )
	{
		m_pBeam[0]->pev->effects &= ~EF_NODRAW;
		m_pBeam[1]->pev->effects &= ~EF_NODRAW;
	} else
	{
		m_pBeam[0]->pev->effects |= EF_NODRAW;
		m_pBeam[1]->pev->effects |= EF_NODRAW;
	}
}

void CAlienFlyer::Activate( void )
{
	// Not yet active, so teleport to first target
	if ( !m_activated )
	{
		m_activated = TRUE;
		entvars_t	*pevTarg = VARS( FIND_ENTITY_BY_TARGETNAME (NULL, STRING(pev->target) ) );
		
		pev->target = pevTarg->target;
		m_pevCurrentTarget = pevTarg;// keep track of this since path corners change our target for us.
    
		UTIL_SetOrigin	(pev, pevTarg->origin - (pev->mins + pev->maxs) * 0.5 );
		
		if ( FStringNull(pev->targetname) )
		{	// not triggered, so start immediately
			pev->nextthink = pev->ltime + 0.1;
			SetThink( HuntThink );
		}
		else
			pev->spawnflags |= SF_TRAIN_WAIT_RETRIGGER;
	}
}

void CAlienFlyer::Precache( void )
{
	PRECACHE_MODEL("models/flyer.mdl");

	m_iSpriteTexture = PRECACHE_MODEL( "sprites/white.spr" );
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("sprites/xenobeam.spr");

	m_iExplode	= PRECACHE_MODEL( "sprites/fexplo.spr" );
	m_iBodyGibs = PRECACHE_MODEL( "models/flyer_gibs.mdl" );
}



void CAlienFlyer::NullThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.5;
}


void CAlienFlyer::StartupUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	pev->effects &= ~EF_NODRAW;
	SetThink( HuntThink );
	SetTouch( FlyTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	SetUse( NULL );
}

void CAlienFlyer :: Killed( entvars_t *pevAttacker, int iGib )
{
	for ( int i = 0; i<2; i++) 
	{
		if (m_pBeam[i])
		{
			m_pBeam[i]->SUB_StartFadeOut();
			m_pBeam[i] = NULL;
		}
	}

	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3;

	UTIL_SetSize( pev, Vector( -32, -32, -64), Vector( 32, 32, 0) );
	SetThink( DyingThink );
	SetTouch( CrashTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;

	m_flDieCounter = gpGlobals->time + 2.5;

	FireTargets( STRING(m_iszDeathTarget), this, this, USE_TOGGLE, 1.0 );
}

void CAlienFlyer :: DyingThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	pev->avelocity = pev->avelocity * 1.02;

	// still falling?
	if (m_flDieCounter > gpGlobals->time )
	{
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG(0,29) + 30  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();

		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z );

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 132 );

			// velocity
			WRITE_COORD( pev->velocity.x ); 
			WRITE_COORD( pev->velocity.y );
			WRITE_COORD( pev->velocity.z );

			// randomization
			WRITE_BYTE( 25 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 1 );	// let client decide

			// duration
			WRITE_BYTE( 30 );// 3.0 seconds

			// flags

			WRITE_BYTE( BREAK_METAL );
		MESSAGE_END();

		// don't stop it we touch a entity
		pev->flags &= ~FL_ONGROUND;
		pev->nextthink = gpGlobals->time + 0.2;
		return;
	}
	else
	// fell
	{
		Vector vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

		// blast circle
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_BEAMCYLINDER );
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_COORD( pev->origin.x);
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z + 2000 ); // reach damage radius over .2 seconds
			WRITE_SHORT( m_iSpriteTexture );
			WRITE_BYTE( 0 ); // startframe
			WRITE_BYTE( 0 ); // framerate
			WRITE_BYTE( 4 ); // life
			WRITE_BYTE( 32 );  // width
			WRITE_BYTE( 0 );   // noise
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 255 );   // r, g, b
			WRITE_BYTE( 192 );   // r, g, b
			WRITE_BYTE( 128 ); // brightness
			WRITE_BYTE( 0 );		// speed
		MESSAGE_END();

		EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/mortarhit.wav", 1.0, 0.3);

		// Vyacheslav Dzhura: fix pointed by Barnz - damage was 300
		// so when players killed the flyer, flyer killed focusemitter, fun isn't it? :)
		RadiusDamage( pev->origin, pev, pev, 0, CLASS_NONE, DMG_BLAST );

		if (/*!(pev->spawnflags & SF_NOWRECKAGE) && */(pev->flags & FL_ONGROUND))
		{
			CBaseEntity *pWreckage = Create( "cycler_wreckage", pev->origin, pev->angles );
			// SET_MODEL( ENT(pWreckage->pev), STRING(pev->model) );
			UTIL_SetSize( pWreckage->pev, Vector( -200, -200, -128 ), Vector( 200, 200, -32 ) );
			pWreckage->pev->frame = pev->frame;
			pWreckage->pev->sequence = pev->sequence;
			pWreckage->pev->framerate = 0;
			pWreckage->pev->dmgtime = gpGlobals->time + 5;
		}

		// gibs
		vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpot );
			WRITE_BYTE( TE_BREAKMODEL);

			// position
			WRITE_COORD( vecSpot.x );
			WRITE_COORD( vecSpot.y );
			WRITE_COORD( vecSpot.z + 64);

			// size
			WRITE_COORD( 400 );
			WRITE_COORD( 400 );
			WRITE_COORD( 128 );

			// velocity
			WRITE_COORD( 0 ); 
			WRITE_COORD( 0 );
			WRITE_COORD( 200 );

			// randomization
			WRITE_BYTE( 30 ); 

			// Model
			WRITE_SHORT( m_iBodyGibs );	//model id#

			// # of shards
			WRITE_BYTE( 100 );

			// duration
			WRITE_BYTE( 200 );// 10.0 seconds

			// flags
			WRITE_BYTE( BREAK_FLESH );
		MESSAGE_END();

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}


void CAlienFlyer::FlyTouch( CBaseEntity *pOther )
{
	// bounce if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP) 
	{
		TraceResult tr = UTIL_GetGlobalTrace( );

		// UNDONE, do a real bounce
		pev->velocity = pev->velocity + tr.vecPlaneNormal * (pev->velocity.Length() + 200);
	}
}


void CAlienFlyer::CrashTouch( CBaseEntity *pOther )
{
	// only crash if we hit something solid
	if ( pOther->pev->solid == SOLID_BSP) 
	{
		SetTouch( NULL );
		pev->nextthink = gpGlobals->time;
	}
}

void CAlienFlyer :: CheckAttack( void )
{
	if (m_flAttackCounter < gpGlobals->time)
		return;

	CBaseEntity *pList[8];

	Vector delta = Vector( 32, 32, 0 );
	Vector mins = pev->origin - delta;
	Vector maxs = pev->origin + delta;
	maxs.z = pev->origin.z;
	mins.z -= 320;

	int count = UTIL_EntitiesInBox( pList, 8, mins, maxs, FL_MONSTER|FL_CLIENT );
	Vector forward;

	if (count > 1)
		ALERT( at_console, "(CAlienFlyer) Found %d victims:\n", count-1 );

	UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

	for ( int i = 0; i < count; i++ )
	{
		if ( pList[i] != this )
		{
			if ( pList[i]->pev->owner != edict() )
			{
				pList[i]->TakeDamage( pev, pev, 150, DMG_CRUSH | DMG_SLASH );
				ALERT( at_console, "(%s) %s\n", STRING(pList[i]->pev->classname), STRING(pList[i]->pev->targetname) );
				if ( FClassnameIs( pList[i]->pev, "item_focusemitter" ))
					m_flAttackCounter = gpGlobals->time - 1.0; // prevent multi-damage 2.5 second attack on focusemitter

				//pList[i]->pev->punchangle.x = 15;
				//pList[i]->pev->velocity = pList[i]->pev->velocity + forward * 100;
			}
		}
	}
}

void CAlienFlyer :: GibMonster( void )
{
	// EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200);		
}

void CAlienFlyer :: HuntThink( void )
{
	StudioFrameAdvance( );
	pev->nextthink = gpGlobals->time + 0.1;

	ShowDamage( );

	if ( m_pGoalEnt == NULL && !FStringNull(pev->target) )// this monster has a target
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if (m_pGoalEnt)
		{
			m_posDesired = m_pGoalEnt->pev->origin;
			UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
			m_vecGoal = gpGlobals->v_forward;
		}
	}

	Look( 4092 );
	m_hEnemy = BestVisibleEnemy( );

	// generic speed up
	if (m_flGoalSpeed < 800)
		m_flGoalSpeed += 10; // was 5

	if (m_hEnemy != NULL)
	{
		// ALERT( at_console, "%s\n", STRING( m_hEnemy->pev->classname ) );
		if (FVisible( m_hEnemy ))
		{
			if (m_flLastSeen < gpGlobals->time - 5)
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->Center( );
		}
		else
		{
			m_hEnemy = NULL;
		}
	}

	m_vecTarget = (m_posTarget - pev->origin).Normalize();

	float flLength = (pev->origin - m_posDesired).Length();

	if ( m_flLaserCounter < gpGlobals->time )
		UseDamagingBeam( false );

	if (m_pGoalEnt)
	{
		// ALERT( at_console, "%.0f\n", flLength );
		CheckAttack();

		// we are close to target entity, check next target
		// and if targeting entity has target, slowdown
		if (flLength < 128) 
		{
			// NEW CODE
			// Save last target in case we need to find it again
			pev->message = m_pGoalEnt->pev->targetname;

			if ( m_pGoalEnt->pev->spawnflags & SF_PATHCORNER_USELASER )
			{
				UseDamagingBeam( true );
				m_flLaserCounter = gpGlobals->time + 2.5;
			}

			if ( m_pGoalEnt->pev->spawnflags & SF_PATHCORNER_ATTACK )
			{
				m_flAttackCounter = gpGlobals->time + 2.5;
				CheckAttack();
			}

			if (m_pGoalEnt->pev->message)
			{
				m_flGoalSpeed -= 32;
				//if (flLength < 32)
				{
					// fire target's "message"
					if ( m_activated == false )
					{
						FireTargets( STRING( m_pGoalEnt->pev->message ), this, this, USE_TOGGLE, 1.0 );
						ALERT( at_console, "AlienFlyer used %s\n", STRING( m_pGoalEnt->pev->message ));
						m_activated = true;
					}
				}
			}
			// END OF NEW CODE

			m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( m_pGoalEnt->pev->target ) );
			if (m_pGoalEnt)
			{
				m_posDesired = m_pGoalEnt->pev->origin;
				UTIL_MakeAimVectors( m_pGoalEnt->pev->angles );
				m_vecGoal = gpGlobals->v_forward;
				flLength = (pev->origin - m_posDesired).Length();
			}
		} else
			m_activated = false;
	}
	else
	{
		m_posDesired = pev->origin;
	}

	if (flLength > 250) // 500
	{
		if (m_flLastSeen + 90 > gpGlobals->time && DotProduct( (m_posTarget - pev->origin).Normalize(), (m_posDesired - pev->origin).Normalize( )) > 0.25)
		{
			m_vecDesired = (m_posTarget - pev->origin).Normalize( );
		}
		else
		{
			m_vecDesired = (m_posDesired - pev->origin).Normalize( );
		}
	}
	else
	{
		m_vecDesired = m_vecGoal;
	}

	Flight( );

	// ALERT( at_console, "%.0f %.0f %.0f\n", gpGlobals->time, m_flLastSeen, m_flPrevSeen );
	if ((m_flLastSeen + 1 > gpGlobals->time) && (m_flPrevSeen + 2 < gpGlobals->time))
	{
		// FireGun was here

		// don't fire rockets and gun on easy mode
		//if (g_iSkillLevel == SKILL_EASY)
		//	m_flNextRocket = gpGlobals->time + 10.0;
	}

	UTIL_MakeAimVectors( pev->angles );
	Vector vecEst = (gpGlobals->v_forward * 800 + pev->velocity).Normalize( );
	// ALERT( at_console, "%d %d %d %4.2f\n", pev->angles.x < 0, DotProduct( pev->velocity, gpGlobals->v_forward ) > -100, m_flNextRocket < gpGlobals->time, DotProduct( m_vecTarget, vecEst ) );

	// rocket firing stuff was here
}


void CAlienFlyer :: Flight( void )
{
	// tilt model 5 degrees
	Vector vecAdj = Vector( 5.0, 0, 0 );

	// estimate where I'll be facing in one seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 2 + vecAdj);
	// Vector vecEst1 = pev->origin + pev->velocity + gpGlobals->v_up * m_flForce - Vector( 0, 0, 384 );
	// float flSide = DotProduct( m_posDesired - vecEst1, gpGlobals->v_right );
	
	float flSide = DotProduct( m_vecDesired, gpGlobals->v_right );

	if (flSide < 0)
	{
		if (pev->avelocity.y < 60)
		{
			pev->avelocity.y += 8; // 9 * (3.0/2.0);
		}
	}
	else
	{
		if (pev->avelocity.y > -60)
		{
			pev->avelocity.y -= 8; // 9 * (3.0/2.0);
		}
	}
	pev->avelocity.y *= 0.98;

	// estimate where I'll be in two seconds
	UTIL_MakeAimVectors( pev->angles + pev->avelocity * 1 + vecAdj);
	Vector vecEst = pev->origin + pev->velocity * 2.0 + gpGlobals->v_up * m_flForce * 20 - Vector( 0, 0, 384 * 2 );

	// add immediate force
	UTIL_MakeAimVectors( pev->angles + vecAdj);
	pev->velocity.x += gpGlobals->v_up.x * m_flForce;
	pev->velocity.y += gpGlobals->v_up.y * m_flForce;
	pev->velocity.z += gpGlobals->v_up.z * m_flForce;
	// add gravity
	pev->velocity.z -= 38.4; // 32ft/sec


	float flSpeed = pev->velocity.Length();
	float flDir = DotProduct( Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0 ), Vector( pev->velocity.x, pev->velocity.y, 0 ) );
	if (flDir < 0)
		flSpeed = -flSpeed;

	float flDist = DotProduct( m_posDesired - vecEst, gpGlobals->v_forward );

	// float flSlip = DotProduct( pev->velocity, gpGlobals->v_right );
	float flSlip = -DotProduct( m_posDesired - vecEst, gpGlobals->v_right );

	// fly sideways
	if (flSlip > 0)
	{
		if (pev->angles.z > -30 && pev->avelocity.z > -15)
			pev->avelocity.z -= 4;
		else
			pev->avelocity.z += 2;
	}
	else
	{

		if (pev->angles.z < 30 && pev->avelocity.z < 15)
			pev->avelocity.z += 4;
		else
			pev->avelocity.z -= 2;
	}

	// sideways drag
	pev->velocity.x = pev->velocity.x * (1.0 - fabs( gpGlobals->v_right.x ) * 0.05);
	pev->velocity.y = pev->velocity.y * (1.0 - fabs( gpGlobals->v_right.y ) * 0.05);
	pev->velocity.z = pev->velocity.z * (1.0 - fabs( gpGlobals->v_right.z ) * 0.05);

	// general drag
	pev->velocity = pev->velocity * 0.995;
	
	// apply power to stay correct height
	if (m_flForce < 80 && vecEst.z < m_posDesired.z) 
	{
		m_flForce += 12;
	}
	else if (m_flForce > 30)
	{
		if (vecEst.z > m_posDesired.z) 
			m_flForce -= 8;
	}

	// pitch forward or back to get to target
	if (flDist > 0 && flSpeed < m_flGoalSpeed && pev->angles.x + pev->avelocity.x > -40)
	{
		// ALERT( at_console, "F " );
		// lean forward
		pev->avelocity.x -= 24; //12.0;
	}
	else if (flDist < 0 && flSpeed > -50 && pev->angles.x + pev->avelocity.x  < 20)
	{
		// ALERT( at_console, "B " );
		// lean backward
		pev->avelocity.x += 12.0;
	}
	else if (pev->angles.x + pev->avelocity.x > 0)
	{
		// ALERT( at_console, "f " );
		pev->avelocity.x -= 4.0;
	}
	else if (pev->angles.x + pev->avelocity.x < 0)
	{
		// ALERT( at_console, "b " );
		pev->avelocity.x += 4.0;
	}

	// ALERT( at_console, "%.0f %.0f : %.0f %.0f : %.0f %.0f : %.0f\n", pev->origin.x, pev->velocity.x, flDist, flSpeed, pev->angles.x, pev->avelocity.x, m_flForce ); 
	// ALERT( at_console, "%.0f %.0f : %.0f %0.f : %.0f\n", pev->origin.z, pev->velocity.z, vecEst.z, m_posDesired.z, m_flForce ); 

    // rotor sounds were here
}

void CAlienFlyer :: ShowDamage( void )
{
    // smoke was here
}

int CAlienFlyer :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pevInflictor->owner == edict())
		return 0;

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2;
	}

	/*
	if ( (bitsDamageType & DMG_BULLET) && flDamage > 50)
	{
		// clip bullet damage at 50
		flDamage = 50;
	}
	*/

	// ALERT( at_console, "%.0f\n", flDamage );
	return CBaseEntity::TakeDamage(  pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CAlienFlyer::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	// ALERT( at_console, "%d %.0f\n", ptr->iHitgroup, flDamage );

	// ignore blades
	if (ptr->iHitgroup == 6 && (bitsDamageType & (DMG_ENERGYBEAM|DMG_BULLET|DMG_CLUB)))
		return;

	// hit hard, hits cockpit, hits engines
	if (flDamage > 50 || ptr->iHitgroup == 1 || ptr->iHitgroup == 2)
	{
		// ALERT( at_console, "%.0f\n", flDamage );
		AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
	}
	else
	{
		// do half damage in the body
		// AddMultiDamage( pevAttacker, this, flDamage / 2.0, bitsDamageType );
		UTIL_Ricochet( ptr->vecEndPos, 2.0 );
	}
}