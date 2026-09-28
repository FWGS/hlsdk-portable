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
// Alien slave monster
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"squadmonster.h"
#include	"schedule.h"
#include	"effects.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"decals.h"

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CDomaFireBall : public CBaseEntity
{
public:
	void Spawn( void );

	static void Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity );
	void Touch( CBaseEntity *pOther );
};

LINK_ENTITY_TO_CLASS( doma_fire_bolt, CDomaFireBall );

void CDomaFireBall:: Spawn( void )
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "doma_fire_bolt" );
	
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "models/fire_bolt.mdl");
	pev->frame = 0;
	pev->framerate = 1.0;
	pev->body = 1;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	FX_Trail(pev->origin, entindex(), PROJ_FLAME );

	pev->effects		= EF_DIMLIGHT;
}

void CDomaFireBall::Shoot( entvars_t *pevOwner, Vector vecStart, Vector vecVelocity )
{
	CDomaFireBall *pSpit = GetClassPtr( (CDomaFireBall *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit->pev, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->pev->angles = UTIL_VecToAngles (pSpit->pev->velocity);
}

void CDomaFireBall :: Touch ( CBaseEntity *pOther )
{
	TraceResult tr;

	Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	UTIL_TraceLine( vecSpot, vecSpot + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr );
	vecSpot = (tr.vecEndPos + tr.vecPlaneNormal * 15);

	FX_Explosion( vecSpot, 132 );
	
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( g_sModelIndexFireball );
		WRITE_BYTE( 0  ); // scale * 10
		WRITE_BYTE( 15  ); // framerate
		WRITE_BYTE( 10 );
	MESSAGE_END();

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	if ( pOther->pev->flags & FL_MONSTER ){
		CBaseMonster *pEnemyMonster;
		pEnemyMonster = pOther->MyMonsterPointer();
		if(pEnemyMonster){
		pEnemyMonster->m_trouch_full_radiusdmg += 1;
		}
	}

	::RadiusDamage_limit( vecSpot, pev, pevOwner, 80, 200, CLASS_ALIEN_MONSTER, DMG_BURN | DMG_BLAST);

	if ( RANDOM_FLOAT( 0 , 1 ) < 0.5 )
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
	}
	else
	{
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
	}

	FX_Trail(pev->origin, entindex(), PROJ_REMOVE );
	UTIL_Remove(this);
}

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================


//=========================================================
// repel 
//=========================================================
Task_t	tlDomaRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		    },
	{ TASK_FACE_IDEAL,			(float)0		    },
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slDomaRepel[] =
{
	{ 
		tlDomaRepel,
		ARRAYSIZE ( tlDomaRepel ), 
		bits_COND_SEE_ENEMY,
		0, 
		"Repel"
	},
};


//=========================================================
// repel 
//=========================================================
Task_t	tlDomaRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slDomaRepelAttack[] =
{
	{ 
		tlDomaRepelAttack,
		ARRAYSIZE ( tlDomaRepelAttack ), 
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

Task_t	tlDomaLeap[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_LEAP },
};

Schedule_t slDomaLeap[] = 
{
	{
		tlDomaLeap,
		ARRAYSIZE ( tlDomaLeap ),
		0,
		0,
		"Doma Leap Fly"
	}
};

Task_t	tlDomaLandAttack[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_LAND },
};

Schedule_t slDomaLandAttack[] = 
{
	{
		tlDomaLandAttack,
		ARRAYSIZE ( tlDomaLandAttack ),
		0,
		0,
		"Doma Land Attack"
	}
};


