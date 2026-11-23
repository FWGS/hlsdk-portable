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
/*

===== turret.cpp ========================================================

*/

//
// TODO: 
//		Take advantage of new monster fields like m_hEnemy and get rid of that OFFSET() stuff
//		Revisit enemy validation stuff, maybe it's not necessary with the newest monster code
//

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"

#define TURRET_RANGE 1200
#define TURRET_TURNRATE 30		//angles per 0.1 second
#define TURRET_MAXWAIT 15		// seconds turret will stay active w/o a target
#define TURRET_MAXSPIN 5		// seconds turret barrel will spin w/o a target
#define TURRET_MACHINE_VOLUME 0.5

enum TURRET_ANIM
{
	TURRET_ANIM_NONE,
	TURRET_ANIM_FIRE,
	TURRET_ANIM_SPIN,
	TURRET_ANIM_DEPLOY,
	TURRET_ANIM_RETIRE,
	TURRET_ANIM_DIE
};

class CBaseTurret : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData *pkvd);
	void EXPORT TurretUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void TraceAttack(entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int Classify();
	int BloodColor() { return DONT_BLEED; }
	void GibMonster() {}
	void EXPORT ActiveThink();
	void EXPORT SearchThink();
	void EXPORT AutoSearchThink();
	void EXPORT TurretDeath();
	virtual void EXPORT SpinDownCall() { m_iSpin = 0; }
	virtual void EXPORT SpinUpCall() { m_iSpin = 1; }
	void EXPORT Deploy();
	void EXPORT Retire();
	void EXPORT Initialize();
	virtual void Ping();
	virtual void EyeOn();
	virtual void EyeOff();
	int Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];

	// other functions
	void SetTurretAnim(TURRET_ANIM anim);
	int MoveTurret();
	virtual void Shoot(Vector &, Vector &) {}

	float m_flMaxSpin;		// Max time to spin the barrel w/o a target
	int m_iSpin;
	CSprite *m_pEyeGlow;
	int m_eyeBrightness;
	int m_iDeployHeight;
	int m_iRetractHeight;
	int m_iMinPitch;
	int m_iBaseTurnRate;	// angles per second
	float m_fTurnRate;		// actual turn rate
	int m_iOrientation;		// 0 = floor, 1 = Ceiling
	int m_iOn;
	int m_fBeserk;			// Sometimes this bitch will just freak out
	int m_iAutoStart;		// true if the turret auto deploys when a target
							// enters its range

	Vector m_vecLastSight;
	float m_flLastSight;	// Last time we saw a target
	float m_flMaxWait;		// Max time to seach w/o a target
	int m_iSearchSpeed;		// Not Used!

	// movement
	float m_flStartYaw;
	Vector m_vecCurAngles;
	Vector m_vecGoalAngles;
	float m_flPingTime;	// Time until the next ping, used when searching
	float m_flSpinUpTime;	// Amount of time until the barrel should spin down when searching
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

