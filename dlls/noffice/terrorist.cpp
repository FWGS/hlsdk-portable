/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
// Hit groups!	
//=========================================================
/*

1 - Head
2 - Stomach
3 - Gun

*/

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"hgrunt.h"

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	TERRORIST_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!
#define TERRORIST_VOL						0.35		// volume of grunt sounds
#define TERRORIST_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define TERRORIST_LIMP_HEALTH				20
#define TERRORIST_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.
#define TERRORIST_NUM_HEADS					2 // how many grunt heads are there? 
#define TERRORIST_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define	TERRORIST_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define TERROR_AK47					( 1 << 0)
#define TERROR_HANDGRENADE			( 1 << 1)
#define TERROR_GRENADELAUNCHER		( 1 << 2)
#define TERROR_SHOTGUN				( 1 << 3)

#define HEAD_GROUP					0
#define HEAD_GRUNT					0
#define HEAD_COMMANDER				1
#define HEAD_SHOTGUN				2
#define HEAD_M203					3
#define GUN_GROUP					1
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_NONE					2

enum
{
	TERROR_SENT_NONE = -1,
	TERROR_SENT_GREN = 0,
	TERROR_SENT_ALERT,
	TERROR_SENT_MONSTER,
	TERROR_SENT_COVER,
	TERROR_SENT_THROW,
	TERROR_SENT_CHARGE,
	TERROR_SENT_TAUNT,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		TERROR_AE_RELOAD		( 2 )
#define		TERROR_AE_KICK			( 3 )
#define		TERROR_AE_BURST1		( 4 )
#define		TERROR_AE_BURST2		( 5 ) 
#define		TERROR_AE_BURST3		( 6 ) 
#define		TERROR_AE_GREN_TOSS		( 7 )
#define		TERROR_AE_GREN_LAUNCH	( 8 )
#define		TERROR_AE_GREN_DROP		( 9 )
#define		TERROR_AE_CAUGHT_ENEMY	( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		TERROR_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

//
// Terrorist special flags.
//
#define TF_HUMAN_SENTRY		1

extern Schedule_t	slGruntCombatFail[];
extern Schedule_t	slGruntEstablishLineOfFire[];
extern Schedule_t	slGruntTakeCover[];
extern Schedule_t	slGruntGrenadeCover[];
extern Schedule_t	slGruntTossGrenadeCover[];
extern Schedule_t	slGruntTakeCoverFromBestSound[];
extern Schedule_t	slGruntHideReload[];
extern Schedule_t	slGruntRangeAttack1A[];
extern Schedule_t	slGruntRangeAttack1B[];


//=========================================================
// Terrorist
//=========================================================
class CTerrorist : public CHGrunt
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void Killed(entvars_t *pevAttacker, int iGib);

	void Shoot(Vector vecSpread, int iBulletType);
	void ShootAK47(Vector vecSpread);

	void DeathSound(void);
	void PainSound(void);
	void GibMonster(void);

	BOOL IsHumanSentry() const;

	BOOL HasWeapon();
	void DropWeapon(Vector vecVelocity = g_vecZero, Vector angVelocity = g_vecZero);

	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

	Schedule_t  *GetScheduleOfType(int Type);

	int	Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	static const char *pPainSounds[];
	static const char *pDeathSounds[];
	static const char *pBurstSounds[];

	int		m_iTerrorFlags;
	BOOL	m_bHasWeapon;
};

LINK_ENTITY_TO_CLASS(monster_terrorist, CTerrorist);
LINK_ENTITY_TO_CLASS(monster_human_sentry, CTerrorist);

TYPEDESCRIPTION	CTerrorist::m_SaveData[] =
{
	DEFINE_FIELD(CTerrorist, m_iTerrorFlags, FIELD_INTEGER),
	DEFINE_FIELD(CTerrorist, m_bHasWeapon, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CTerrorist, CHGrunt);

const char *CTerrorist::pPainSounds[] =
{
	"hgrunt/gr_pain1.wav",
	"hgrunt/gr_pain2.wav",
	"hgrunt/gr_pain3.wav",
	"hgrunt/gr_pain4.wav",
	"hgrunt/gr_pain5.wav",
};

const char *CTerrorist::pDeathSounds[] =
{
	"hgrunt/gr_die1.wav",
	"hgrunt/gr_die2.wav",
	"hgrunt/gr_die3.wav",
};

const char *CTerrorist::pBurstSounds[] =
{
	"hgrunt/gr_mgun1.wav",
	"hgrunt/gr_mgun2.wav",
};


//=========================================================
// Shoot
//=========================================================
void CTerrorist::Shoot(Vector vecSpread, int iBulletType)
{
	if( m_hEnemy == 0 )
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, vecSpread, 2048, iBulletType); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}

