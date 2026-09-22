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

class CUnknowMs2 : public CBaseMonster
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


	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	void SetActivity ( Activity NewActivity );

	void RunAI( void );

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_else_rabbit, CUnknowMs2 );



void CUnknowMs2 :: RunAI( void )
{
	CBaseMonster :: RunAI();

	if(pev->skin == 1 && pev->sequence == LookupActivity ( ACT_RUN )){
	m_flGroundSpeed = 240;
	}
	else if(pev->skin == 1 && pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
	m_flGroundSpeed = 220;
	}
	else if(pev->skin == 0 && pev->sequence == LookupActivity ( ACT_WALK )){
	m_flGroundSpeed = 80;
	}

	if(pev->skin == 1 && pev->frags != 1){
		pev->frags = 1;
	
		if (g_iSkillLevel == SKILL_HARD){
		pev->health			= 220;
		}
		else{
		pev->health			= 180;
		}

		pev->max_health		= pev->health;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CUnknowMs2 :: Classify ( void )
{
	return	CLASS_HUMAN_ASS;
}

BOOL CUnknowMs2 :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// flying?
	if ( pev->movetype == MOVETYPE_TOSS)
	{
		if (pev->flags & FL_ONGROUND)
		{
			pev->movetype = MOVETYPE_STEP;
		}
	}

	float dist = 80;
	if(pev->skin == 0){
	dist = 64;
	}
	else if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) > 60 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
			dist += 60;
			}
		}
	}

			if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
			{
				if (m_hEnemy->IsAlive() ){
					if(pev->skin == 1 && pev->sequence == LookupActivity ( ACT_RUN )){
					pev->sequence = LookupActivity ( ACT_RUN_SCARED );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
					else if(pev->skin != 1){
					return TRUE;
					}
				}
			}
			else if (flDist >= dist + 45 )
			{
					if(pev->sequence == LookupActivity ( ACT_RUN_SCARED )){
					pev->sequence = LookupActivity ( ACT_RUN );
					ResetSequenceInfo( );
					pev->frame = 0;
					}
			}

	return FALSE;
}

void CUnknowMs2::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

Schedule_t* CUnknowMs2 :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CUnknowMs2 :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CUnknowMs2 :: SetYawSpeed ( void )
{
	pev->yaw_speed = 120;
}

int CUnknowMs2 :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if(pev->health == pev->max_health){
	ClearSchedule();//快速反应
	m_alert = 100;
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CUnknowMs2::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}



//=========================================================
// SetActivity 
//=========================================================
void CUnknowMs2 :: SetActivity ( Activity NewActivity )
{
	int	iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	switch ( NewActivity)
	{
	case ACT_RUN:
		if ( pev->skin != 1 )
		{
			// limp!
			iSequence = LookupActivity ( ACT_WALK );
		}
		else{
			iSequence = LookupActivity ( NewActivity );
		}
		break;
	default:
		iSequence = LookupActivity ( NewActivity );
		break;
	}
	
	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}

		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity );
	//	pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CUnknowMs2 :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;

	dmg = 20;

	switch( pEvent->event )
	{
		case 1:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 72;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 72, dmg, DMG_SLASH | DMG_NEVERGIB );
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 234 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 235 );
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else if(m_hEnemy != NULL && m_fightmode == 1){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 75){
					m_hEnemy->TakeDamage( pev, pev, dmg, DMG_SLASH | DMG_NEVERGIB );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}


			
		}
		break;

		case 2:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 72;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 72, dmg, DMG_SLASH | DMG_NEVERGIB );
				if(pHurt){
					if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
					|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
					|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
					FX_Explosion( pHurt->Center(), 234 );
					}
					else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
					FX_Explosion( pHurt->Center(), 235 );
					}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else if(m_hEnemy != NULL && m_fightmode == 1){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 75){
					m_hEnemy->TakeDamage( pev, pev, dmg, DMG_SLASH | DMG_NEVERGIB );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}


			
		}
		break;

		case 3:
		{
			TraceResult tr;
			UTIL_MakeVectors(pev->angles);
			Vector vecSrc	= BodyTarget_c(pev->origin);
			Vector vecEnd	= vecSrc + gpGlobals->v_forward * 72;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

			if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
			int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
			int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
			FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, dmg * 2, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
			ApplyMultiDamage( pev, pev );
			EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else{
				CBaseEntity *pHurt = CheckTraceHullAttack( 72, dmg * 2, DMG_SLASH | DMG_NEVERGIB );
				if(pHurt){
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) && pHurt->pev->takedamage ){

						if (pHurt->Classify() == CLASS_PLAYER || pHurt->Classify() == CLASS_PLAYER_ALLY
						|| pHurt->Classify() == CLASS_HUMAN_ASS || pHurt->Classify() == CLASS_HUMAN_PASSIVE
						|| pHurt->Classify() == CLASS_HUMAN_MILITARY){
						FX_Explosion( pHurt->Center(), 234 );
						}
						else if (pHurt->Classify() == CLASS_ALIEN_MONSTER || pHurt->Classify() == CLASS_ALIEN_MILITARY){
						FX_Explosion( pHurt->Center(), 235 );
						}
						EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
				else if(m_hEnemy != NULL && m_fightmode == 1){
					if(( pev->origin - m_hEnemy->pev->origin).Length() <= 72){
					m_hEnemy->TakeDamage( pev, pev, dmg * 2, DMG_SLASH | DMG_NEVERGIB );
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, "weapons/knife_hitbody.wav", 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
					}
				}
			}


			
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
void CUnknowMs2 :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/biohomo_rbzombie.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_TOSS;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 180;
	}
	else{
	pev->health			= 150;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	m_ignoredamage		= 2;

	m_forcefuckdoor  = TRUE;
	m_selfmode = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	if(pev->skin == 1){
	m_killed_exp = 50;
	}
	else{
	m_killed_exp = 25;
	}
	m_rpgms_level = 25;

	m_chase_mode = 2;
	m_chase_failed_max = 4;
	pev->netname = MAKE_STRING( "Rabbit.Man" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CUnknowMs2 :: Precache()
{
	PRECACHE_MODEL("models/biohomo_rbzombie.mdl");
}	
