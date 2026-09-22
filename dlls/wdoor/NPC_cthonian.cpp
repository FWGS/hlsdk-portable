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
#include	"player.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class Ccthonian : public CBaseMonster
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

	void AlertSound( void );
	void AttackSound( void );

	void SetObjectCollisionBox( void )
	{
		pev->absmin = pev->origin + Vector(-72, -72, 0);
		pev->absmax = pev->origin + Vector(72, 72, 160);
	}

	static const char *pAttackSounds[];
	static const char *pAlertSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

};

LINK_ENTITY_TO_CLASS( monster_cthonian, Ccthonian );

const char *Ccthonian::pAttackSounds[] = 
{
	"cof/bc_attack1.wav",
};

const char *Ccthonian::pAlertSounds[] = 
{
	"cof/bc_attackgrowl.wav",
	"cof/bc_attackgrowl2.wav",
	"cof/bc_attackgrowl3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	Ccthonian :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

void Ccthonian :: RunAI( void )
{
	if(pev->sequence == LookupActivity ( ACT_WALK )
	|| pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
	m_flGroundSpeed = 300;
	}

	CBaseMonster :: RunAI();
}


BOOL Ccthonian :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	float dist = 192;
	if(m_hEnemy != NULL){
		if ( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) >= 96 )
		{
			if (m_hEnemy->pev->flags & FL_ONGROUND)
			{
				dist += 64;
				if( (pev->flags & FL_ONGROUND) && pev->velocity.Length() <= 150){
				pev->velocity.x += RANDOM_LONG(-600,600);
				pev->velocity.y += RANDOM_LONG(-600,600);
				}
			}
		}
	}

	if (flDist <= dist && m_hEnemy != NULL && flDot >= 0.5)
	{
			if (m_hEnemy->IsAlive() ){
				if(pev->sequence == LookupActivity ( ACT_WALK )){
				pev->sequence = LookupActivity ( ACT_WALK_SCARED );
				ResetSequenceInfo( );
				pev->frame = 0;
				}
			}
	}
	else if (flDist >= dist + 64 )
	{
			if(pev->sequence == LookupActivity ( ACT_WALK_SCARED )){
			pev->sequence = LookupActivity ( ACT_WALK );
			ResetSequenceInfo( );
			pev->frame = 0;
			}
	}

	return FALSE;
}

Schedule_t* Ccthonian :: GetScheduleOfType ( int Type )
{
	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *Ccthonian :: GetSchedule ( void )
{
	return CBaseMonster::GetSchedule();
}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void Ccthonian :: SetYawSpeed ( void )
{
	pev->yaw_speed = 180;
}

int Ccthonian :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void Ccthonian::Killed( entvars_t *pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void Ccthonian :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void Ccthonian :: AttackSound( void )
{
	int iPitch = RANDOM_FLOAT( 90, 110 );
	switch ( RANDOM_LONG( 0, 1 ) )
	{
		case 0:
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 1, ATTN_NORM, 0, iPitch );	
			break;
		case 1:
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 1, ATTN_NORM, 0, iPitch );	
			break;
	}
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void Ccthonian :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	int dmg;
	dmg = 50;//深渊巨口

	switch( pEvent->event )
	{
		case 2:
		{
			if(m_hEnemy != NULL){
				TraceResult tr;
				UTIL_MakeVectors(pev->angles);
				Vector vecSrc	= BodyTarget(pev->origin);
				Vector vecEnd	= m_hEnemy->Center();
				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );
				CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

				if ( tr.flFraction < 1.0 && pEntity->pev->takedamage ){
					if(( tr.vecEndPos - vecSrc).Length() <= 96){
					int tex = (int)TEXTURETYPE_Trace(&tr, vecSrc, vecEnd);
					int surface = (int)SURFACETYPE_Trace(&tr, vecSrc, vecEnd,Classify(),1);
					FX_ImpBullet( tr.vecEndPos, tr.vecPlaneNormal, vecSrc, surface, BULLET_CROWBAR, (float)tex );

					ClearMultiDamage( );
					pEntity->TraceAttack(pev, dmg, gpGlobals->v_forward, &tr, DMG_SLASH | DMG_NEVERGIB ); 
					ApplyMultiDamage( pev, pev );

					TakeHealth(25, DMG_GENERIC);//吸血攻击

					AttackSound();
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
void Ccthonian :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/cthonian.mdl");
	UTIL_SetSize( pev, Vector(-32,-32,0), Vector(32,32,96) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_YELLOW;

	if (g_iSkillLevel == SKILL_HARD){
	pev->health			= 420;
	}
	else{
	pev->health			= 350;
	}

	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	
	MonsterInit();

	pev->skin = 0;

	m_ignoredamage		= 1;

	m_MoveFail_FuckRoad = TRUE;
	m_MoveFail_SimpleRoad = TRUE;
	m_ignoreFail_OFF = 0;
	m_ignoreFail = 1919;//暴走の模式

	m_killed_exp = 125;
	m_rpgms_level = 45;

	pev->friction = 0.5;
	pev->gravity = 1.5;

	pev->netname = MAKE_STRING( "Cthonian" );
}
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void Ccthonian :: Precache()
{
	PRECACHE_MODEL("models/cthonian.mdl");

	PRECACHE_SOUND ("bullchicken/bc_bite2.wav");
	PRECACHE_SOUND ("bullchicken/bc_bite3.wav");

	int i;

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);
}	