class CDoma : public CSquadMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int	 ISoundMask( void );
	int  Classify ( void );

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );

	void domaboss_laser_fire( void );

	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	void DeathSound( void );

	void RunAI( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector( -240, -240, 0 );
		pev->absmax = pev->origin + Vector( 240, 240, 720 );
	}

	Schedule_t *GetSchedule( void );
	Schedule_t *GetScheduleOfType ( int Type );
	CUSTOM_SCHEDULES;

	void StartTask ( Task_t *pTask );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_teleportorigin;
	Vector m_teleportorigin2;
	Vector m_teleportorigin3;
	Vector m_speedvec;

	float m_flNextSkill1Time;
	float m_flNextSkill2Time;
	float m_flNextSkill3Time;
	float m_flNextSkill4Time;
	float m_flNextSkill5Time;
	float m_flKillBeamTime;
	float m_flFlyingTime;

	int doma_fire;
};
LINK_ENTITY_TO_CLASS( monster_doma_boss, CDoma );

TYPEDESCRIPTION	CDoma::m_SaveData[] = 
{
	DEFINE_FIELD( CDoma, m_teleportorigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CDoma, m_teleportorigin2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CDoma, m_teleportorigin3, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CDoma, m_flNextSkill1Time, FIELD_TIME ),
	DEFINE_FIELD( CDoma, m_flNextSkill2Time, FIELD_TIME ),
	DEFINE_FIELD( CDoma, m_flNextSkill3Time, FIELD_TIME ),
	DEFINE_FIELD( CDoma, m_flNextSkill4Time, FIELD_TIME ),
	DEFINE_FIELD( CDoma, m_flNextSkill5Time, FIELD_TIME ),
	DEFINE_FIELD( CDoma, m_flKillBeamTime, FIELD_TIME ),
	DEFINE_FIELD( CDoma, m_flFlyingTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CDoma, CSquadMonster );

enum
{
	SCHED_DOMA_REPEL = LAST_COMMON_SCHEDULE + 1,
	SCHED_DOMA_REPEL_ATTACK,
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CDoma :: Classify ( void )
{
	return	CLASS_ALIEN_MONSTER;
}

BOOL CDoma :: FCanCheckAttacks ( void )
{
	return TRUE;
}

void CDoma :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->movetype == MOVETYPE_NOCLIP){
		if(pev->deadflag == DEAD_NO){
			if(pev->frags == 0){
				if(pev->origin.z < m_teleportorigin2.z){
				pev->velocity.z = 128;
				}
				else{
				pev->velocity.z = 0;
				}

				if(pev->origin.y > m_teleportorigin3.y){
					m_speedvec = (m_teleportorigin3 - pev->origin).Normalize() * 128;
					pev->velocity.y = m_speedvec.y;
				}
				else{
					m_speedvec = (m_teleportorigin - pev->origin).Normalize() * 128;
					pev->velocity.y = m_speedvec.y;
				}
			}
			else{
				if(pev->origin.z > m_teleportorigin.z){
					pev->velocity.z = -1800;
				}
				else{
					UTIL_SetOrigin( pev, m_teleportorigin3);

					::RadiusDamage2( pev->origin + Vector(0,0,80) , pev, pev, 400, 1250, CLASS_ALIEN_MONSTER, DMG_ENERGYBLAST | DMG_CONCUSSION);
					FX_Explosion( pev->origin + Vector(0,0,80),146);
					EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "hydra/shock.wav", 1.0, 0.3, 0, 100 + RANDOM_LONG(-5,5) );

					pev->movetype = MOVETYPE_NONE;
					pev->frags = 0;
					pev->armorvalue = 0;
					pev->velocity.z = 0;

					if(m_teleportorigin3 != m_teleportorigin){
					m_teleportorigin3 = m_teleportorigin;
					}
					else{
					m_teleportorigin3 = pev->origin - Vector(0,3072,0);
					}
				}
			}
		}
		else{
			if(pev->origin.z > m_teleportorigin.z){
				pev->velocity.z = -1024;
			}
			else{
				UTIL_SetOrigin( pev, m_teleportorigin3);
			}
		}
	}

	if(m_flKillBeamTime > 0 && m_flKillBeamTime < gpGlobals->time){
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_KILLBEAM );
	WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
	MESSAGE_END();
	m_flKillBeamTime = 0;
	}
}


