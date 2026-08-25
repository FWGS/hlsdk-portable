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

extern DLL_GLOBAL int  g_iSkillLevel;



#define BOLT_AIR_VELOCITY	1600
#define BOLT_WATER_VELOCITY	1000

class CZombieBullet : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);

	int m_iTrail;

public:
	static CZombieBullet *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(zombie_bullet, CZombieBullet);

CZombieBullet *CZombieBullet::BoltCreate(void)
{
	// Create a new entity with CZombieBullet private data
	CZombieBullet *pBolt = GetClassPtr((CZombieBullet *)NULL);
	pBolt->pev->classname = MAKE_STRING("zombie_bullet");
	pBolt->Spawn();

	return pBolt;
}

void CZombieBullet::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.6;

	SET_MODEL(ENT(pev), "models/shell.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-1, -1, -1), Vector(1, 1, 1));

	SetTouch(&CZombieBullet::BoltTouch);
	SetThink(&CZombieBullet::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CZombieBullet::Precache()
{
	PRECACHE_MODEL("models/shell.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/ric1.wav");
	PRECACHE_SOUND("weapons/ric2.wav");
	PRECACHE_SOUND("weapons/ric3.wav");
	PRECACHE_SOUND("weapons/ric4.wav");
	PRECACHE_SOUND("weapons/ric5.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CZombieBullet::Classify(void)
{
	return	CLASS_NONE;
}

void CZombieBullet::BoltTouch(CBaseEntity *pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if (pOther->IsPlayer())
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowClient, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);
		}
		else
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
		}
		if (pOther->pev->size.z <= 90)
			pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_forward * 50;

		ApplyMultiDamage(pev, pevOwner);

		pev->velocity = Vector(0, 0, 0);
		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}
	}
	else
	{

		switch (RANDOM_LONG(0, 4))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/ric1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7)); break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/ric2.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7)); break;
		case 2:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/ric3.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7)); break;
		case 3:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/ric4.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7)); break;
		case 4:
			EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/ric5.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7)); break;
		}

		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 12);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}
	UTIL_Remove(this);
}

void CZombieBullet::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CZombieBullet::ExplodeThink(void)
{
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15); // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB);

	UTIL_Remove(this);
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03
#define ZOMBIE_AE_DRAWGUN			4
#define	ZOMBIE_AE_SHOOT				5
#define	ZOMBIE_AE_HOLSTER			6

#define ZOMBIE_FLINCH_DELAY		2		// at most one flinch every n secs

class CZombie : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int Classify( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	int IgnoreConditions( void );
	int LastSpoken;
	int DodgeTimer;
	int DodgePoints = 15;
	int m_flNextShootTime;
	void Shoot(void);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	void FireProjBullets(Vector vecShootOrigin, Vector vecDirShooting, Vector AnglesAim);
	void EXPORT Dodged(void);

	float m_flNextFlinch;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void DeathSound( void );

	int		m_iShotgunShell;

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist ) { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
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
	"zombie/zo_attack3.wav",
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
	"zombie/zo_alert40.wav",
};

const char *CZombie::pPainSounds[] =
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
	"zombie/zo_pain3.wav",
};

const char *CZombie::pDeathSounds[] =
{
	"zombie/zo_death1.wav",
	"zombie/zo_death2.wav",
	"zombie/zo_death3.wav",
};

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CZombie::Classify( void )
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombie::SetYawSpeed( void )
{
	pev->yaw_speed = 300;
}

int CZombie::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - ( pevInflictor->absmin + pevInflictor->absmax ) * 0.5f;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3f;
	}

	// HACK HACK -- until we fix this.
	if( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombie::PainSound( void )
{
	if (LastSpoken < gpGlobals->time)
	{
		int pitch = 95 + RANDOM_LONG( 0, 9 );

		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pPainSounds ), 1.0, ATTN_NORM, 0, pitch );
		LastSpoken = gpGlobals->time + 10;
	}
}

void CZombie::AlertSound( void )
{
	if (LastSpoken < gpGlobals->time)
	{
		int pitch = 95 + RANDOM_LONG( 0, 9 );

		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAlertSounds ), 1.0, ATTN_NORM, 0, pitch );
		LastSpoken = gpGlobals->time + 10;
	}
}

void CZombie::IdleSound( void )
{
	if (LastSpoken < gpGlobals->time)
	{
		// Play a random idle sound
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pIdleSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		LastSpoken = gpGlobals->time + 6;
	}
}

void CZombie::AttackSound( void )
{
	if (LastSpoken < gpGlobals->time)
	{
		// Play a random attack sound
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pAttackSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
		LastSpoken = gpGlobals->time + 10;
	}
}