TYPEDESCRIPTION CBaseTurret::m_SaveData[] =
{
	DEFINE_FIELD(CBaseTurret, m_flMaxSpin, FIELD_FLOAT),
	DEFINE_FIELD(CBaseTurret, m_iSpin, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_pEyeGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(CBaseTurret, m_eyeBrightness, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_iDeployHeight, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_iRetractHeight, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_iMinPitch, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_iBaseTurnRate, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_fTurnRate, FIELD_FLOAT),
	DEFINE_FIELD(CBaseTurret, m_iOrientation, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_fBeserk, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_iAutoStart, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_vecLastSight, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CBaseTurret, m_flLastSight, FIELD_TIME),
	DEFINE_FIELD(CBaseTurret, m_flMaxWait, FIELD_FLOAT),
	DEFINE_FIELD(CBaseTurret, m_iSearchSpeed, FIELD_INTEGER),
	DEFINE_FIELD(CBaseTurret, m_flStartYaw, FIELD_FLOAT),
	DEFINE_FIELD(CBaseTurret, m_vecCurAngles, FIELD_VECTOR),
	DEFINE_FIELD(CBaseTurret, m_vecGoalAngles, FIELD_VECTOR),
	DEFINE_FIELD(CBaseTurret, m_flPingTime, FIELD_TIME),
	DEFINE_FIELD(CBaseTurret, m_flSpinUpTime, FIELD_TIME)
};

IMPLEMENT_SAVERESTORE(CBaseTurret, CBaseMonster);

class CTurret : public CBaseTurret
{
public:
	void Spawn();
	void Precache();
	void SpinUpCall();
	void SpinDownCall();
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int Save(CSave &save);
	int Restore(CRestore &restore);
	static TYPEDESCRIPTION m_SaveData[];
private:
	int m_iStartSpin;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

TYPEDESCRIPTION CTurret::m_SaveData[] =
{
	DEFINE_FIELD(CTurret, m_iStartSpin, FIELD_INTEGER)
};

IMPLEMENT_SAVERESTORE(CTurret, CBaseTurret);

class CMiniTurret : public CBaseTurret
{
public:
	void Spawn();
	void Precache();
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
};




//=========================================================
// Sentry gun - smallest turret, placed near grunt entrenchments
//=========================================================
class CSentry : public CBaseTurret
{
public:
	void Spawn();
	void Precache();
	// other functions
	void Shoot(Vector &vecSrc, Vector &vecDirToEnemy);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	void EXPORT SentryTouch(CBaseEntity *pOther);
	void EXPORT SentryDeath();
};

LINK_ENTITY_TO_CLASS(monster_turret, CTurret);
LINK_ENTITY_TO_CLASS(monster_miniturret, CMiniTurret);
LINK_ENTITY_TO_CLASS(monster_sentry, CSentry);

void CBaseTurret::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "maxsleep"))
		m_flMaxWait = (float)atof(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "orientation"))
		m_iOrientation = atoi(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "searchspeed"))
		m_iSearchSpeed = atoi(pkvd->szValue);
	else if (FStrEq(pkvd->szKeyName, "turnrate"))
		m_iBaseTurnRate = atoi(pkvd->szValue);
	else if (!FStrEq(pkvd->szKeyName, "style") && !FStrEq(pkvd->szKeyName, "height") &&
		!FStrEq(pkvd->szKeyName, "value1") && !FStrEq(pkvd->szKeyName, "value2") &&
		!FStrEq(pkvd->szKeyName, "value3"))
	{
		CBaseMonster::KeyValue(pkvd);
		return;
	}
	pkvd->fHandled = TRUE;
}

void CBaseTurret::Precache()
{
	PRECACHE_SOUND("turret/tu_fire1.wav");
	PRECACHE_SOUND("turret/tu_ping.wav");
	PRECACHE_SOUND("turret/tu_active2.wav");
	PRECACHE_SOUND("turret/tu_die.wav");
	PRECACHE_SOUND("turret/tu_die2.wav");
	PRECACHE_SOUND("turret/tu_die3.wav");
	// PRECACHE_SOUND ("turret/tu_retract.wav"); // just use deploy sound to save memory
	PRECACHE_SOUND("turret/tu_deploy.wav");
	PRECACHE_SOUND("turret/tu_spinup.wav");
	PRECACHE_SOUND("turret/tu_spindown.wav");
	PRECACHE_SOUND("turret/tu_search.wav");
	PRECACHE_SOUND("turret/tu_alert.wav");
}

void CBaseTurret::Spawn()
{
	Precache();
	pev->nextthink = gpGlobals->time + 1;
	pev->movetype = MOVETYPE_FLY;
	pev->sequence = 0;
	pev->frame = 0;
	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_AIM;
	pev->flags |= FL_MONSTER;
	SetUse(&CBaseTurret::TurretUse);
	if ((pev->spawnflags & (SF_MONSTER_TURRET_AUTOACTIVATE | SF_MONSTER_TURRET_STARTINACTIVE)) == SF_MONSTER_TURRET_AUTOACTIVATE)
		m_iAutoStart = TRUE;
	ResetSequenceInfo();
	SetBoneController(0, 0);
	SetBoneController(1, 0);
	m_flFieldOfView = -1;
}

void CTurret::Precache()
{
	CBaseTurret::Precache();
	PRECACHE_MODEL("models/turret.mdl");
	PRECACHE_MODEL("sprites/flare3.spr");
}