void CDoma :: domaboss_laser_fire ( void )
{
	Vector org,vecdir;
	GetAttachment( 0, org,vecdir);
	FX_Trail(org, entindex(), 142);

	m_HackedGunPos = org;

	if ( HasConditions( bits_COND_SEE_ENEMY ) ){
		CBaseEntity *pEnemy = m_hEnemy;
		if ( pEnemy )
		{//精准射击の加持
		m_vecEnemyLKP = pEnemy->pev->origin;
		}
	}

	Vector vecShootDir = ShootAtEnemy( m_HackedGunPos );

	UTIL_VecToAngles( vecShootDir );

	FireBullets(1, m_HackedGunPos, vecShootDir, g_vecZero, 16384, BULLET_DOMA_LASER,0);

	FireBeam(m_HackedGunPos, vecShootDir, 25, 100, pev);

	EMIT_SOUND_DYN( ENT(pev), CHAN_STREAM, "doma/doma_fire2.wav", 1, 0.3, 0, 100);
}

//=========================================================
// DieSound
//=========================================================

void CDoma :: DeathSound( void )
{

}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CDoma :: ISoundMask ( void) 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}


void CDoma::Killed( entvars_t *pevAttacker, int iGib )
{
	CSquadMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CDoma :: SetYawSpeed ( void )
{
	pev->yaw_speed = 60;
}


//=========================================================
// start task
//=========================================================
void CDoma :: StartTask ( Task_t *pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if ( pev->movetype == MOVETYPE_NOCLIP )
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;

	default: 
		CBaseMonster :: StartTask( pTask );
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CDoma :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	// ALERT( at_console, "event %d : %f\n", pEvent->event, pev->frame );
	switch( pEvent->event )
	{
		case 1:
		{
			pev->movetype = MOVETYPE_NOCLIP;
			pev->frags = 0;
			pev->armorvalue = 0;
			m_flNextSkill3Time = gpGlobals->time + 50.0;
			m_flNextSkill4Time = gpGlobals->time + 25.0;
		}
		break;

		case 2:
		{
			if(m_enemyget_mode == 1){
			m_enemyget_mode = 0;
			}
			else{
			m_enemyget_mode = 1;
			}

			if(m_flNextSkill5Time < gpGlobals->time && pev->health < pev->max_health * 0.8){
			Vector vecStart, angleGun;
			GetAttachment( 0, vecStart, angleGun );
			FX_Trail(pev->origin, entindex(), 148);
			pev->armorvalue = 1;
			m_flNextSkill5Time = gpGlobals->time + RANDOM_FLOAT( 10.0, 15.0 );
			}
			else{
			pev->armorvalue = 0;
			}

			m_flNextSkill1Time = gpGlobals->time + RANDOM_FLOAT( 6.0, 10.0 );
		}
		break;

		case 3:
		{
			if(m_hEnemy != NULL && pev->armorvalue == 0){
			Vector	vecSpitOffset;
			Vector	vecSpitDir;
			Vector  vangle;

			UTIL_MakeVectors ( pev->angles );

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			GetAttachment( 0, vecSpitOffset, vangle );

			vecSpitDir = ( m_hEnemy->Center() - vecSpitOffset ).Normalize();

			Vector Sog = vecSpitOffset + gpGlobals->v_forward * 4;
			// do stuff for this event.
			//AttackSound();

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY, Sog);
			WRITE_BYTE(3);
			WRITE_COORD( Sog.x);	// pos
			WRITE_COORD( Sog.y);	
			WRITE_COORD( Sog.z);
			WRITE_SHORT(doma_fire);
			WRITE_BYTE(15);
			WRITE_BYTE(15);
			WRITE_BYTE(4);
			MESSAGE_END();

			CDomaFireBall::Shoot( pev, vecSpitOffset, vecSpitDir * 1800 );
			}
			else{
				pev->armorvalue += 1;
				if(pev->armorvalue >= 6){
				domaboss_laser_fire();
				m_flNextSkill5Time = gpGlobals->time + RANDOM_FLOAT( 25.0, 30.0 );
				pev->armorvalue = 0;
				}
			}
		}
		break;

		case 4:
		{
			CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin, 2048, 0.3 );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 2 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 8 ); // life
			WRITE_BYTE( 8 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 255 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 3 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 8 ); // life
			WRITE_BYTE( 8 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 255 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 4 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 8 ); // life
			WRITE_BYTE( 8 );  // width
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 255 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			m_flKillBeamTime = gpGlobals->time + 1.8;
		}
		break;

		case 5:
		{
			if(m_hEnemy != NULL){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				
				Vector vecSrc	= pev->origin + 128;

				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
					if(tr.flFraction < 1.0 && (vecSrc - tr.vecEndPos).Length() <= 384){
						int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
						int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
						FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

						if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
						pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 1200;
						}

						FX_Explosion(pEntity->Center(), 47 );

						ClearMultiDamage( );
						pEntity->TraceAttack(pev, 400, gpGlobals->v_forward, &tr, DMG_CRUSH | DMG_CONCUSSION); 
						ApplyMultiDamage( pev, pev );
					}
				}
			}

			if(m_flNextSkill2Time < gpGlobals->time){
			::RadiusDamage2( pev->origin + Vector(0,0,80) , pev, pev, 400, 1250, CLASS_ALIEN_MONSTER, DMG_ENERGYBLAST | DMG_CONCUSSION);
			FX_Explosion( pev->origin + Vector(0,0,80),146);

			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "hydra/shock.wav", 1.0, 0.3, 0, 100 + RANDOM_LONG(-5,5) );

			m_flNextSkill2Time = gpGlobals->time + RANDOM_FLOAT( 8.0, 12.0 );
			}

		}
		break;

		case 6:
		{
			pev->frags = 1;
			pev->velocity.z = -1800;
		}
		break;

		case 7:
		{
			if(m_hEnemy != NULL){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				
				Vector vecSrc	= pev->origin;
				vecSrc.z = m_teleportorigin.z + 128;

				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) ){
					if(tr.flFraction < 1.0 && (vecSrc - tr.vecEndPos).Length() <= 384){
						int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
						int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
						FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

						if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
						{
						pEntity->pev->velocity = pEntity->pev->velocity + (pEntity->pev->origin - pev->origin).Normalize() * 1200;
						}

						FX_Explosion(pEntity->Center(), 47 );

						ClearMultiDamage( );
						pEntity->TraceAttack(pev, 400, gpGlobals->v_forward, &tr, DMG_CRUSH | DMG_CONCUSSION); 
						ApplyMultiDamage( pev, pev );
					}
				}
			}
		}
		break;

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckRangeAttack1 - normal beam attack 
//=========================================================
BOOL CDoma :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if (m_flNextSkill1Time > gpGlobals->time)
	{
		return FALSE;
	}
	
	return TRUE;
}

