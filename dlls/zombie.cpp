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
#include	"player.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

#define ZOMBIE_FLINCH_DELAY		2		// at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	BOOL IsBlood ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int IgnoreConditions( void );
	Schedule_t* GetScheduleOfType(int Type);
	int BuckshotCount;
	BOOL HeadGibbed;
	Vector HeadPos;
	CBasePlayer* pBeheader;

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void CheckHeadAttacker( entvars_t *pevAttacker );
	void OnDying();

	virtual int ZombiePitch() {
		return 100 + RANDOM_LONG(-5,5);
	}
};

LINK_ENTITY_TO_CLASS( monster_zombie, CZombie )

const char *CZombie::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZombie::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZombie::pAttackSounds[] =
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZombie::pIdleSounds[] =
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZombie::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZombie::pPainSounds[] =
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CZombie::Classify( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed( void )
{
	int ys;

	ys = 180;
#if 0
	switch ( m_Activity )
	{
	}
#endif
	pev->yaw_speed = ys;
}

void CZombie::PainSound( void )
{
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, ZombiePitch() );
}

void CZombie::AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1.0, ATTN_NORM, 0, pitch );
}

void CZombie::IdleSound( void )
{
	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, ZombiePitch() );
}

void CZombie::AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, ZombiePitch() );
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombie::HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case ZOMBIE_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
			//ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5 , 5 ) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
		break;
		case ZOMBIE_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
			//ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
		break;
		case ZOMBIE_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgBothSlash, DMG_SLASH );
			if( pHurt )
			{
				if( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackHitSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );
			}
			else
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, RANDOM_SOUND_ARRAY( pAttackMissSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG( -5, 5 ) );

			if( RANDOM_LONG( 0, 1 ) )
				AttackSound();
		}
		break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

BOOL CZombie :: IsBlood ( void )
{
	int Blood = RANDOM_LONG(0,100);

	if (g_iSkillLevel == SKILL_HARD && Blood >= 35)
		return TRUE;
	else if (g_iSkillLevel == SKILL_MEDIUM && Blood >= 50)
		return TRUE;
	else if (g_iSkillLevel == SKILL_EASY && Blood >= 75)
		return TRUE;

	return FALSE;
}