void CTurret::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/turret.mdl");
	pev->health = gSkillData.turretHealth;
	m_HackedGunPos = Vector(0, 0, 12.75);
	pev->view_ofs.z = 12.75;
	m_flMaxSpin = TURRET_MAXSPIN;
	CBaseTurret::Spawn();
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch = -15;
	UTIL_SetSize(pev, Vector(-32, -32, -16), Vector(32, 32, 16));
	SetThink(&CBaseTurret::Initialize);
	m_pEyeGlow = CSprite::SpriteCreate("sprites/flare3.spr", pev->origin, FALSE);
	m_pEyeGlow->SetTransparency(kRenderGlow, 255, 0, 0, 0, kRenderFxNoDissipation);
	m_pEyeGlow->SetAttachment(edict(), 2);
	m_eyeBrightness = 0;
	pev->nextthink = gpGlobals->time + 0.3;
}

void CMiniTurret::Precache()
{
	CBaseTurret::Precache();
	PRECACHE_MODEL("models/miniturret.mdl");
	PRECACHE_SOUND("weapons/hks1.wav");
	PRECACHE_SOUND("weapons/hks2.wav");
	PRECACHE_SOUND("weapons/hks3.wav");
}

void CMiniTurret::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/miniturret.mdl");
	pev->health = gSkillData.miniturretHealth;
	m_HackedGunPos = Vector(0, 0, 12.75);
	pev->view_ofs.z = 12.75;
	m_flMaxSpin = 0;
	CBaseTurret::Spawn();
	m_iRetractHeight = 16;
	m_iDeployHeight = 32;
	m_iMinPitch = -15;
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16));
	SetThink(&CBaseTurret::Initialize);
	pev->nextthink = gpGlobals->time + 0.3;
}

void CSentry::Precache()
{
	CBaseTurret::Precache();
	PRECACHE_MODEL("models/sentry.mdl");
}

void CSentry::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/sentry.mdl");
	pev->health = gSkillData.sentryHealth;
	m_HackedGunPos = Vector(0, 0, 48);
	pev->view_ofs.z = 48;
	m_flMaxWait = 1000000;
	m_flMaxSpin = 1000000;
	CBaseTurret::Spawn();
	m_iRetractHeight = 64;
	m_iDeployHeight = 64;
	m_iMinPitch = -60;
	UTIL_SetSize(pev, Vector(-16, -16, -64), Vector(16, 16, 64));
	SetTouch(&CSentry::SentryTouch);
	SetThink(&CBaseTurret::Initialize);
	pev->nextthink = gpGlobals->time + 0.3;
}

