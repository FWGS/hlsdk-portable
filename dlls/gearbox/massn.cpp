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
// Black Ops - Male Assassin
//=========================================================

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
#define	MASSN_CLIP_SIZE				36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

// Weapon flags
#define MASSN_9MMAR					(1 << 0)
#define MASSN_HANDGRENADE			(1 << 1)
#define MASSN_GRENADELAUNCHER		(1 << 2)
#define MASSN_SNIPERRIFLE			(1 << 3)

// Body groups.
#define HEAD_GROUP					1
#define GUN_GROUP					2

// Head values
#define HEAD_WHITE					0
#define HEAD_BLACK					1
#define HEAD_GOGGLES				2

// Gun values
#define GUN_MP5						0
#define GUN_SNIPERRIFLE				1
#define GUN_NONE					2

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		MASSN_AE_KICK			( 3 )
#define		MASSN_AE_BURST1			( 4 )
#define		MASSN_AE_CAUGHT_ENEMY	( 10 ) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define		MASSN_AE_DROP_GUN		( 11 ) // grunt (probably dead) is dropping his mp5.

//=========================================================
// Purpose:
//=========================================================
class CMassn : public CHGrunt
{
public:
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void GibMonster(void);
	void Sniperrifle(void);

	BOOL FOkToSpeak(void);

	void Spawn( void );
	void Precache( void );

	void DeathSound(void);
	void PainSound(void);
	void IdleSound(void);
};

LINK_ENTITY_TO_CLASS(monster_male_assassin, CMassn)

//=========================================================
// Purpose:
//=========================================================
BOOL CMassn::FOkToSpeak(void)
{
	return FALSE;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CMassn::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// Purpose:
//=========================================================
void CMassn::IdleSound(void)
{
}

//=========================================================
// Shoot
//=========================================================
void CMassn::Sniperrifle(void)
{
	if (m_hEnemy == 0)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 2048, BULLET_MONSTER_762, 0); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CMassn::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	Vector	vecShootDir;
	Vector	vecShootOrigin;

	switch (pEvent->event)
	{
	case MASSN_AE_DROP_GUN:
	{
		Vector	vecGunPos;
		Vector	vecGunAngles;

		if(GetBodygroup(GUN_GROUP) != GUN_NONE)
		{
			GetAttachment(0, vecGunPos, vecGunAngles);

			// switch to body group with no gun.
			SetBodygroup(GUN_GROUP, GUN_NONE);

			// now spawn a gun.
			if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
			{
				DropItem("weapon_sniperrifle", vecGunPos, vecGunAngles);
			}
			else
			{
				DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
			}

			if (FBitSet(pev->weapons, MASSN_GRENADELAUNCHER))
			{
				DropItem("ammo_ARgrenades", BodyTarget(pev->origin), vecGunAngles);
			}
		}
	}
	break;


	case MASSN_AE_BURST1:
	{
		if (FBitSet(pev->weapons, MASSN_9MMAR))
		{
			Shoot();

			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if (RANDOM_LONG(0, 1))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM);
			}
			else
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM);
			}
		}
		else if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
		{
			Sniperrifle();

			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM);
		}

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);
	}
	break;

	case MASSN_AE_KICK:
	{
		CBaseEntity *pHurt = Kick();

		if (pHurt)
		{
			// SOUND HERE!
			UTIL_MakeVectors(pev->angles);
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage(pev, pev, gSkillData.massnDmgKick, DMG_CLUB);
		}
	}
	break;

	case MASSN_AE_CAUGHT_ENEMY:
		break;

	default:
		CHGrunt::HandleAnimEvent(pEvent);
		break;
	}
}

void CMassn::GibMonster()
{
	Vector vecGunPos;
	Vector vecGunAngles;

	if( GetBodygroup( GUN_GROUP ) != GUN_NONE )
	{
		// throw a gun if the grunt has one
		GetAttachment( 0, vecGunPos, vecGunAngles );

		CBaseEntity *pGun;

		if( FBitSet( pev->weapons, MASSN_SNIPERRIFLE ) )
		{
			pGun = DropItem( "weapon_sniperrifle", vecGunPos, vecGunAngles );
		}
		else
		{
			pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
		}

		if( pGun )
		{
			pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
			pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}

		if( FBitSet( pev->weapons, MASSN_GRENADELAUNCHER ) )
		{
			pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
				pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}

	CBaseMonster::GibMonster();
}

//=========================================================
// Spawn
//=========================================================
void CMassn::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/massn.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.massnHealth;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = -1;

	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;

	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	m_HackedGunPos = Vector(0, 0, 55);

	if (pev->weapons == 0)
	{
		// initialize to original values
		pev->weapons = MASSN_9MMAR | MASSN_HANDGRENADE;
		// pev->weapons = HGRUNT_SHOTGUN;
		// pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
	}

	if (FBitSet(pev->weapons, MASSN_SNIPERRIFLE))
	{
		SetBodygroup(GUN_GROUP, GUN_SNIPERRIFLE);
		m_cClipSize = 5;
	}
	else
	{
		m_cClipSize = MASSN_CLIP_SIZE;
	}
	m_cAmmoLoaded = m_cClipSize;

	if (RANDOM_LONG(0, 99) < 80)
		pev->skin = 0;	// light skin
	else
		pev->skin = 1;	// dark skin

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CMassn::Precache()
{
	PRECACHE_MODEL("models/massn.mdl");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	PRECACHE_SOUND("hgrunt/gr_reload1.wav");

	PRECACHE_SOUND("weapons/glauncher.wav");

	PRECACHE_SOUND("weapons/sniper_bolt1.wav");
	PRECACHE_SOUND("weapons/sniper_fire.wav");

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	// get voice pitch
	if (RANDOM_LONG(0, 1))
		m_voicePitch = 109 + RANDOM_LONG(0, 7);
	else
		m_voicePitch = 100;

	m_iBrassShell = PRECACHE_MODEL("models/shell.mdl");// brass shell
}

//=========================================================
// PainSound
//=========================================================
void CMassn::PainSound(void)
{
}

//=========================================================
// DeathSound 
//=========================================================
void CMassn::DeathSound(void)
{
}


//=========================================================
// CAssassinRepel - when triggered, spawns a monster_male_assassin
// repelling down a line.
//=========================================================

class CAssassinRepel : public CHGruntRepel
{
public:
	const char* TrooperName() {
		return "monster_male_assassin";
	}
};

LINK_ENTITY_TO_CLASS(monster_assassin_repel, CAssassinRepel)

class CDeadMassn : public CBaseMonster
{
public:
	void Spawn( void );
	int Classify( void ) { return CLASS_HUMAN_MILITARY; }

	void KeyValue( KeyValueData *pkvd );

	int m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char *m_szPoses[3];
};

const char *CDeadMassn::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadMassn::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_massassin_dead, CDeadMassn )

//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadMassn::Spawn( void )
{
	PRECACHE_MODEL( "models/massn.mdl" );
	SET_MODEL( ENT( pev ), "models/massn.mdl" );

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if( pev->sequence == -1 )
	{
		ALERT( at_console, "Dead massn with bad pose\n" );
	}

	// Corpses have less health
	pev->health = 8;

	// map old bodies onto new bodies
	switch( pev->body )
	{
	case 0:
		pev->body = 0;
		pev->skin = 0;
		break;
	case 1:
		pev->body = 2;
		pev->skin = 0;
		break;
	case 2:
		pev->body = 7;
		pev->skin = 0;
		break;
	case 3:
		pev->body = 8;
		pev->skin = 0;
		break;
	}

	MonsterInitDead();
}