//=========================================================
// ShootAK47
//=========================================================
void CTerrorist::ShootAK47(Vector vecSpread)
{
	Shoot(vecSpread, BULLET_MONSTER_MP5);

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBurstSounds), 1, ATTN_NORM);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTerrorist::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case TERROR_AE_DROP_GUN:
	{
		if (HasWeapon())
		{
			DropWeapon();
		}
		break;
	}

	case TERROR_AE_BURST1:
	{
		if (FBitSet(pev->weapons, TERROR_AK47))
		{
			ShootAK47( VECTOR_CONE_10DEGREES );
		}
		else
		{
			Shotgun();

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case TERROR_AE_BURST2:
	case TERROR_AE_BURST3:
		ShootAK47( VECTOR_CONE_10DEGREES );
		break;

	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

void CTerrorist::Killed(entvars_t *pevAttacker, int iGib)
{
	SetBodygroup(GUN_GROUP, GUN_NONE);

	CHGrunt::Killed(pevAttacker, iGib);
}

//=========================================================
// Spawn
//=========================================================
void CTerrorist::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/terrorist.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hgruntHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = TERROR_SENT_NONE;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	m_bHasWeapon = TRUE;

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = TERROR_AK47;
	}
	
	// Prevent terrorist from using grenade launchers.
	pev->weapons &= ~TERROR_GRENADELAUNCHER;

	if (FBitSet(pev->weapons, TERROR_SHOTGUN))
	{
		SetBodygroup(GUN_GROUP, GUN_SHOTGUN);
		m_cClipSize = SHOTGUN_MAX_CLIP;
	}
	else
	{
		m_cClipSize = TERRORIST_CLIP_SIZE;
	}

	if( pev->body == -1 )
		pev->body = RANDOM_LONG( 0, 3 );
 
	m_cAmmoLoaded = m_cClipSize;

	m_iTerrorFlags = 0;

	if (FClassnameIs(pev, "monster_human_sentry"))
	{
		m_iTerrorFlags |= TF_HUMAN_SENTRY;
	}

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CTerrorist::Precache()
{
	PRECACHE_MODEL("models/terrorist.mdl");

	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pBurstSounds );

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/sbarrel1.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 100 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
void CTerrorist::GibMonster(void)
{
	if (HasWeapon() )
	{
		Vector velocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(200, 300));
		Vector angVelocity = Vector(0, RANDOM_FLOAT(200, 400), 0);

		DropWeapon(velocity, angVelocity);
	}

	CBaseMonster::GibMonster();
}

//=========================================================
// PainSound
//=========================================================
void CTerrorist::PainSound(void)
{
	if (gpGlobals->time > m_flNextPainTime)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), 1, ATTN_NORM);

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CTerrorist::DeathSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), 1, ATTN_IDLE);
}

//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
void CTerrorist::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	if (pev->takedamage)
	{
		if (!IsHumanSentry())
		{
			if( FBitSet( bitsDamageType, DMG_CLUB ) )
				flDamage *= 2.0f;

			if (ptr->iHitgroup == HITGROUP_RIGHTARM)
			{
				flDamage -= 20.0f;

				if( flDamage <= 0.0f )
					flDamage = 10.0f;

				// ==========================================
				// Code changes for- Night at the Office:
				// ==========================================
				//
				// -Droppable guns. If terrorist is shot in the arm, he will drop
				//  his gun and seek cover. Very useful in situation where player
				//  is low on health/ammo or in a stealthy situation. Difficult to
				//  get an arm shot while in combat, due to stance of the terrorist.

				if (HasWeapon())
				{
					Vector velocity = Vector( RANDOM_FLOAT( -5, 5 ), RANDOM_FLOAT( -5, 5 ), RANDOM_FLOAT( -5, 5 ) );
					Vector angVelocity = Vector(0, RANDOM_FLOAT(1, 10), 0);

					DropWeapon(velocity, angVelocity);

					// Remove ability to shoot at enemy, now that we dropped our weapon.
					m_afCapability &= ~bits_CAP_RANGE_ATTACK1;
				}
			}
			// check for helmet shot
			else if (ptr->iHitgroup == 11)
                	{
                        	// it's head shot anyways
				ptr->iHitgroup = HITGROUP_HEAD;

				flDamage *= 5.0f;
			}
		}
	}

	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