//=========================================================
// CheckRangeAttack2 - check bravery and try to resurect dead comrades
//=========================================================
BOOL CDoma :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if (m_flNextSkill2Time > gpGlobals->time || flDist >= 1024)
	{//Bug Fix 3.0 一直以来都没发现的隐藏bug，Doma攻击判定出错修复!
		return FALSE;
	}
	
	return TRUE;
}


//=========================================================
// Spawn
//=========================================================
void CDoma :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/doma_boss.mdl");
	UTIL_SetSize(pev, Vector( -160, -160, 0 ), Vector( 160, 160, 480 ));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	m_bloodColor		= DONT_BLEED;
	pev->effects		= 0;
	
	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 90000;
	}
	else{
	pev->health			= 75000;
	}

	pev->view_ofs		= Vector ( 0, 0, 64 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	pev->flags			|= FL_FLY;

	MonsterInit();

	pev->gravity		= 3.0;

	m_flNextSkill1Time = gpGlobals->time + 6.0;//火球
	m_flNextSkill2Time = gpGlobals->time + 3.0;//龙爪
	m_flNextSkill3Time = gpGlobals->time + 30.0;//飞行
	m_flNextSkill4Time = 0;//落地击
	m_flNextSkill5Time = 0;//蓄力击
	m_flKillBeamTime = 0;
	m_flFlyingTime = 0;

	m_ignoredamage = 1;
	m_longming	   = 1;

	m_selfmode = TRUE;
	pev->body = 0;

	m_aimenemy_mod = 6;

	m_killed_exp = 6000;
	m_is_the_boss = TRUE;
	m_rpgms_level = 120;
	pev->netname = MAKE_STRING( "Doma" );

	m_freeze_def = 2;//冻结抗性LV2

	m_singdelay_max = 0;//0反应
	m_singdelay_use = m_singdelay_max;

	m_facing_fucking_mode = 1;

	pev->effects		= EF_DIMLIGHT;

	m_teleportorigin = pev->origin;
	m_teleportorigin2 = pev->origin + Vector(0,0,1024);
	m_teleportorigin3 = pev->origin - Vector(0,3072,0);

	m_enemyget_mode = 1;//远距离!

	m_attack_dist = 1280;

	m_no_pov_limit = 1;
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CDoma :: Precache()
{
	PRECACHE_MODEL("models/doma_boss.mdl");
	PRECACHE_MODEL("models/doma.mdl");
	PRECACHE_MODEL("models/fire_bolt.mdl");
	
	PRECACHE_SOUND("majo/flame_hitwall.wav");
	PRECACHE_SOUND("doma/doma_fire2.wav");

	PRECACHE_SOUND ("hydra/shock.wav");

	doma_fire =  PRECACHE_MODEL("sprites/anim_spr9.spr");
}	


//=========================================================
// TakeDamage - get provoked when injured
//=========================================================

int CDoma :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if( (bitsDamageType & DMG_BLAST) || (bitsDamageType & DMG_ENERGYBLAST)
	|| (bitsDamageType & DMG_VALVE_SWORD) || (bitsDamageType & DMG_MORTAR)
	|| (bitsDamageType & DMG_CLUB) ){
		flDamage *= 1.5;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

void CDoma::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{//流血可能容易致命BUG出现？
	if ( pev->dmgtime != gpGlobals->time || (RANDOM_LONG(0,100) < 20))
	{
	pev->dmgtime = gpGlobals->time;
	UTIL_Sparks(ptr->vecEndPos);
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


DEFINE_CUSTOM_SCHEDULES( CDoma )
{
	slDomaRepel,
	slDomaRepelAttack,
	slDomaLeap,
	slDomaLandAttack,
};

IMPLEMENT_CUSTOM_SCHEDULES( CDoma, CSquadMonster );


//=========================================================
//=========================================================
Schedule_t *CDoma :: GetSchedule( void )
{

	if ( pev->movetype == MOVETYPE_NOCLIP )
	{
		if(m_flNextSkill4Time <= gpGlobals->time && m_freezetime == 0){
			return GetScheduleOfType( SCHED_ARM_WEAPON );//落地
		}

		// repel down a rope, 
		if ( m_MonsterState == MONSTERSTATE_COMBAT)
			return GetScheduleOfType ( SCHED_DOMA_REPEL_ATTACK );
		else
			return GetScheduleOfType ( SCHED_DOMA_REPEL );

	}
	else{
		if(pev->movetype != MOVETYPE_NOCLIP && m_flNextSkill3Time <= gpGlobals->time && m_freezetime == 0
		&& pev->health < pev->max_health * 0.8){
			return GetScheduleOfType( SCHED_COWER );//起飞
		}
	}

	return CBaseMonster :: GetSchedule();
}


Schedule_t *CDoma :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{

	case SCHED_DOMA_REPEL:
		{
			return &slDomaRepel[ 0 ];
		}

	case SCHED_DOMA_REPEL_ATTACK:
		{
			return &slDomaRepelAttack[ 0 ];
		}

	case SCHED_COWER:
		{
		return slDomaLeap;
		}
		break;

	case SCHED_ARM_WEAPON:
		{
		return slDomaLandAttack;
		}
		break;

	}
	return CSquadMonster :: GetScheduleOfType( Type );
}




