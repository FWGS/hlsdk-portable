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

class CUnknowMs3 : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	float m_flNextFlinch;
	float m_flNextPainTime;

	void DeathSound( void );

	void Killed( entvars_t *pevAttacker, int iGib );

	Schedule_t *GetScheduleOfType ( int Type );
	Schedule_t *GetSchedule ( void );

	int m_hiderec;

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void RunAI( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_else_ghost, CUnknowMs3 );



void CUnknowMs3 :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 350;
	}

	if(pev->deadflag != DEAD_NO){
	pev->effects = 0;
	pev->solid = SOLID_SLIDEBOX;
	}

	if(m_alert == 0 && m_hEnemy == NULL && m_MonsterState == MONSTERSTATE_IDLE){
	SetState( MONSTERSTATE_ALERT );
	m_alert = 100;//时刻保持警惕!
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CUnknowMs3 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CUnknowMs3 :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL && pev->deadflag == DEAD_NO)
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( pEnemy )
		{
			if(pEnemy->FInViewCone( this ) && FVisible( pEnemy ) ){
				pev->effects = EF_MUZZLEFLASH;
				m_hiderec += 1;
				if(m_hiderec > 4){
				m_hiderec = 4;
				pev->effects |= EF_NODRAW;
				pev->solid = SOLID_NOT;
				RouteClear();
				SetActivity ( ACT_IDLE );
				pev->flags |= FL_NOTARGET;
				}
				return FALSE;
			}
		}
	}
	
	if (pev->effects & EF_NODRAW){
		pev->effects &= ~EF_NODRAW;
		pev->solid = SOLID_SLIDEBOX;
		pev->flags &= ~FL_NOTARGET;
		m_hiderec = 0;
	}

	float dist = 70;
	if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
	{
		return TRUE;
	}
		

	return FALSE;
}

void CUnknowMs3::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CUnknowMs3 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CUnknowMs3 :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CUnknowMs3 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 360;
}

int CUnknowMs3 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CUnknowMs3::Killed( entvars_t *pevAttacker, int iGib )
{
	pev->effects = 0;
	pev->solid = SOLID_SLIDEBOX;
	CBaseMonster::Killed( pevAttacker, iGib );
}



//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CUnknowMs3 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 45;

	switch( pEvent->event )
	{
		case 1:
		{

			if(m_hEnemy != NULL){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				
				Vector vecSrc	= Center();
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 ){
					if(( vecSrc - tr.vecEndPos).Length() <= 110){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					if ( pEntity->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
					pEntity->pev->velocity.z += 150;
					}

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH); 
					ApplyMultiDamage( pev, pev );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "tyant_boss/slash.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}
			
		}
		break;


		case 2:
		{
			EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, "newadd/ghost_alert.wav", 1.0, ATTN_NORM, 0, 100 );
		}
		break;

		case 3:
		{
			EMIT_SOUND_DYN( ENT(pev), CHAN_BODY, "newadd/ghost_step.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG(-10,10));
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
void CUnknowMs3 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/ghost.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 100;
	}
	else{
	pev->health			= 90;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_ignoredamage = 1;
	m_hiderec = 0;

	m_forcefuckdoor  = TRUE;

	m_selfmode = TRUE;
	m_killed_exp = 150;
	m_rpgms_level = 60;

	m_singdelay_max = 1;//快速反应
	m_singdelay_use = m_singdelay_max;
	pev->netname = MAKE_STRING( "Unknow.Ghost" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CUnknowMs3 :: Precache()
{
	PRECACHE_MODEL("models/ghost.mdl");
	PRECACHE_SOUND ("newadd/ghost_alert.wav");
	PRECACHE_SOUND ("newadd/ghost_die.wav");
	PRECACHE_SOUND ("newadd/ghost_step.wav");
	PRECACHE_SOUND ("tyant_boss/slash.wav");
}	



//=========================================================
// DeathSound 
//=========================================================
void CUnknowMs3 :: DeathSound ( void )
{
	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "newadd/ghost_die.wav", 1, ATTN_NORM, 0, 100);
	pev->effects = 0;
	pev->solid = SOLID_SLIDEBOX;
}