//=========================================================
Schedule_t* CTerrorist :: GetScheduleOfType ( int Type ) 
{
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			// Human sentries are not allowed to take cover.
			if (IsHumanSentry())
			{
				return CSquadMonster::GetScheduleOfType( SCHED_COMBAT_FACE );
			}
			else
			{
				if (InSquad())
				{
					if (g_iSkillLevel == SKILL_HARD && HasConditions(bits_COND_CAN_RANGE_ATTACK2) && OccupySlot(bits_SLOTS_HGRUNT_GRENADE))
					{
						if (FOkToSpeak())
						{
							SENTENCEG_PlayRndSz(ENT(pev), "HG_THROW", TERRORIST_SENTENCE_VOLUME, TERRORIST_ATTN, 0, m_voicePitch);
							JustSpoke();
						}
						return slGruntTossGrenadeCover;
					}
					else
					{
						return &slGruntTakeCover[0];
					}
				}
				else
				{
					if (HasWeapon())
					{
						if (RANDOM_LONG(0, 1))
						{
							return &slGruntTakeCover[0];
						}
						else
						{
							return &slGruntGrenadeCover[0];
						}
					}
					else
					{
						return &slGruntTakeCover[0];
					}
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			// Human sentries are not allowed to run away from sounds.
			if (IsHumanSentry())
			{
				return GetScheduleOfType( SCHED_COWER );
			}
			else
			{
				return &slGruntTakeCoverFromBestSound[ 0 ];
			}
		}
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			// Human sentries are not allowed to strafe to get a better shot.
			if (IsHumanSentry())
			{
				return &slGruntCombatFail[ 0 ];
			}
			else
			{
				return  &slGruntEstablishLineOfFire[0];
			}
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			if ( HasWeapon() )
			{
				// randomly stand or crouch
				if (RANDOM_LONG(0, 9) == 0)
					m_fStanding = RANDOM_LONG(0, 1);

				if (m_fStanding)
					return &slGruntRangeAttack1B[0];
				else
					return &slGruntRangeAttack1A[0];
			}
			else
			{
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}
		}

	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			// Human sentries can only reload in situ.
			if (IsHumanSentry())
			{
				return CSquadMonster::GetScheduleOfType( SCHED_RELOAD );
			}
			else
			{
				return &slGruntHideReload[ 0 ];
			}
		}
	default:
		{
			return CHGrunt :: GetScheduleOfType ( Type );
		}
	}
}

BOOL CTerrorist::IsHumanSentry() const
{
	return (m_iTerrorFlags & TF_HUMAN_SENTRY);
}

BOOL CTerrorist::HasWeapon()
{
	return m_bHasWeapon; /* (GetBodygroup(GUN_GROUP) != GUN_NONE)*/;
}

void CTerrorist::DropWeapon(Vector vecVelocity, Vector angVelocity)
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	CBaseEntity *pGun;
	GetAttachment(0, vecGunPos, vecGunAngles);

	// switch to body group with no gun.
	SetBodygroup(GUN_GROUP, GUN_NONE);

	// now spawn a gun.
	if (FBitSet(pev->weapons, TERROR_SHOTGUN))
	{
		pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
	}
	else
	{
		pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
	}

	if (pGun)
	{
		pGun->pev->velocity = vecVelocity;
		pGun->pev->avelocity = angVelocity;
	}

	m_bHasWeapon = FALSE;
}


//=========================================================
// DEAD TERRORIST PROP
//=========================================================
class CDeadTerrorist : public CDeadHGrunt
{
public:
	void Spawn(void);
};

LINK_ENTITY_TO_CLASS(monster_terrorist_dead, CDeadTerrorist);

//=========================================================
// ********** DeadTerrorist SPAWN **********
//=========================================================
void CDeadTerrorist::Spawn(void)
{
	PRECACHE_MODEL("models/terrorist.mdl");
	SET_MODEL(ENT(pev), "models/terrorist.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);

	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead terrorist with bad pose\n");
	}

	// Corpses have less health
	pev->health = 8;

	// map old bodies onto new bodies
	switch (pev->body)
	{
	case 0: // Grunt with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(GUN_GROUP, GUN_MP5);
		break;
	case 1: // Commander with Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(GUN_GROUP, GUN_MP5);
		break;
	case 2: // Grunt no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(GUN_GROUP, GUN_NONE);
		break;
	case 3: // Commander no Gun
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup(GUN_GROUP, GUN_NONE);
		break;
	}

	MonsterInitDead();
}
