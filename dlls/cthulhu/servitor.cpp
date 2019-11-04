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
//=========================================================
// Servitor
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"player.h"
#include	"schedule.h"


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	SERVITOR_AE_ATTACK_RIGHT	0x01
#define	SERVITOR_AE_ATTACK_LEFT		0x02
#define	SERVITOR_AE_GRAB			0x03
#define	SERVITOR_AE_BITE			0x04
#define	SERVITOR_AE_THROW			0x05

#define SERVITOR_FLINCH_DELAY		8		// at most one flinch every n secs

#define SERVITOR_REACH				256

class CServitor : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions ( void );

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );

	void EXPORT ServitorThink( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	virtual BOOL CheckMeleeAttack1( float flDot, float flDist );
	virtual BOOL CheckMeleeAttack2( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	CBaseEntity* ServitorCheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	void			DropVictim(void);

	BOOL			mbGrabbedVictim;
	CBaseEntity*	mpVictim;
	int				miVictimMoveType;
};

LINK_ENTITY_TO_CLASS( monster_servitor, CServitor );

const char *CServitor::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CServitor::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CServitor::pAttackSounds[] = 
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CServitor::pIdleSounds[] = 
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CServitor::pAlertSounds[] = 
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CServitor::pPainSounds[] = 
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

TYPEDESCRIPTION	CServitor::m_SaveData[] = 
{
	DEFINE_FIELD( CServitor, mbGrabbedVictim,  FIELD_BOOLEAN ),
	DEFINE_FIELD( CServitor, mpVictim,         FIELD_CLASSPTR ),
	DEFINE_FIELD( CServitor, miVictimMoveType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CServitor, CBaseMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CServitor :: Classify ( void )
{
	return	m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CServitor :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CServitor :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	//int iGround = FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND );

	// Cthulhu: but this stops the monster from hitting flying monsters that are
	// melee attacking it (e.g. nightgaunt)
	// Solution: explicitly check for monster types that we cannot hit

	BOOL bHit = TRUE;	// we can hit by default
	/*
	if (m_hEnemy)
	{
		if (FClassnameIs( m_hEnemy->pev, "hornet"))			bHit = FALSE;
		if (FClassnameIs( m_hEnemy->pev, "monster_snark"))	bHit = FALSE;
	}
	*/

	//if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL && iGround )
	if ( flDist <= SERVITOR_REACH && flDot >= 0.7 && m_hEnemy != NULL && bHit )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
BOOL CServitor :: CheckMeleeAttack2 ( float flDot, float flDist )
{
	// Decent fix to keep folks from kicking/punching hornets and snarks is to check the onground flag(sjb)
	//int iGround = FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND );

	// Cthulhu: but this stops the monster from hitting flying monsters that are
	// melee attacking it (e.g. nightgaunt)
	// Solution: explicitly check for monster types that we cannot hit

	BOOL bHit = TRUE;	// we can hit by default
	/*
	if (m_hEnemy)
	{
		if (FClassnameIs( m_hEnemy->pev, "hornet"))			bHit = FALSE;
		if (FClassnameIs( m_hEnemy->pev, "monster_snark"))	bHit = FALSE;
	}
	*/

	//if ( flDist <= 64 && flDot >= 0.7 && m_hEnemy != NULL && iGround )
	if ( flDist <= SERVITOR_REACH && flDot >= 0.7 && m_hEnemy != NULL && bHit )
	{
		return TRUE;
	}
	return FALSE;
}

int CServitor :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 20% damage from bullets
	if ( bitsDamageType & DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.2;
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CServitor :: PainSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 3)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CServitor :: AlertSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CServitor :: IdleSound( void )
{
	int pitch = 95 + RANDOM_LONG(0,9);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}

void CServitor :: AttackSound( void )
{
	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CServitor :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case SERVITOR_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = ServitorCheckTraceHullAttack( SERVITOR_REACH + 96, gSkillData.zombieDmgOneSlash * 2, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 150;
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

		case SERVITOR_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = ServitorCheckTraceHullAttack( SERVITOR_REACH + 96, gSkillData.zombieDmgOneSlash * 2, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 150;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case SERVITOR_AE_GRAB:
		{
			// get the position of attachment 1 (index 0) (right hand)
			Vector vecPosition;
			Vector vecJunk;
			CBaseEntity* pEntity = NULL;
			GetAttachment( 0, vecPosition, vecJunk );
			// look in the region of his hand
			bool bFound = false;
			while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecPosition, 64 )) != NULL)
			{
				// is it a monster or player
				if (pEntity->pev->flags & (FL_MONSTER|FL_CLIENT))
				{
					// is it small enough (near man sized)
					if (pEntity->pev->absmax.z - pEntity->pev->absmin.z <= 96)
					{
						// set the grabbed flag to true
						mbGrabbedVictim = TRUE;
						// set the grabbed monster pointer
						mpVictim = pEntity;
						// make monster prone (cf barnacle)
						mpVictim->FBecomeProne();
						// save the grabbed monsters original movetype
						miVictimMoveType = mpVictim->pev->movetype;
						// set the grabbed monsters movetype to fly
						mpVictim->pev->movetype = MOVETYPE_FLY;
						// set it's origin to the servitors hand
						mpVictim->pev->origin = vecPosition - Vector(0,0,48);
						bFound = true;
						break;
					}
				}
			}
			// if we didn't find something
			if (!bFound)
			{
				// stop the sequence
				ResetSequenceInfo();
			}
		}
		break;

		case SERVITOR_AE_BITE:
		{
			// we have gotten the monster as far as our mouth
			if (mbGrabbedVictim)
			{
				// if the victim is dead
				if (!mpVictim->IsAlive())
				{
					// just drop it
					DropVictim(); // reset movetype, unset flag and pointer (cf barnacle release)
					ResetSequenceInfo();
				}
				else
				{
					// apply damage
					mpVictim->TakeDamage ( pev, pev, mpVictim->pev->health, DMG_SLASH | DMG_ALWAYSGIB );
					if (!mpVictim->IsAlive())
					{
						mbGrabbedVictim = FALSE;
						mpVictim = NULL;
					}
				}
			}
		}
		break;

		case SERVITOR_AE_THROW:
		{
			if (mbGrabbedVictim)
			{
				// we have bitten this monster, but it is still alive!
				// so just throw it away
				mpVictim->pev->movetype = miVictimMoveType;
				if (mpVictim->pev->flags & FL_MONSTER)
				{
					((CBaseMonster*)mpVictim)->m_IdealMonsterState = MONSTERSTATE_IDLE;
				}
				else // player
				{
					((CBasePlayer*)mpVictim)->BarnacleVictimReleased();
				}
				// face the victim the same way as the monster and then turn 180
				mpVictim->pev->angles = pev->angles;
				mpVictim->pev->angles.y = UTIL_AngleMod(mpVictim->pev->angles.y + 180.0);

				mpVictim->pev->punchangle.z = -20;
				mpVictim->pev->punchangle.x = 20;
				mpVictim->pev->velocity = mpVictim->pev->velocity + gpGlobals->v_forward * -500;
				mpVictim->pev->velocity = mpVictim->pev->velocity + gpGlobals->v_up * 100;
				mpVictim = NULL;
				mbGrabbedVictim = FALSE;
			}
		}
		break;

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

void CServitor :: ServitorThink()
{
	// do we need to start the monster off...
	if (m_afCapability == bits_CAP_DOORS_GROUP)
	{
		StartMonster();
		SetThink(ServitorThink);
	}

	//if we have grabbed someone
	if (mbGrabbedVictim)
	{
		SetNextThink(0.05);
		float flInterval = StudioFrameAdvance( ); // animate

		// are we dead
		if (!IsAlive())
		{
			DropVictim();
		}
		// else are they dead
		else if (!mpVictim->IsAlive())
		{
			DropVictim();
		}
		else
		{
			// set their origin to the correct position
			Vector vecPosition;
			Vector vecJunk;
			GetAttachment( 0, vecPosition, vecJunk );
			mpVictim->pev->origin = vecPosition - Vector(0,0,48);
		}

		DispatchAnimEvents( flInterval );
	}
	else
	{
		CBaseMonster::MonsterThink();
	}
}

void CServitor :: DropVictim()
{
	mpVictim->pev->movetype = miVictimMoveType;
	if (mpVictim->pev->flags & FL_MONSTER)
	{
		((CBaseMonster*)mpVictim)->m_IdealMonsterState = MONSTERSTATE_IDLE;
	}
	mpVictim->pev->velocity = g_vecZero;
	mpVictim = NULL;
	mbGrabbedVictim = FALSE;
}

//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
// Used for many contact-range melee attacks. Bites, claws, etc.

// Overridden for Servitor because his swing starts lower as
// a percentage of his height (otherwise he swings over the
// players head)
//=========================================================
CBaseEntity* CServitor::ServitorCheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	// the enemy may be standing on something, still within reach of a big monster like this,
	// but above the 'normal' swipe height

	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += 32;
	//vecStart.z += 128;
	//Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) - (gpGlobals->v_up * flDist * 0.3);
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist);

	for (double dUp = 0.0; dUp <= 192.0; dUp += 64.0)
	{
		UTIL_TraceHull( vecStart + Vector(0,0,dUp), vecEnd + Vector(0,0,dUp), dont_ignore_monsters, head_hull, ENT(pev), &tr );
		
		if ( tr.pHit )
		{
			CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

			// don't hit the world
			if (FClassnameIs( pEntity->pev, "worldspawn")) continue;

			if ( iDamage > 0 )
			{
				pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
			}

			return pEntity;
		}
	}

	return NULL;
}

//=========================================================
// Spawn
//=========================================================
void CServitor :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/monsters/servitor.mdl");
	UTIL_SetSize( pev, Vector( -48, -48, 0 ), Vector( 48, 48, 256 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_YELLOW;

	if (pev->health == 0)
		pev->health			= gSkillData.gargantuaHealth;
	//pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin (taken from the model file).
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	// initialise the victim variables
	mbGrabbedVictim = FALSE;
	mpVictim = NULL;
	miVictimMoveType = 0;

	MonsterInit();

	SetThink(ServitorThink);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CServitor :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/monsters/servitor.mdl");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CServitor::IgnoreConditions ( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK2))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + SERVITOR_FLINCH_DELAY;
	}

	return iIgnore;
}




