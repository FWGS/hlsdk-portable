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
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"
#include	"animation.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CPurpleguy : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	void RunAI( void );

	float m_flyingtime;
	float m_flNextSkillTime;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
};

LINK_ENTITY_TO_CLASS( monster_purple_guy, CPurpleguy );

TYPEDESCRIPTION	CPurpleguy::m_SaveData[] = 
{
	DEFINE_FIELD( CPurpleguy, m_flNextSkillTime, FIELD_TIME ),
	DEFINE_FIELD( CPurpleguy, m_flyingtime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CPurpleguy, CBaseMonster );


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CPurpleguy :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

void CPurpleguy :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_RUN )
	|| pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
	m_flGroundSpeed = 360;
	}

	if(pev->movetype == MOVETYPE_FLY){
		if(m_flyingtime > gpGlobals->time && m_freezetime == 0){
			UTIL_MakeVectors ( pev->angles );
			pev->velocity = gpGlobals->v_forward * 3000;
		}
		else{
			pev->movetype = MOVETYPE_STEP;
			m_flyingtime = 0;
		}
	}

	CBaseMonster :: RunAI();
}

BOOL CPurpleguy :: CheckRangeAttack1 ( float flDot, float flDist )
{
	float dist = 640;

	if(m_flNextSkillTime > gpGlobals->time){
	return FALSE;
	}
	
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= dist && flDot >= 0.5 && flDist >= 128)
	{
	return TRUE;
	}

	return FALSE;
}


BOOL CPurpleguy :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if(pev->movetype == MOVETYPE_FLY){
	return FALSE;
	}

	float dist = 128;
	int height_attack = 0;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 96 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 64;
			height_attack = 1;
			}
		}
	}

			if (flDist <= dist && m_hEnemy != NULL)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence != LookupActivity ( ACT_RANGE_ATTACK1 ) &&
					pev->sequence != LookupActivity ( ACT_RUN_SCARED ) && flDot >= 0.5){
						pev->sequence = LookupActivity ( ACT_RUN_SCARED );
						ResetSequenceInfo( );
						pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 60 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED ) ){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

Schedule_t* CPurpleguy :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CPurpleguy :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CPurpleguy :: SetYawSpeed ( void )
{
	pev->yaw_speed = 300;
}

void CPurpleguy::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == 1)
	{
		if ( pev->dmgtime != gpGlobals->time)
		{
		pev->dmgtime = gpGlobals->time;
		UTIL_WhiteSparks( ptr->vecEndPos, ptr->vecPlaneNormal, 9, 5, 5, 100 );//puntos
		}
		flDamage *= 0.75;//ͷ����Ӳ�����˺�ֻ��1.5��
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CPurpleguy :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( (bitsDamageType & DMG_SHOCK) ){
	flDamage *= 0.8;//�׵翹��
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CPurpleguy::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CPurpleguy :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg1,dmg2,dmg3;
	dmg1 = 18;//�ҳ�ȭ
	dmg2 = 45;//��ͨȭ
	dmg3 = 90;//ħ��ȭ

	switch( pEvent->event )
	{
		case 1:
		{
		
			if(m_hEnemy != NULL){
				
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				if(( vecSrc - tr.vecEndPos).Length() <= 100){
					if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg1, gpGlobals->v_forward, &tr, DMG_SLASH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
			
		}
		break;

		case 2:
		case 3:
		{
		
			if(m_hEnemy != NULL){
				
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
				if(( vecSrc - tr.vecEndPos).Length() <= 120){
					if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg2, gpGlobals->v_forward, &tr, DMG_SLASH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "newadd/fist_hitbod2.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
			
		}
		break;

		case 4:
		{
			FX_Explosion( Center(), 108 );
			::RadiusDamage2( Center(), pev, pev, 120, 360, CLASS_NONE, DMG_BLAST);

			SetThink ( &CPurpleguy::SUB_Remove );
			pev->nextthink = gpGlobals->time;

			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_EXPLOSION);
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );
				WRITE_SHORT( g_sModelIndexFireball );
				WRITE_BYTE( 0 ); // no sprite
				WRITE_BYTE( 15  ); // framerate
				WRITE_BYTE( TE_EXPLFLAG_NONE );
			MESSAGE_END();

		}
		break;

		case 5://���2
		{
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_BEAMFOLLOW );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			WRITE_SHORT(g_sModelIndexTrail );	// model
			WRITE_BYTE( 6 ); // life
			WRITE_BYTE( 6 );  // width
			WRITE_BYTE( 128 );	// R
			WRITE_BYTE( 0 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 192 );	// brightness
			MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

			pev->movetype = MOVETYPE_FLY;
			m_flyingtime = gpGlobals->time + 0.5;
			m_flNextSkillTime = gpGlobals->time + 8;
		}
		break;

		case 6:
		{
			Vector vecGunPos,vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );

			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget(pev->origin);
			Vector vecEnd	= vecGunPos;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
			if ( tr.flFraction < 1.0 ){
			vecGunPos = tr.vecEndPos + (tr.vecPlaneNormal * 4);
			}
			else{
			vecGunPos = tr.vecEndPos;
			}

			FX_Explosion( vecGunPos, 131);
			::RadiusDamage_limit( vecGunPos, pev, pev, dmg3, 150, CLASS_HUMAN_ASS, DMG_ENERGYBEAM | DMG_CONCUSSION);
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/froster_firegrenade.wav", 1.0, 0.6, 0, 100 + RANDOM_LONG(-5,5) );
			pev->movetype = MOVETYPE_STEP;
			m_flyingtime = 0;

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_KILLBEAM );
			WRITE_SHORT( entindex() + 0x1000 * 1 );		// entity, attachment
			MESSAGE_END();
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
void CPurpleguy :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/purple_guy.mdl");
	UTIL_SetSize(pev, Vector(-16,-16,0), Vector(16,16,96));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 960;
	}
	else{
	pev->health			= 800;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_chase_mode = 1;
	m_headdef	 = 2;//ͷ��Ӳ��

	pev->skin = 0;

	m_ignoredamage		= 1;
	pev->gravity		= 1.5;

	m_ignoreFail_MAX = 30;
	m_ignoreFail_OFF = 0;
	m_forcefuckdoor  = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_killed_exp = 200;
	m_rpgms_level = 60;
	pev->netname = MAKE_STRING( "Purple.Guy" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CPurpleguy :: Precache()
{
	PRECACHE_MODEL("models/purple_guy.mdl");
	PRECACHE_SOUND("weapons/froster_firegrenade.wav");
}	