void CBaseTurret::Initialize()
{
	m_iOn = 0;
	m_fBeserk = 0;
	m_iSpin = 0;
	SetBoneController(0, 0);
	SetBoneController(1, 0);
	if (!m_iBaseTurnRate)
		m_iBaseTurnRate = TURRET_TURNRATE;
	if (!m_flMaxWait)
		m_flMaxWait = TURRET_MAXWAIT;
	m_flStartYaw = pev->angles.y;
	if (m_iOrientation == 1)
	{
		pev->idealpitch = 180;
		pev->angles.x = 180;
		pev->view_ofs.z = -pev->view_ofs.z;
		pev->effects |= EF_INVLIGHT;
		pev->angles.y += 180;
		if (pev->angles.y > 360)
			pev->angles.y -= 360;
	}
	m_vecGoalAngles.x = 0;
	if (m_iAutoStart)
	{
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(&CBaseTurret::AutoSearchThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	else
		SetThink(&CBaseEntity::SUB_DoNothing);
}

void CBaseTurret::TurretUse(CBaseEntity *, CBaseEntity *, USE_TYPE useType, float)
{
	if (!ShouldToggle(useType, m_iOn))
		return;
	pev->nextthink = gpGlobals->time + 0.1;
	if (m_iOn)
	{
		m_hEnemy = NULL;
		m_iAutoStart = 0;
		//!!!! this should spin down first!!BUGBUG
		SetThink(&CBaseTurret::Retire);
	}
	else
	{

		// if the turret is flagged as an autoactivate turret, re-enable it's ability open self.
		if (pev->spawnflags & SF_MONSTER_TURRET_AUTOACTIVATE)
			m_iAutoStart = 1;
		SetThink(&CBaseTurret::Deploy);
	}
}

int CBaseTurret::Classify()
{
	return (m_iOn || m_iAutoStart) ? CLASS_MACHINE : CLASS_NONE;
}

void CBaseTurret::SetTurretAnim(TURRET_ANIM anim)
{
	if (pev->sequence == anim)
		return;
	if (!((pev->sequence == TURRET_ANIM_FIRE && anim == TURRET_ANIM_SPIN) ||
		(pev->sequence == TURRET_ANIM_SPIN && anim == TURRET_ANIM_FIRE)))
		pev->frame = 0;
	pev->sequence = anim;
	ResetSequenceInfo();
	if (anim == TURRET_ANIM_RETIRE)
	{
		pev->frame = 255;
		pev->framerate = -1;
	}
	else if (anim == TURRET_ANIM_DIE)
		pev->framerate = 1;
}

void CBaseTurret::Deploy()
{
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance();
	if (pev->sequence != TURRET_ANIM_DEPLOY)
	{
		m_iOn = 1;
		SetTurretAnim(TURRET_ANIM_DEPLOY);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
		SUB_UseTargets(this, USE_ON, 0);
	}
	if (m_fSequenceFinished)
	{
		pev->maxs.z = (float)m_iDeployHeight;
		pev->mins.z = (float)-m_iDeployHeight;
		UTIL_SetSize(pev, pev->mins, pev->maxs);
		m_vecCurAngles.x = 0;
		m_vecCurAngles.y = UTIL_AngleMod(pev->angles.y + (m_iOrientation == 1 ? 180 : 0));
		SetTurretAnim(TURRET_ANIM_SPIN);
		pev->framerate = 0;
		SetThink(&CBaseTurret::SearchThink);
	}
	m_flLastSight = gpGlobals->time + m_flMaxWait;
}

void CBaseTurret::Retire()
{
	// make the turret level
	m_vecGoalAngles.x = 0;
	m_vecGoalAngles.y = m_flStartYaw;
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance();
	EyeOff();
	if (MoveTurret())
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
		return;
	}
	if (m_iSpin)
		SpinDownCall();
	else if (pev->sequence != TURRET_ANIM_RETIRE)
	{
		SetTurretAnim(TURRET_ANIM_RETIRE);
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "turret/tu_deploy.wav", TURRET_MACHINE_VOLUME, ATTN_NORM, 0, 120);
		SUB_UseTargets(this, USE_OFF, 0);
	}
	else if (m_fSequenceFinished)
	{
		m_iOn = 0;
		m_flLastSight = 0;
		SetTurretAnim(TURRET_ANIM_NONE);
		pev->maxs.z = (float)m_iRetractHeight;
		pev->mins.z = (float)-m_iRetractHeight;
		UTIL_SetSize(pev, pev->mins, pev->maxs);
		if (m_iAutoStart)
		{
			SetThink(&CBaseTurret::AutoSearchThink);
			pev->nextthink = gpGlobals->time + 0.1;
		}
		else
			SetThink(&CBaseEntity::SUB_DoNothing);
	}
}

void CBaseTurret::AutoSearchThink()
{
	// ensure rethink
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.3;
	if (m_hEnemy != NULL && !m_hEnemy->IsAlive())
		m_hEnemy = NULL;

	// Acquire Target

	if (m_hEnemy == NULL)
	{
		Look(TURRET_RANGE);
		m_hEnemy = BestVisibleEnemy();
	}
	if (m_hEnemy != NULL)
	{
		SetThink(&CBaseTurret::Deploy);
		EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_alert.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
	}
}

void CBaseTurret::SearchThink()
{
	// ensure rethink
	SetTurretAnim(TURRET_ANIM_SPIN);
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
	if (!m_flSpinUpTime && m_flMaxSpin)
		m_flSpinUpTime = gpGlobals->time + m_flMaxSpin;
	Ping();
	if (m_hEnemy != NULL && !m_hEnemy->IsAlive())
		m_hEnemy = NULL;


	// Acquire Target
	if (m_hEnemy == NULL)
	{
		Look(TURRET_RANGE);
		m_hEnemy = BestVisibleEnemy();
	}

	// If we've found a target, spin up the barrel and start to attack
	if (m_hEnemy != NULL)
	{
		m_flLastSight = 0;
		m_flSpinUpTime = 0;
		SetThink(&CBaseTurret::ActiveThink);
	}
	else
	{
		// Are we out of time, do we need to retract?
		if (gpGlobals->time > m_flLastSight)
		{
			//Before we retrace, make sure that we are spun down.
			m_flLastSight = 0;
			m_flSpinUpTime = 0;
			SetThink(&CBaseTurret::Retire);
		}
		else if (m_flSpinUpTime && gpGlobals->time > m_flSpinUpTime)
			SpinDownCall();
		m_vecGoalAngles.y = (float)(m_vecGoalAngles.y + 0.1 * m_fTurnRate);
		if (m_vecGoalAngles.y >= 360)
			m_vecGoalAngles.y -= 360;
		MoveTurret();
	}
}

