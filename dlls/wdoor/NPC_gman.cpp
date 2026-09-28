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
// GMan - misunderstood servant of the people
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGMan : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int ISoundMask ( void );

	int	Save( CSave &save ); 
	int Restore( CRestore &restore );
	static TYPEDESCRIPTION m_SaveData[];

	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );

	EHANDLE m_hTalkTarget;
	float m_flTalkTime;
};
LINK_ENTITY_TO_CLASS( monster_gman, CGMan );
LINK_ENTITY_TO_CLASS( monster_gman2, CGMan );

TYPEDESCRIPTION	CGMan::m_SaveData[] = 
{
	DEFINE_FIELD( CGMan, m_hTalkTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CGMan, m_flTalkTime, FIELD_TIME ),
};
IMPLEMENT_SAVERESTORE( CGMan, CBaseMonster );


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGMan :: Classify ( void )
{
	return	CLASS_NONE;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGMan :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGMan :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case 1:
		{
			CBaseEntity *pEntity = UTIL_FindEntityByClassname( NULL, "monster_cleaner" );
			if ( pEntity )//对Kadoma特攻
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "debris/beamstart2.wav", 1, 0.7);	
					
				int teleb = PRECACHE_MODEL("sprites/b-tele1.spr");
				MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pEntity->pev->origin);
				WRITE_BYTE(3);
				WRITE_COORD( pEntity->pev->origin.x );
				WRITE_COORD( pEntity->pev->origin.y );
				WRITE_COORD( pEntity->pev->origin.z + 18);
				WRITE_SHORT(teleb);
				WRITE_BYTE(15);
				WRITE_BYTE(15);
				WRITE_BYTE(4);
				MESSAGE_END();

				UTIL_Remove( pEntity );
			}

			CBaseEntity *pEntity2 = UTIL_FindEntityByClassname( NULL, "monster_hydra_boss" );
			if ( pEntity2 )//对Hydra特攻
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "debris/beamstart2.wav", 1, 0.7);	
					
				int teleb = PRECACHE_MODEL("sprites/b-tele1.spr");
				MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pEntity2->pev->origin);
				WRITE_BYTE(3);
				WRITE_COORD( pEntity2->pev->origin.x - 128 );
				WRITE_COORD( pEntity2->pev->origin.y - 128 );
				WRITE_COORD( pEntity2->pev->origin.z + 128);
				WRITE_SHORT(teleb);
				WRITE_BYTE(75);
				WRITE_BYTE(15);
				WRITE_BYTE(4);
				MESSAGE_END();

				UTIL_Remove( pEntity2 );
			}
		}
		break;

		case 2:
		{
				//自身传送
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "debris/beamstart2.wav", 1, 0.7);	
					
				int teleb = PRECACHE_MODEL("sprites/b-tele1.spr");
				MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY,pev->origin);
				WRITE_BYTE(3);
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z + 36);
				WRITE_SHORT(teleb);
				WRITE_BYTE(15);
				WRITE_BYTE(15);
				WRITE_BYTE(4);
				MESSAGE_END();

				UTIL_Remove( this );
		}
		break;

		case 3://掏出针筒
		{
			SetBodygroup( 2, 1 );
		}
		break;

		case 5://渐变消失!
		{
			//SUB_StartFadeOut();
			pev->renderfx = kRenderFxExplode;
			pev->rendercolor.x = 255;
			pev->rendercolor.y = 255;
			pev->rendercolor.z = 255;
			FX_Explosion( Center(), 127);
			EMIT_SOUND_DYN ( ENT(pev), CHAN_STREAM, "newadd/exp2_frost.wav", 1.0, 0.6, 0, 100);
		}
		break;

		case 6://治疗身体
		{
			Vector	vecGunPos;
			Vector	vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			SetBodygroup( 0, 0 );
			FX_Explosion( vecGunPos, 44);
			FX_Explosion( vecGunPos, 45);
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "rmxp/136-Light02.wav", VOL_NORM, ATTN_NORM); 
			SetBits( pev->effects, EF_DIMLIGHT);
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGMan :: ISoundMask ( void )
{
	return	NULL;
}

//=========================================================
// Spawn
//=========================================================
void CGMan :: Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), "models/gman.mdl" );
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= DONT_BLEED;
	pev->health			= 100;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	if(pev->weapons == 1){
	pev->solid			= SOLID_NOT;
	pev->takedamage		= DAMAGE_NO;
	pev->movetype		= MOVETYPE_NONE;
	pev->effects	    |= EF_NODRAW;
	}
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGMan :: Precache()
{
	PRECACHE_MODEL( "models/gman.mdl" );
	PRECACHE_MODEL("sprites/b-tele1.spr");
	PRECACHE_SOUND("debris/beamstart2.wav" );

	UTIL_PrecacheOther( "monster_gman_boss" );
}	


//=========================================================
// AI Schedules Specific to this monster
//=========================================================


void CGMan :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT:
		if (m_hPlayer == NULL)
		{
			m_hPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		}
		break;
	}
	CBaseMonster::StartTask( pTask );
}

void CGMan :: RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_WAIT:
		// look at who I'm talking to
		if (m_flTalkTime > gpGlobals->time && m_hTalkTarget != NULL)
		{
			float yaw = VecToYaw(m_hTalkTarget->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			// turn towards vector
			SetBoneController( 0, yaw );
		}
		// look at player, but only if playing a "safe" idle animation
		else if (m_hPlayer != NULL && pev->sequence == 0)
		{
			float yaw = VecToYaw(m_hPlayer->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			// turn towards vector
			SetBoneController( 0, yaw );
		}
		else 
		{
			SetBoneController( 0, 0 );
		}
		CBaseMonster::RunTask( pTask );
		break;
	default:
		SetBoneController( 0, 0 );
		CBaseMonster::RunTask( pTask );
		break;
	}
}


//=========================================================
// Override all damage
//=========================================================
int CGMan :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	return 0;
}


void CGMan::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	UTIL_Ricochet( ptr->vecEndPos, 1.0 );
	return;
}


void CGMan::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	CBaseMonster::PlayScriptedSentence( pszSentence, duration, volume, attenuation, bConcurrent, pListener );

	m_flTalkTime = gpGlobals->time + duration;
	m_hTalkTarget = pListener;
}