void CZombie::DeathSound(void)
{
	// Play a random attack sound
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, RANDOM_SOUND_ARRAY( pDeathSounds ), 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
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
		case ZOMBIE_AE_DRAWGUN:
		{
			pev->body = 1;
		}
		break;
		case ZOMBIE_AE_HOLSTER:
		{
			pev->body = 0;
		}
		break;
		case ZOMBIE_AE_SHOOT:
		{
			Shoot();		
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
void CZombie::Spawn()
{
	Precache();

	SET_MODEL( ENT( pev ), "models/zombie.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid		= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health		= gSkillData.zombieHealth * 2;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.3;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;
	DodgePoints			= RANDOM_LONG(0, 15);

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombie::Precache()
{
	PRECACHE_MODEL( "models/zombie.mdl" );

	PRECACHE_SOUND_ARRAY( pAttackHitSounds );
	PRECACHE_SOUND_ARRAY( pAttackMissSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );

	PRECACHE_SOUND("weapons/sshotgun_shoot.wav");
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	PRECACHE_SOUND_ARRAY(pDeathSounds);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

int CZombie::IgnoreConditions( void )
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if( m_Activity != ACT_RANGE_ATTACK1 )
	{
		pev->body = 0;
	}


	if( ( m_Activity == ACT_MELEE_ATTACK1 ) || ( m_Activity == ACT_MELEE_ATTACK1 ) || ( m_Activity == ACT_RANGE_ATTACK1 ) )
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

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CZombie::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist >= 512)
	{
		return FALSE;
	}

	if (flDist > 100 && flDist <= 800 && flDot >= 0.5 && gpGlobals->time >= m_flNextShootTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 80)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}
		return TRUE;
	}

	return FALSE;
}

void CZombie::Shoot(void)
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector	vecSpitOffset;
	Vector	vecSpitDir;
	Vector anglesAim = pev->v_angle + pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;
	UTIL_MakeVectors(pev->angles);

	// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
	// we should be able to read the position of bones at runtime for this info.
	vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 55);
	vecSpitOffset = (pev->origin + vecSpitOffset);
	if (m_hEnemy)
		vecSpitDir = ((m_hEnemy->pev->origin + gpGlobals->v_up * 48) - vecSpitOffset).Normalize();
	else
		vecSpitDir = ((pev->origin + gpGlobals->v_forward * 32) - vecSpitOffset).Normalize();

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecSpitOffset - vecSpitDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	//FireBullets(16, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0); // shoot +-7.5 degrees
	FireProjBullets(vecSpitOffset, vecSpitDir, anglesAim);


	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/sshotgun_shoot.wav", 1.0, ATTN_NORM, 0, 100);

	pev->effects = EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecSpitDir);
	m_flNextShootTime = gpGlobals->time + 10;
}


void CZombie::FireProjBullets(Vector vecShootOrigin, Vector vecDirShooting, Vector AnglesAim)
{
	Vector vecSrc[5];
	vecSrc[0] = vecShootOrigin;
	vecSrc[1] = vecShootOrigin + gpGlobals->v_right * 3;
	vecSrc[2] = vecShootOrigin + gpGlobals->v_right * 6;
	vecSrc[3] = vecShootOrigin - gpGlobals->v_right * 3;
	vecSrc[4] = vecShootOrigin - gpGlobals->v_right * 6;
	Vector vecDir[5];
	vecDir[0] = gpGlobals->v_forward;
	vecDir[1] = gpGlobals->v_forward + gpGlobals->v_right * 0.05;
	vecDir[2] = gpGlobals->v_forward + gpGlobals->v_right * 0.1;
	vecDir[3] = gpGlobals->v_forward - gpGlobals->v_right * 0.05;
	vecDir[4] = gpGlobals->v_forward - gpGlobals->v_right * 0.1;

	CZombieBullet *pBolt[5];
	for (int i = 0; i < 5; i++)
	{
		pBolt[i] = CZombieBullet::BoltCreate();
		pBolt[i]->pev->origin = vecSrc[i] + gpGlobals->v_forward * RANDOM_LONG(-5, 5) + gpGlobals->v_up * RANDOM_LONG(-10, 10);
		pBolt[i]->pev->angles = pBolt[i]->pev->angles = UTIL_VecToAngles(vecDirShooting);
		pBolt[i]->pev->owner = ENT(pev);
		pBolt[i]->pev->velocity = vecDir[i] * BOLT_AIR_VELOCITY;
		pBolt[i]->pev->speed = BOLT_AIR_VELOCITY;
	}

	return;
}

void CZombie::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (g_iSkillLevel == SKILL_HARD)
		if ((bitsDamageType == DMG_BULLET) && pev->health > 10)
		{
			if (DodgePoints > 0)
			{
				DodgePoints--;
				if (DodgePoints <= 0)
					DodgeTimer = gpGlobals->time + 20;

				flDamage = 0;
				UTIL_MakeVectors(pev->angles);
				switch (RANDOM_LONG(0, 2))
				{
				case 0:
				{
						  pev->sequence = LookupSequence("dodgeup"); break;
						  pev->velocity = pev->velocity + gpGlobals->v_forward * 150 + gpGlobals->v_up * 60;
				}
				case 1:
				{
						  pev->sequence = LookupSequence("dodgeleft"); break;
						  pev->velocity = pev->velocity + gpGlobals->v_forward * 150 + gpGlobals->v_up * 60;
				}
				case 2:
				{
						  pev->sequence = LookupSequence("dodgeright"); break;
						  pev->velocity = pev->velocity + gpGlobals->v_forward * 150 + gpGlobals->v_up * 60;
				}	
				}
			
				pev->frame = 0;
				pev->framerate = 1;
				ResetSequenceInfo();
				pev->nextthink = gpGlobals->time + 0.1;
				m_IdealMonsterState = MONSTERSTATE_IDLE;
			}
			else
			{
				if (gpGlobals->time >= DodgeTimer)
				{
					DodgePoints = 8;
				}
			}
		}
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CZombie::Dodged(void)
{
	ResetSequenceInfo();
	m_IdealMonsterState = MONSTERSTATE_IDLE;
}