void CBaseTurret::ActiveThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance();
	if (!m_iOn || m_hEnemy == NULL)
	{
		m_hEnemy = NULL;
		m_flLastSight = gpGlobals->time + m_flMaxWait;
		SetThink(&CBaseTurret::SearchThink);
		return;
	}

	// if it's dead, look for something new
	if (!m_hEnemy->IsAlive())
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 0.5;
		else if (gpGlobals->time > m_flLastSight)
		{
			m_hEnemy = NULL;
			m_flLastSight = gpGlobals->time + m_flMaxWait;
			SetThink(&CBaseTurret::SearchThink);
			return;
		}
	}
	Vector vecMid = pev->origin + pev->view_ofs;
	Vector vecMidEnemy = m_hEnemy->BodyTarget(vecMid);
	BOOL fVisible = FBoxVisible(pev, m_hEnemy->pev, vecMidEnemy);
	Vector vecDirToEnemy = vecMidEnemy - vecMid;
	float flDistToEnemy = vecDirToEnemy.Length();
	Vector vec = UTIL_VecToAngles(vecDirToEnemy);
	if (!fVisible || flDistToEnemy > TURRET_RANGE)
	{
		if (!m_flLastSight)
			m_flLastSight = gpGlobals->time + 0.5;
		else if (gpGlobals->time > m_flLastSight)
		{
			m_hEnemy = NULL;
			m_flLastSight = gpGlobals->time + m_flMaxWait;
			SetThink(&CBaseTurret::SearchThink);
			return;
		}
		fVisible = FALSE;
	}
	else
		m_vecLastSight = vecMidEnemy;
	UTIL_MakeAimVectors(m_vecCurAngles);
	if (m_iSpin && (DotProduct(vecDirToEnemy.Normalize(), gpGlobals->v_forward) > 0.866 || m_fBeserk))
	{
		Vector vecSrc, vecAng;
		GetAttachment(0, vecSrc, vecAng);
		SetTurretAnim(TURRET_ANIM_FIRE);
		Shoot(vecSrc, gpGlobals->v_forward);
	}
	else
		SetTurretAnim(TURRET_ANIM_SPIN);

	//move the gun
	if (m_fBeserk)
	{
		if (RANDOM_LONG(0, 9) == 0)
		{
			m_vecGoalAngles.y = RANDOM_FLOAT(0, 360);
			m_vecGoalAngles.x = RANDOM_FLOAT(0, 90) - 90 * m_iOrientation;
			TakeDamage(pev, pev, 1, DMG_GENERIC); // don't beserk forever
			return;
		}
	}
	else if (fVisible)
	{
		if (vec.y > 360) vec.y -= 360;
		if (vec.y < 0) vec.y += 360;
		if (vec.x < -180) vec.x += 360;
		if (vec.x > 180) vec.x -= 360;
		if (m_iOrientation == 0)
		{
			if (vec.x > 90) vec.x = 90;
			else if (vec.x < m_iMinPitch) vec.x = (float)m_iMinPitch;
		}
		else
		{
			if (vec.x < -90) vec.x = -90;
			else if (vec.x > -m_iMinPitch) vec.x = (float)-m_iMinPitch;
		}

		// ALERT(at_console, "->[%.2f]\n", vec.x);

		m_vecGoalAngles.y = vec.y;
		m_vecGoalAngles.x = vec.x;
	}
	SpinUpCall();
	MoveTurret();
}

void CTurret::SpinUpCall()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	// Are we already spun up? If not start the two stage process.
	if (!m_iSpin)
	{
		SetTurretAnim(TURRET_ANIM_SPIN);
		// for the first pass, spin up the the barrel
		if (!m_iStartSpin)
		{
			pev->nextthink = gpGlobals->time + 1.0; // spinup delay
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_spinup.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
			m_iStartSpin = 1;
			pev->framerate = 0.1;
		}
		// after the barrel is spun up, turn on the hum
		else if (pev->framerate >= 1)
		{
			pev->nextthink = gpGlobals->time + 0.1; // retarget delay
			EMIT_SOUND(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
			m_iStartSpin = 0;
			m_iSpin = 1;
		}
		else
			pev->framerate = (float)(pev->framerate + 0.075);
	}
	if (m_iSpin)
		SetThink(&CBaseTurret::ActiveThink);
}

