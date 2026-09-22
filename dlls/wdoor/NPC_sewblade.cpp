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

Task_t	tlSewBlade_wakeup[] =
{
	{ TASK_STOP_MOVING,					0				 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_LAND },
};

Schedule_t slSewBlade_wakeup[] = 
{
	{
		tlSewBlade_wakeup,
		ARRAYSIZE ( tlSewBlade_wakeup ),
		0,
		0,
		"Sew Wake Up"
	}
};


class CSewBlade : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	int IgnoreConditions ( void );
	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void RunAI( void );

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckMeleeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	CUSTOM_SCHEDULES;
};

const char *CSewBlade::pAttackSounds[] = 
{
	"cof/sewmo_attack1.wav",
	"cof/sewmo_attack2.wav",
};

const char *CSewBlade::pAlertSounds[] = 
{
	"cof/sewmo_alert10.wav",
	"cof/sewmo_alert20.wav",
	"cof/sewmo_alert30.wav",
};

const char *CSewBlade::pPainSounds[] = 
{
	"cof/sewmo_pain1.wav",
	"cof/sewmo_pain2.wav",
};

LINK_ENTITY_TO_CLASS( monster_sewblade, CSewBlade );

TYPEDESCRIPTION	CSewBlade::m_SaveData[] = 
{
	DEFINE_FIELD( CSewBlade, m_fGunDrawn, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CSewBlade, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( CSewBlade )
{
	slSewBlade_wakeup,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSewBlade, CBaseMonster );
//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CSewBlade :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CSewBlade :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	if(pev->body == 0){
			if (flDist <= 100 && m_hEnemy != NULL && flDot >= 0.5)
			{
				return TRUE;
			}
	}
	else{
			float dist = 90;
			if(m_hEnemy != NULL){
				if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
				{
					if (m_hEnemy->pev->flags & FL_ONGROUND)
					{
						if( (pev->flags & FL_ONGROUND) && pev->velocity.Length() <= 150){
						pev->velocity.x += RANDOM_LONG(-600,600);
						pev->velocity.y += RANDOM_LONG(-600,600);
						}
						dist += 60;
					}
				}
			}

			if (flDist <= dist && m_hEnemy != NULL)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->sequence == LookupActivity ( ACT_WALK )){
					pev->sequence = LookupActivity ( ACT_WALK_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
				}
			}
			else if (flDist >= dist + 90 )
			{
					if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
					pev->sequence = LookupActivity ( ACT_WALK );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}
	}

	return FALSE;
}

void CSewBlade::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CSewBlade :: GetScheduleOfType ( int Type )
{
	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slSewBlade_wakeup;
		}
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CSewBlade :: GetSchedule ( void )
{
	if (!m_fGunDrawn && (m_hEnemy != NULL || pev->health <= 0 || m_allydeadcheck == 1) ){
		return GetScheduleOfType( SCHED_ARM_WEAPON );
	}
	else if (pev->body == 0 && pev->health <= pev->max_health * 0.5){
		return GetScheduleOfType( SCHED_COWER );
	}

	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CSewBlade :: SetYawSpeed ( void )
{
	if(m_fGunDrawn){
	pev->yaw_speed = 180;
	}
	else{
	pev->yaw_speed = 0;
	}
}

int CSewBlade :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->sequence == LookupActivity ( ACT_FALL )
	|| pev->sequence == LookupActivity ( ACT_LAND )
	|| pev->sequence == LookupActivity ( ACT_COWER )){
	flDamage *= 0.5;//¼õÉË!
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CSewBlade::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );

	if(pev->body == 0 && m_gibed == 0){
	pev->body = 1;
	EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/slime_blast1.wav", 1, ATTN_NORM);		
	FX_Explosion(Center(), 239 );
	::RadiusDamage2(pev->origin, pev, pev, 80, 160, CLASS_HUMAN_ASS, DMG_BLOOD);
	}
}

void CSewBlade :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if ( m_flNextPainTime > gpGlobals->time )
	{
		return;
	}

	m_flNextPainTime = gpGlobals->time + 1.0;

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CSewBlade :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CSewBlade :: IdleSound( void )
{

}