//=========================================================
// Spawn
//=========================================================
void CZombie::Spawn()
{
	Precache();

	if (!IsBlood())
	{
		if (pev->model)
			SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
		else
			SET_MODEL( ENT( pev ), "models/zombie.mdl" );
	}
	else 
	{
		if (pev->body == 1) // reset body, if zombie was set as technician
			pev->body = 0;

		SET_MODEL(ENT(pev), "models/zombie_blood.mdl");
	}

	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_YELLOW;
	if (pev->health == 0)
		pev->health		= gSkillData.zombieHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache()
{
	if (pev->model)
		PRECACHE_MODEL(STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL( "models/zombie.mdl" );

	PRECACHE_MODEL("models/zombie_assassin.mdl");
	PRECACHE_MODEL("models/zombie_barney.mdl");
	PRECACHE_MODEL("models/zombie_blood.mdl");
	PRECACHE_MODEL("models/zombie_soldier.mdl");
	PRECACHE_MODEL("models/xen_zombie_suit.mdl");

	PRECACHE_SOUND("debris/beamstart1.wav");

	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CZombie::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( ( m_Activity == ACT_MELEE_ATTACK1 ) || ( m_Activity == ACT_MELEE_ATTACK1 ) )
	{
#if 0
		if( pev->health < 20 )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE| bits_COND_HEAVY_DAMAGE );
		else
#endif
		if( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= ( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE );
	}

	if( ( m_Activity == ACT_SMALL_FLINCH ) || ( m_Activity == ACT_BIG_FLINCH ) )
	{
		if( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}

	return iIgnore;
}

Schedule_t* CZombie::GetScheduleOfType(int Type)
{
	if (Type == SCHED_CHASE_ENEMY_FAILED && HasMemory(bits_MEMORY_BLOCKER_IS_ENEMY))
	{
		return CBaseMonster::GetScheduleOfType(SCHED_CHASE_ENEMY);
	}
	return CBaseMonster::GetScheduleOfType(Type);
}

void CZombie :: TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if ( ptr->iHitgroup == 10 )
		UTIL_Sparks( ptr->vecEndPos );	// hit armor for zombie_suit

	if	( ptr->iHitgroup == 1 )
	{

		if ( (bitsDamageType & DMG_BULLET) && flDamage == gSkillData.plrDmgBuckshot )
			BuckshotCount++;

		HeadPos = ptr->vecEndPos;

		if ( !HeadGibbed && pev->health <= flDamage * gSkillData.monHead && flDamage >= 10 )
		{
			if ( pev->body == 1 )	//  zombie_technician is implemented via "body = 1"
				pev->body = 3;
			else
				pev->body = 2;

			GibHeadMonster( ptr->vecEndPos, TRUE );
			HeadGibbed = TRUE;
			ScoreForHeadGib(pevAttacker);
			CheckHeadAttacker(pevAttacker);
		}
	}
	
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CZombie :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();

	if ( !HeadGibbed && (pev->health <= flDamage && BuckshotCount >= 4) ) // Hack to handle shotgun shells as each shell is a separate TraceAttack
	{
		if ( pev->body == 1 )	//  zombie_technician is implemented via "body = 1" 
			pev->body = 3;
		else
			pev->body = 2;

		GibHeadMonster( HeadPos, TRUE );
		HeadGibbed = TRUE;
		ScoreForHeadGib(pevAttacker);
		CheckHeadAttacker(pevAttacker);
	}

	BuckshotCount = 0; 	// reset the number of shells if zombie stayed alive
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombie::CheckHeadAttacker(entvars_t *pevAttacker)
{
	CBasePlayer* pPlayer = CBasePlayer::PlayerInstance(pevAttacker);
	if (pPlayer != NULL && pPlayer->m_pActiveItem != NULL && pPlayer->m_pActiveItem->m_iId == WEAPON_PIPEWRENCH)
	{
		pBeheader = pPlayer;
	}
}

void CZombie::OnDying()
{
	if (pBeheader != NULL && pev->solid != SOLID_NOT && HeadGibbed) // lost its head but hasn't been fully gibbed
	{
		pBeheader->SetAchievement("ACH_JEWELRY_WORK");
	}
}

//=========================================================
// Security Guard Zombie
//=========================================================

class CZombieBarney : public CZombie
{
public:
	//=========================================================
	// HandleAnimEvent - catches the monster-specific messages
	// that occur when tagged animation frames are played.
	//=========================================================
	void HandleAnimEvent( MonsterEvent_t *pEvent )
	{
		switch( pEvent->event )
		{
			case ZOMBIE_AE_ATTACK_RIGHT:
			{
				// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombiebarneyDmgOneSlash, DMG_SLASH );
			if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.z = -18;
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
					}
					// Play a random attack hit sound
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else // Play a random attack miss sound
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
			}
			break;

			case ZOMBIE_AE_ATTACK_LEFT:
			{
				// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombiebarneyDmgOneSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.z = 18;
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
			}
			break;

			case ZOMBIE_AE_ATTACK_BOTH:
			{
				// do stuff for this event.
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombiebarneyDmgBothSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
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
	void Spawn()
	{
		Precache( );

		if ( !IsBlood() )
		{
			if (pev->model)
				SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
			else
				SET_MODEL(ENT(pev), "models/zombie_barney.mdl");
		}
		else 
			SET_MODEL(ENT(pev), "models/zombie_blood.mdl");
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

		pev->solid			= SOLID_SLIDEBOX;
		pev->movetype		= MOVETYPE_STEP;
		m_bloodColor		= BLOOD_COLOR_YELLOW;
		if (pev->health == 0)
			pev->health			= gSkillData.zombiebarneyHealth;
		pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
		m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
		m_MonsterState		= MONSTERSTATE_NONE;
		m_afCapability		= bits_CAP_DOORS_GROUP;

		MonsterInit();
	}
};
LINK_ENTITY_TO_CLASS( monster_zombie_barney, CZombieBarney );





//=========================================================
// Mercenary Zombie
//=========================================================

class CZombieSoldier : public CZombie
{
public:
	//=========================================================
	// HandleAnimEvent - catches the monster-specific messages
	// that occur when tagged animation frames are played.
	//=========================================================
	void HandleAnimEvent( MonsterEvent_t *pEvent )
	{
		switch( pEvent->event )
		{
			case ZOMBIE_AE_ATTACK_RIGHT:
			{
				// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombiesoldierDmgOneSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.z = -18;
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
					}
					// Play a random attack hit sound
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else // Play a random attack miss sound
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
			}
			break;

			case ZOMBIE_AE_ATTACK_LEFT:
			{
				// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombiesoldierDmgOneSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.z = 18;
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
			}
			break;

			case ZOMBIE_AE_ATTACK_BOTH:
			{
				// do stuff for this event.
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombiesoldierDmgBothSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
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
	void Spawn()
	{
		Precache( );

		if ( !IsBlood() )
		{
			if (pev->model)
				SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
			else
				SET_MODEL(ENT(pev), "models/zombie_soldier.mdl");
		}
		else 
			SET_MODEL(ENT(pev), "models/zombie_blood.mdl");
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

		pev->solid			= SOLID_SLIDEBOX;
		pev->movetype		= MOVETYPE_STEP;
		m_bloodColor		= BLOOD_COLOR_YELLOW;
		if (pev->health == 0)
			pev->health			= gSkillData.zombiesoldierHealth;
		pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
		m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
		m_MonsterState		= MONSTERSTATE_NONE;
		m_afCapability		= bits_CAP_DOORS_GROUP;

		MonsterInit();
	}
};
LINK_ENTITY_TO_CLASS( monster_zombie_soldier, CZombieSoldier );




//=========================================================
// Delta Protective Suit Zombie
//=========================================================

class CZombieSuit : public CZombieSoldier
{
public:
	//=========================================================
	// Spawn
	//=========================================================
	void Spawn()
	{
		Precache( );

		if (pev->model)
			SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
		else
			SET_MODEL(ENT(pev), "models/xen_zombie_suit.mdl");
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

		pev->solid			= SOLID_SLIDEBOX;
		pev->movetype		= MOVETYPE_STEP;
		m_bloodColor		= BLOOD_COLOR_YELLOW;
		if (pev->health == 0)
			pev->health			= gSkillData.zombiesoldierHealth;
		pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
		m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
		m_MonsterState		= MONSTERSTATE_NONE;
		m_afCapability		= bits_CAP_DOORS_GROUP;

		MonsterInit();
	}
};
LINK_ENTITY_TO_CLASS( monster_zombie_suit, CZombieSuit );



//=========================================================
// Mercenary Assassin Zombie
//=========================================================

class CZombieStealth : public CZombie
{
public:
	//=========================================================
	// HandleAnimEvent - catches the monster-specific messages
	// that occur when tagged animation frames are played.
	//=========================================================
	void HandleAnimEvent( MonsterEvent_t *pEvent )
	{
		if ( m_Activity == ACT_RUN )	// always invisible if moving
		{
			if ( pev->renderamt != InvisOpacity() )
				EMIT_SOUND (ENT(pev), CHAN_WEAPON, "debris/beamstart1.wav", 0.2, ATTN_NORM );

			pev->renderamt = InvisOpacity();
			pev->rendermode = kRenderTransTexture;
		}
		else
		{	
			pev->renderamt = 255;
			pev->rendermode = kRenderNormal;
		}

		switch( pEvent->event )
		{
			case ZOMBIE_AE_ATTACK_RIGHT:
			{
				// do stuff for this event.
		//		ALERT( at_console, "Slash right!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.z = -18;
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
					}
					// Play a random attack hit sound
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else // Play a random attack miss sound
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
			}
			break;

			case ZOMBIE_AE_ATTACK_LEFT:
			{
				// do stuff for this event.
		//		ALERT( at_console, "Slash left!\n" );
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgOneSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.z = 18;
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
			}
			break;

			case ZOMBIE_AE_ATTACK_BOTH:
			{
				// do stuff for this event.
				CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieDmgBothSlash, DMG_SLASH );
				if ( pHurt )
				{
					if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
					{
						pHurt->pev->punchangle.x = 5;
						pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
					}
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
				}
				else
					EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

				if (RANDOM_LONG(0,1))
					AttackSound();
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
	void Spawn()
	{
		Precache( );

		if (pev->model)
			SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
		else
			SET_MODEL(ENT(pev), "models/zombie_assassin.mdl");
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

		pev->solid			= SOLID_SLIDEBOX;
		pev->movetype		= MOVETYPE_STEP;
		m_bloodColor		= BLOOD_COLOR_YELLOW;
		if (pev->health == 0)
			pev->health			= gSkillData.zombieHealth;
		pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
		m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
		m_MonsterState		= MONSTERSTATE_NONE;
		m_afCapability		= bits_CAP_DOORS_GROUP;

		MonsterInit();
	}

	int InvisOpacity() const {
		if ( g_iSkillLevel == SKILL_HARD )
			return 70;
		else
			return 150;
	}

	virtual int ZombiePitch() {
		return 110 + RANDOM_LONG(-5,5);
	}
};
LINK_ENTITY_TO_CLASS( monster_zombie_assassin, CZombieStealth );