void CTurret::SpinDownCall()
{
	if (!m_iSpin)
		return;
	SetTurretAnim(TURRET_ANIM_SPIN);
	if (pev->framerate == 1)
	{
		STOP_SOUND(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav");
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "turret/tu_spindown.wav", TURRET_MACHINE_VOLUME, ATTN_NORM);
	}
	pev->framerate = (float)(pev->framerate - 0.02);
	if (pev->framerate <= 0)
	{
		pev->framerate = 0;
		m_iSpin = 0;
	}
}

void CBaseTurret::Ping()
{
	if (!m_flPingTime)
		m_flPingTime = gpGlobals->time + 1;
	else if (gpGlobals->time >= m_flPingTime)
	{
		m_flPingTime = gpGlobals->time + 1;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "turret/tu_ping.wav", 1, 0.5);
		EyeOn();
	}
	else if (m_eyeBrightness > 0)
		EyeOff();
}

void CBaseTurret::EyeOn()
{
	if (m_pEyeGlow)
	{
		m_eyeBrightness = 255;
		m_pEyeGlow->SetBrightness(m_eyeBrightness);
	}
}

void CBaseTurret::EyeOff()
{
	if (m_pEyeGlow && m_eyeBrightness > 0)
	{
		m_eyeBrightness -= 30;
		if (m_eyeBrightness < 0) m_eyeBrightness = 0;
		m_pEyeGlow->SetBrightness(m_eyeBrightness);
	}
}

void CTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	FireBullets(1, vecSrc, vecDirToEnemy, VECTOR_CONE_10DEGREES, TURRET_RANGE, BULLET_MONSTER_12MM, 1);
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "turret/tu_fire1.wav", 1, 0.6);
	pev->effects |= EF_MUZZLEFLASH;
}

void CMiniTurret::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	FireBullets(1, vecSrc, vecDirToEnemy, VECTOR_CONE_10DEGREES, TURRET_RANGE, BULLET_MONSTER_9MM, 1);
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks3.wav", 1, ATTN_NORM); break;
	}
	pev->effects |= EF_MUZZLEFLASH;
}

void CSentry::Shoot(Vector &vecSrc, Vector &vecDirToEnemy)
{
	FireBullets(1, vecSrc, vecDirToEnemy, VECTOR_CONE_10DEGREES, TURRET_RANGE, BULLET_MONSTER_MP5, 1);
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM); break;
	case 1: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM); break;
	case 2: EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hks3.wav", 1, ATTN_NORM); break;
	}
	pev->effects |= EF_MUZZLEFLASH;
}

void CBaseTurret::TraceAttack(entvars_t *pevAttacker, float flDamage, Vector, TraceResult *ptr, int bitsDamageType)
{
	if (ptr->iHitgroup == 10)
	{
		// hit armor
		if (pev->dmgtime != gpGlobals->time || RANDOM_LONG(0, 10) < 1)
		{
			UTIL_Ricochet(ptr->vecEndPos, RANDOM_FLOAT(1, 2));
			pev->dmgtime = gpGlobals->time;
		}
		flDamage = 0.1; // don't hurt the monster much, but allow bits_COND_LIGHT_DAMAGE to be generated
	}
	if (pev->takedamage)
		AddMultiDamage(pevAttacker, this, flDamage, bitsDamageType);
}

// take damage. bitsDamageType indicates type of damage sustained, ie: DMG_BULLET

int CBaseTurret::TakeDamage(entvars_t *, entvars_t *, float flDamage, int)
{
	if (!pev->takedamage)
		return 0;
	if (!m_iOn)
		flDamage = (float)(flDamage / 10.0);
	Vector vecSrc, vecAng;
	GetAttachment(1, vecSrc, vecAng);
	if (RANDOM_LONG(0, 10) > 5)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(vecSrc.x + RANDOM_FLOAT(-16, 16));
			WRITE_COORD(vecSrc.y + RANDOM_FLOAT(-16, 16));
			WRITE_COORD(vecSrc.z - 32);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(5);
			WRITE_BYTE(8);
		MESSAGE_END();
	}
	UTIL_Sparks(vecSrc);
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;
		pev->flags &= ~FL_MONSTER;
		SetUse(NULL);
		SetThink(&CBaseTurret::TurretDeath);
		SUB_UseTargets(this, USE_ON, 0); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;
		return 0;
	}
	if (pev->health <= 10 && m_iOn)
	{
		m_fBeserk = 1;
		SetThink(&CBaseTurret::SearchThink);
	}
	return 1;
}