void CSewBlade :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CSewBlade :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 30;

	switch( pEvent->event )
	{
		case 1:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 130){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					Vector vecSrc	= BodyTarget(pev->origin);
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}

			AttackSound();
		}
		break;

		case 2:
		{
			if(m_hEnemy != NULL){
				if(( pev->origin - m_hEnemy->pev->origin).Length() <= 110){
					TraceResult tr;
					UTIL_MakeVectors(pev->angles);
					Vector vecSrc	= BodyTarget(pev->origin);
					Vector vecEnd	= m_hEnemy->Center();
					UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
					CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

					if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}

			AttackSound();
		}
		break;

		case 3:
		{
			EMIT_SOUND(ENT(pev), CHAN_BODY, "newadd/slime_blast1.wav", 1, ATTN_NORM);		
			FX_Explosion(Center(), 239 );

			::RadiusDamage2(pev->origin, pev, pev, 80, 160, CLASS_HUMAN_ASS, DMG_BLOOD);

			pev->body = 1;

			if(!m_fGunDrawn){
				m_crouchmode        = 0;
				m_fGunDrawn         = TRUE;
				m_flDistLook		= 4096;
				m_flFieldOfView		= 0.5;
				pev->frags			= 0;
				pev->flags		   &= ~FL_FROZEN;
				m_facing_fucking_mode = 1;
				pev->takedamage = DAMAGE_AIM;
				if(pev->health < 1){
				pev->health = 1;
				}
			}
		}
		break;

		case 5:
			{
				m_crouchmode        = 0;
				m_fGunDrawn         = TRUE;
				m_flDistLook		= 4096;
				m_flFieldOfView		= 0.5;
				pev->frags			= 0;
				pev->flags		   &= ~FL_FROZEN;
				m_facing_fucking_mode = 1;
				pev->takedamage = DAMAGE_AIM;
				if(pev->health < 1){
				pev->health = 1;
				}
			}
			break;

		case 4:
			{
			TraceResult tr;
			UTIL_TraceLine(Center(), pev->origin, ignore_monsters, ENT(pev), &tr);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, Center(), 0, 191, 0 );
		//	EMIT_SOUND_DYN ( ENT(pev), CHAN_BODY, "player/water_small_splash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}


void CSewBlade :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if (pev->body == 0 && pev->health > 0 && pev->health <= pev->max_health * 0.5){
		if(pev->armortype == 0){
		ClearSchedule();
		pev->armortype = 1;
		}
	}

	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
		if(pev->body == 0){
		m_flGroundSpeed = 180;
		}
		else{
		m_flGroundSpeed = 270;
		}
	}
}

//=========================================================
// Spawn
//=========================================================
void CSewBlade :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/sewblade.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 600;
	}
	else{
	pev->health			= 500;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();

	m_ignoredamage = 1;

	if(pev->frags == 1){
	m_crouchmode        = 1;
	m_fGunDrawn         = FALSE;
	m_flFieldOfView		= 0;
	m_flDistLook        = 180;
	pev->flags			|= FL_FROZEN;
	pev->takedamage		 = DAMAGE_YES;
	m_singdelay_max		 = 1;
	}
	else{
	m_crouchmode        = 0;
	m_fGunDrawn         = TRUE;
	m_flFieldOfView		= 0.5;
	}

	m_ignoreFail_MAX = 30;
	m_ignoreFail_OFF = 0;
	m_MoveFail_SimpleRoad = TRUE;
	m_MoveFail_FuckRoad = TRUE;
	m_killed_exp = 100;
	m_rpgms_level = 55;

	pev->gravity         = 1.5;

	pev->netname = MAKE_STRING( "Sew.Blade" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CSewBlade :: Precache()
{
	PRECACHE_MODEL("models/sewblade.mdl");

	PRECACHE_SOUND("newadd/slime_blast1.wav" );

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

int CSewBlade::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	return iIgnore;
}