int CSentry::TakeDamage(entvars_t *, entvars_t *, float flDamage, int)
{
	if (!pev->takedamage)
		return 0;
	if (!m_iOn)
	{
		SetThink(&CBaseTurret::Deploy);
		SetUse(NULL);
		pev->nextthink = gpGlobals->time + 0.1;
	}
	Vector vecSrc, vecAng;
	GetAttachment(1, vecSrc, vecAng);
	if (pev->dmgtime + RANDOM_FLOAT(0, 2) > gpGlobals->time)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(vecSrc.x + RANDOM_FLOAT(-16, 16));
			WRITE_COORD(vecSrc.y + RANDOM_FLOAT(-16, 16));
			WRITE_COORD(vecSrc.z - 32);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(10);
			WRITE_BYTE(8);
		MESSAGE_END();
	}
	if (pev->dmgtime + RANDOM_FLOAT(0, 8) > gpGlobals->time)
		UTIL_Sparks(vecSrc);
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		pev->health = 0;
		pev->takedamage = DAMAGE_NO;
		pev->dmgtime = gpGlobals->time;
		pev->flags &= ~FL_MONSTER;
		SetUse(NULL);
		SetThink(&CSentry::SentryDeath);
		SUB_UseTargets(this, USE_ON, 0); // wake up others
		pev->nextthink = gpGlobals->time + 0.1;
		return 0;
	}
	return 1;
}

void CSentry::SentryTouch(CBaseEntity *pOther)
{
	if (pOther && (pOther->IsPlayer() || (pOther->pev->flags & FL_MONSTER)))
		TakeDamage(pOther->pev, pOther->pev, 0, 0);
}

void CBaseTurret::TurretDeath()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
	if (pev->deadflag != DEAD_DEAD)
	{
		pev->deadflag = DEAD_DEAD;
		float flRndSound = RANDOM_FLOAT(0, 1);
		if (flRndSound <= 0.33)
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die.wav", 1, ATTN_NORM);
		else if (flRndSound <= 0.66)
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die2.wav", 1, ATTN_NORM);
		else
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die3.wav", 1, ATTN_NORM);
		STOP_SOUND(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav");
		m_vecGoalAngles.x = m_iOrientation == 0 ? -15.0f : -90.0f;
		SetTurretAnim(TURRET_ANIM_DIE);
		EyeOn();
	}
	EyeOff();
	if (pev->dmgtime + RANDOM_FLOAT(0, 2) > gpGlobals->time)
	{
		// lots of smoke
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x));
			WRITE_COORD(RANDOM_FLOAT(pev->absmin.y, pev->absmax.y));
			WRITE_COORD(pev->origin.z - m_iOrientation * 64);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(25); // scale * 10
			WRITE_BYTE(10 - m_iOrientation * 5); // framerate
		MESSAGE_END();
	}
	if (pev->dmgtime + RANDOM_FLOAT(0, 5) > gpGlobals->time)
	{
		float y = RANDOM_FLOAT(pev->absmin.y, pev->absmax.y);
		Vector vecSrc(RANDOM_FLOAT(pev->absmin.x, pev->absmax.x), y, 0);
		vecSrc.z = m_iOrientation == 0 ? RANDOM_FLOAT(pev->origin.z, pev->absmax.z) : RANDOM_FLOAT(pev->absmin.z, pev->origin.z);
		UTIL_Sparks(vecSrc);
	}
	if (m_fSequenceFinished && !MoveTurret() && gpGlobals->time > pev->dmgtime + 5)
	{
		pev->framerate = 0;
		SetThink(NULL);
	}
}

void CSentry::SentryDeath()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;
	if (pev->deadflag != DEAD_DEAD)
	{
		pev->deadflag = DEAD_DEAD;
		float flRndSound = RANDOM_FLOAT(0, 1);
		if (flRndSound <= 0.33)
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die.wav", 1, ATTN_NORM);
		else if (flRndSound <= 0.66)
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die2.wav", 1, ATTN_NORM);
		else
			EMIT_SOUND(ENT(pev), CHAN_BODY, "turret/tu_die3.wav", 1, ATTN_NORM);
		STOP_SOUND(ENT(pev), CHAN_STATIC, "turret/tu_active2.wav");
		SetBoneController(0, 0);
		SetBoneController(1, 0);
		SetTurretAnim(TURRET_ANIM_DIE);
		pev->solid = SOLID_NOT;
		pev->angles.y = UTIL_AngleMod(pev->angles.y + RANDOM_LONG(0, 2) * 120);
		EyeOn();
	}
	EyeOff();
	Vector vecSrc, vecAng;
	GetAttachment(1, vecSrc, vecAng);
	if (pev->dmgtime + RANDOM_FLOAT(0, 2) > gpGlobals->time)
	{
		// lots of smoke
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_SMOKE);
			WRITE_COORD(vecSrc.x + RANDOM_FLOAT(-16, 16));
			WRITE_COORD(vecSrc.y + RANDOM_FLOAT(-16, 16));
			WRITE_COORD(vecSrc.z - 32);
			WRITE_SHORT(g_sModelIndexSmoke);
			WRITE_BYTE(15); // scale * 10
			WRITE_BYTE(8); // framerate
		MESSAGE_END();
	}
	if (pev->dmgtime + RANDOM_FLOAT(0, 8) > gpGlobals->time)
		UTIL_Sparks(vecSrc);
	if (m_fSequenceFinished && gpGlobals->time > pev->dmgtime + 5)
	{
		pev->framerate = 0;
		SetThink(NULL);
	}
}

int CBaseTurret::MoveTurret()
{
	int state = 0;
	// any x movement?

	if (m_vecCurAngles.x != m_vecGoalAngles.x)
	{
		float flDir = m_vecGoalAngles.x > m_vecCurAngles.x ? 1.0f : -1.0f;
		m_vecCurAngles.x = (float)(m_vecCurAngles.x + 0.1 * m_fTurnRate * flDir);

		// if we started below the goal, and now we're past, peg to goal
		if (flDir == 1)
		{
			if (m_vecCurAngles.x > m_vecGoalAngles.x) m_vecCurAngles.x = m_vecGoalAngles.x;
		}
		else if (m_vecCurAngles.x < m_vecGoalAngles.x)
			m_vecCurAngles.x = m_vecGoalAngles.x;
		SetBoneController(1, m_iOrientation == 0 ? -m_vecCurAngles.x : m_vecCurAngles.x);
		state = 1;
	}
	if (m_vecCurAngles.y != m_vecGoalAngles.y)
	{
		float flDir = m_vecGoalAngles.y > m_vecCurAngles.y ? 1.0f : -1.0f;
		float flDist = (float)fabs(m_vecGoalAngles.y - m_vecCurAngles.y);
		if (flDist > 180)
		{
			flDist = 360 - flDist;
			flDir = -flDir;
		}
		if (flDist > 30)
		{
			if (m_fTurnRate < m_iBaseTurnRate * 10) m_fTurnRate += m_iBaseTurnRate;
		}
		else if (m_fTurnRate > 45)
			m_fTurnRate -= m_iBaseTurnRate;
		else
			m_fTurnRate += m_iBaseTurnRate;
		m_vecCurAngles.y = (float)(m_vecCurAngles.y + 0.1 * m_fTurnRate * flDir);
		if (m_vecCurAngles.y < 0) m_vecCurAngles.y += 360;
		else if (m_vecCurAngles.y >= 360) m_vecCurAngles.y -= 360;
		if (flDist < 0.05 * m_iBaseTurnRate) m_vecCurAngles.y = m_vecGoalAngles.y;
		SetBoneController(0, m_iOrientation == 0 ? m_vecCurAngles.y - pev->angles.y : pev->angles.y - 180 - m_vecCurAngles.y);
		state = 1;
	}
	if (!state) m_fTurnRate = (float)m_iBaseTurnRate;

	//ALERT(at_console, "(%.2f, %.2f)->(%.2f, %.2f)\n", m_vecCurAngles.x, 
	//	m_vecCurAngles.y, m_vecGoalAngles.x, m_vecGoalAngles.y);
	return state;
}
