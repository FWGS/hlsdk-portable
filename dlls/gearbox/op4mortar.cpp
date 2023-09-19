#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"
#include "monsters.h"
#include "player.h"
#include "soundent.h"
#include "decals.h"

class CMortarShell : public CGrenade
{
public:
	void Precache();
	void EXPORT BurnThink();
	void EXPORT MortarExplodeTouch(CBaseEntity *pOther);
	void Spawn();
	void EXPORT FlyThink();
	int Save(CSave &save);
	int Restore(CRestore &restore);
	static CMortarShell *CreateMortarShell(Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, int velocity);

	static TYPEDESCRIPTION m_SaveData[];
	int m_iTrail;
	BOOL m_iSoundedOff;
	float m_flIgniteTime;
	float m_velocity;
};

LINK_ENTITY_TO_CLASS(mortar_shell, CMortarShell)

TYPEDESCRIPTION CMortarShell::m_SaveData[] =
{
	DEFINE_FIELD(CMortarShell, m_velocity, FIELD_FLOAT),
	DEFINE_FIELD(CMortarShell, m_flIgniteTime, FIELD_FLOAT),
	DEFINE_FIELD(CMortarShell, m_iSoundedOff, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CMortarShell, CGrenade)

void CMortarShell::Precache()
{
	PRECACHE_MODEL("models/mortarshell.mdl");
	m_iTrail = PRECACHE_MODEL("sprites/wep_smoke_01.spr");
	PRECACHE_SOUND("weapons/ofmortar.wav");
}

void CMortarShell::Spawn()
{
	Precache();

	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL(edict(), "models/mortarshell.mdl");

	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	UTIL_SetOrigin(pev, pev->origin);
	pev->classname = MAKE_STRING("mortar_shell");

	SetThink(&CMortarShell::BurnThink);
	SetTouch(&CMortarShell::MortarExplodeTouch);

	UTIL_MakeVectors(pev->angles);

	pev->velocity = -(gpGlobals->v_forward * m_velocity);
	pev->gravity = 1;

	pev->dmg = gSkillData.plrDmgRPG * 2;

	pev->nextthink = gpGlobals->time + 0.01;
	m_flIgniteTime = gpGlobals->time;
	m_iSoundedOff = FALSE;
}

void CMortarShell::MortarExplodeTouch(CBaseEntity *pOther)
{
	pev->enemy = pOther->edict();

	const Vector direction = pev->velocity.Normalize();

	const Vector vecSpot = pev->origin - direction * 32;

	TraceResult tr;
	UTIL_TraceLine(vecSpot, vecSpot + direction * 64, ignore_monsters, edict(), &tr);

	pev->model = 0;

	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;

	if (tr.flFraction != 1.0f)
	{
		pev->origin = 0.6f * ((pev->dmg - 24.0f) * tr.vecPlaneNormal) + tr.vecEndPos;
	}

	const int contents = UTIL_PointContents(pev->origin);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);

	if (contents == CONTENTS_WATER)
		WRITE_SHORT(g_sModelIndexWExplosion);
	else
		WRITE_SHORT(g_sModelIndexFireball);

	WRITE_BYTE(static_cast<int>((pev->dmg - 50.0) * 5.0));
	WRITE_BYTE(10);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 1024, 3.0);

	entvars_t* pOwner = VARS(pev->owner);
	pev->owner = NULL;

	RadiusDamage(pev, pOwner, pev->dmg, CLASS_NONE, 64);

	if (RANDOM_FLOAT(0, 1) >= 0.5)
		UTIL_DecalTrace(&tr, DECAL_SCORCH2);
	else
		UTIL_DecalTrace(&tr, DECAL_SCORCH1);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(edict(), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(edict(), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(edict(), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM);
		break;
	}

	pev->effects |= EF_NODRAW;

	SetThink(&CMortarShell::Smoke);

	pev->velocity = g_vecZero;

	pev->nextthink = gpGlobals->time + 0.3;

	if (contents != CONTENTS_WATER)
	{
		const int sparkCount = RANDOM_LONG(0, 3);

		for (int i = 0; i < sparkCount; ++i)
		{
			CBaseEntity::Create("spark_shower", pev->origin, tr.vecPlaneNormal);
		}
	}
}


void CMortarShell::BurnThink()
{
	pev->angles = UTIL_VecToAngles(pev->velocity);

	pev->angles.x -= 90;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(0);
	WRITE_COORD(0);
	WRITE_COORD(1);
	WRITE_SHORT(m_iTrail);
	WRITE_BYTE(1);
	WRITE_BYTE(12);
	WRITE_BYTE(120);
	MESSAGE_END();

	if (gpGlobals->time > m_flIgniteTime + 0.2)
	{
		SetThink(&CMortarShell::FlyThink);
	}

	pev->nextthink = gpGlobals->time + 0.01;
}

void CMortarShell::FlyThink()
{
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->angles.x -= 90.0f;

	if(pev->velocity.z < 20.0f && !m_iSoundedOff)
	{
		m_iSoundedOff = TRUE;
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/ofmortar.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NONE);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

CMortarShell *CMortarShell::CreateMortarShell(Vector vecOrigin, Vector vecAngles, CBaseEntity *pOwner, int velocity)
{
	CMortarShell *pShell = GetClassPtr( (CMortarShell *)NULL );
	UTIL_SetOrigin(pShell->pev, vecOrigin);

	pShell->pev->angles = vecAngles;
	pShell->m_velocity = velocity;

	pShell->Spawn();

	pShell->pev->owner = pOwner->edict();

	return pShell;
}

#define SF_MORTAR_ACTIVE (1 << 0)
#define SF_MORTAR_LINE_OF_SIGHT (1 << 4)
#define SF_MORTAR_CONTROLLABLE (1 << 5)

class COp4Mortar : public CBaseMonster
{
public:
	void Spawn();
	virtual int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void Precache();
	void KeyValue(KeyValueData *pvkd);
	void UpdatePosition(float direction, int controller);
	void AIUpdatePosition();
	virtual int Save(CSave &save);
	virtual int Restore(CRestore &restore);
	CBaseEntity *FindTarget();
	void EXPORT MortarThink();
	static TYPEDESCRIPTION m_SaveData[];
	int ObjectCaps() { return 0; }
	void PlaySound();

	int d_x;
	int d_y;
	float m_lastupdate;
	int m_direction;
	Vector m_start;
	Vector m_end;
	int m_velocity;
	int m_hmin;
	int m_hmax;
	float m_fireLast;
	float m_maxRange;
	float m_minRange;
	int m_iEnemyType;
	float m_fireDelay;
	float m_trackDelay;
	BOOL m_tracking;
	float m_zeroYaw;
	Vector m_vGunAngle;
	Vector m_vIdealGunVector;
	Vector m_vIdealGunAngle;

	float m_lastTimePlayedSound;
};

LINK_ENTITY_TO_CLASS(op4mortar, COp4Mortar)

TYPEDESCRIPTION	COp4Mortar::m_SaveData[] =
{
	DEFINE_FIELD(COp4Mortar, d_x, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, d_y, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_lastupdate, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_direction, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_start, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_end, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_velocity, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_hmin, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_hmax, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_fireLast, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_maxRange, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_minRange, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_iEnemyType, FIELD_INTEGER),
	DEFINE_FIELD(COp4Mortar, m_fireDelay, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_trackDelay, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_tracking, FIELD_BOOLEAN),
	DEFINE_FIELD(COp4Mortar, m_zeroYaw, FIELD_FLOAT),
	DEFINE_FIELD(COp4Mortar, m_vGunAngle, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_vIdealGunVector, FIELD_VECTOR),
	DEFINE_FIELD(COp4Mortar, m_vIdealGunAngle, FIELD_VECTOR),
};

IMPLEMENT_SAVERESTORE( COp4Mortar, CBaseMonster )


void COp4Mortar::Precache()
{
	PRECACHE_MODEL("models/mortar.mdl");
	PRECACHE_SOUND("weapons/mortarhit.wav");
	PRECACHE_SOUND("player/pl_grate1.wav");
	UTIL_PrecacheOther("mortar_shell");
}

void COp4Mortar::Spawn()
{
	Precache();

	UTIL_SetOrigin(pev, pev->origin);

	SET_MODEL(edict(), "models/mortar.mdl");

	pev->health = 1;
	pev->sequence = LookupSequence("idle");

	ResetSequenceInfo();

	pev->frame = 0;
	pev->framerate = 1;

	m_tracking = FALSE;

	if (m_fireDelay < 0.5)
		m_fireDelay = 5;

	if (m_minRange == 0)
		m_minRange = 128;

	if (m_maxRange == 0)
		m_maxRange = 2048;

	InitBoneControllers();

	m_vGunAngle = g_vecZero;

	m_lastupdate = gpGlobals->time;

	m_zeroYaw = UTIL_AngleMod(pev->angles.y + 180.0);

	m_fireLast = gpGlobals->time;
	m_trackDelay = gpGlobals->time;

	m_hEnemy = NULL;

	pev->nextthink = gpGlobals->time + 0.01;
	SetThink(&COp4Mortar::MortarThink);
}

void COp4Mortar::PlaySound()
{
	if (gpGlobals->time > m_lastTimePlayedSound + 0.12f)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/pl_grate1.wav", 1.0f, ATTN_NORM);
		m_lastTimePlayedSound = gpGlobals->time + 0.12f;
	}
}

void COp4Mortar::UpdatePosition(float direction, int controller)
{
	if (gpGlobals->time - m_lastupdate >= 0.06)
	{
		switch (controller)
		{
		case 0:
			d_x = 3 * direction;
			break;

		case 1:
			d_y = 3 * direction;
			break;
		}

		m_vGunAngle.x = d_x + m_vGunAngle.x;
		m_vGunAngle.y = d_y + m_vGunAngle.y;

		if (m_hmin > m_vGunAngle.y)
		{
			m_vGunAngle.y = m_hmin;
			d_y = 0;
		}

		if (m_vGunAngle.y > m_hmax)
		{
			m_vGunAngle.y = m_hmax;
			d_y = 0;
		}

		if (m_vGunAngle.x < 10)
		{
			m_vGunAngle.x = 10;
			d_x = 0;
		}
		else if (m_vGunAngle.x > 90)
		{
			m_vGunAngle.x = 90;
			d_x = 0;
		}

		if (0 != d_x || 0 != d_y)
		{
			PlaySound();
		}

		SetBoneController(0, m_vGunAngle.x);
		SetBoneController(1, m_vGunAngle.y);

		d_x = 0;
		d_y = 0;

		m_lastupdate = gpGlobals->time;
	}
}

void COp4Mortar::MortarThink()
{
	const float flInterval = StudioFrameAdvance();

	if (m_fSequenceFinished)
	{
		if (pev->sequence != LookupSequence("idle"))
		{
			pev->frame = 0;
			pev->sequence = LookupSequence("idle");
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents(flInterval);

	//GlowShellUpdate();

	pev->nextthink = gpGlobals->time + 0.1;

	if ((pev->spawnflags & SF_MORTAR_ACTIVE) != 0)
	{
		if (!m_hEnemy)
		{
			m_hEnemy = FindTarget();
		}

		CBaseEntity* pEnemy = m_hEnemy;

		if (pEnemy)
		{
			const float distance = (pEnemy->pev->origin - pev->origin).Length();

			if (pEnemy->IsAlive() && m_minRange <= distance && distance <= m_maxRange)
			{
				if (gpGlobals->time - m_trackDelay > 0.5)
				{
					Vector vecPos, vecAngle;
					GetAttachment(0, vecPos, vecAngle);

					m_vIdealGunVector = VecCheckThrow(pev, vecPos, pEnemy->pev->origin, m_velocity / 2);

					m_vIdealGunAngle = UTIL_VecToAngles(m_vIdealGunVector);

					m_trackDelay = gpGlobals->time;
				}

				AIUpdatePosition();

				const float idealDistance = m_vIdealGunVector.Length();

				if (idealDistance > 1.0)
				{
					if (gpGlobals->time - m_fireLast > m_fireDelay)
					{
						EMIT_SOUND(edict(), CHAN_VOICE, "weapons/mortarhit.wav", VOL_NORM, ATTN_NORM);
						UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 1000.0);

						Vector vecPos, vecAngle;
						GetAttachment(0, vecPos, vecAngle);

						vecAngle = m_vGunAngle;

						vecAngle.y = UTIL_AngleMod(pev->angles.y + m_vGunAngle.y);

						if (CMortarShell::CreateMortarShell(vecPos, vecAngle, this, idealDistance))
						{
							pev->sequence = LookupSequence("fire");
							pev->frame = 0;
							ResetSequenceInfo();
						}

						m_fireLast = gpGlobals->time;
					}
				}
				else
				{
					m_fireLast = gpGlobals->time;
				}
			}
			else
			{
				m_hEnemy = NULL;
			}
		}
	}
}

CBaseEntity *COp4Mortar::FindTarget()
{
	CBaseEntity* pPlayerTarget = UTIL_FindEntityByClassname(NULL, "player");

	if (!pPlayerTarget)
		return pPlayerTarget;

	m_pLink = NULL;

	CBaseEntity* pIdealTarget = NULL;
	float flIdealDist = m_maxRange;

	Vector barrelEnd, barrelAngle;
	GetAttachment(0, barrelEnd, barrelAngle);

	if (pPlayerTarget->IsAlive())
	{
		const float distance = (pPlayerTarget->pev->origin - pev->origin).Length();

		if (distance >= m_minRange && m_maxRange >= distance)
		{
			TraceResult tr;
			UTIL_TraceLine(barrelEnd, pPlayerTarget->pev->origin + pPlayerTarget->pev->view_ofs, dont_ignore_monsters, edict(), &tr);

			if ((pev->spawnflags & SF_MORTAR_LINE_OF_SIGHT) == 0 || tr.pHit == pPlayerTarget->pev->pContainingEntity)
			{
				if (0 == m_iEnemyType)
					return pPlayerTarget;

				flIdealDist = distance;
				pIdealTarget = pPlayerTarget;
			}
		}
	}

	const Vector maxRange(m_maxRange, m_maxRange, m_maxRange);

	CBaseEntity* pList[100];
	const int count = UTIL_EntitiesInBox(pList, ARRAYSIZE(pList), pev->origin - maxRange, pev->origin + maxRange, FL_MONSTER | FL_CLIENT);

	for (int i = 0; i < count; ++i)
	{
		CBaseEntity* pEntity = pList[i];

		if (this == pEntity)
			continue;

		if ((pEntity->pev->spawnflags & SF_MONSTER_PRISONER) != 0)
			continue;

		if (pEntity->pev->health <= 0)
			continue;

		CBaseMonster* pMonster = pEntity->MyMonsterPointer();

		if (!pMonster)
			continue;

		if (pMonster->IRelationship(pPlayerTarget) != R_AL)
			continue;

		if ((pEntity->pev->flags & FL_NOTARGET) != 0)
			continue;

		if (!FVisible(pEntity))
			continue;

		if (pEntity->IsPlayer() && (pev->spawnflags & SF_MORTAR_ACTIVE) != 0)
		{
			if (pMonster->FInViewCone(this))
			{
				pev->spawnflags &= ~SF_MORTAR_ACTIVE;
			}
		}
	}

	for (CBaseEntity* pEntity = m_pLink; pEntity; pEntity = pEntity->m_pLink)
	{
		const float distance = (pEntity->pev->origin - pev->origin).Length();

		if (distance >= m_minRange && m_maxRange >= distance && (!pIdealTarget || flIdealDist > distance))
		{
			TraceResult tr;
			UTIL_TraceLine(barrelEnd, pEntity->pev->origin + pEntity->pev->view_ofs, dont_ignore_monsters, edict(), &tr);

			if ((pev->spawnflags & SF_MORTAR_LINE_OF_SIGHT) != 0)
			{
				if (tr.pHit == pEntity->edict())
				{
					flIdealDist = distance;
				}
				if (tr.pHit == pEntity->edict())
					pIdealTarget = pEntity;
			}
			else
			{
				flIdealDist = distance;
				pIdealTarget = pEntity;
			}
		}
	}

	return pIdealTarget;
}

int COp4Mortar::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, 0, bitsDamageType);
}

void COp4Mortar::KeyValue(KeyValueData *pvkd)
{
	if(FStrEq(pvkd->szKeyName, "h_max"))
	{
		m_hmax = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else if(FStrEq(pvkd->szKeyName, "h_min"))
	{
		m_hmin = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else if(FStrEq(pvkd->szKeyName, "mortar_velocity"))
	{
		m_velocity = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else if(FStrEq(pvkd->szKeyName, "mindist"))
	{
		m_minRange = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else if(FStrEq(pvkd->szKeyName, "maxdist"))
	{
		m_maxRange = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else if(FStrEq(pvkd->szKeyName, "enemytype"))
	{
		m_iEnemyType = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else if(FStrEq(pvkd->szKeyName, "firedelay"))
	{
		m_fireDelay = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else
	{
		CBaseToggle::KeyValue(pvkd);
	}
}

void COp4Mortar::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (useType == USE_TOGGLE && (!pActivator || pActivator->IsPlayer()))
	{
		if ((pev->spawnflags & SF_MORTAR_ACTIVE) == 0 && (pev->spawnflags & SF_MORTAR_CONTROLLABLE) != 0)
		{
			//Player fired a mortar
			EMIT_SOUND(edict(), CHAN_VOICE, "weapons/mortarhit.wav", VOL_NORM, ATTN_NONE);
			UTIL_ScreenShake(pev->origin, 12.0, 100.0, 2.0, 1000.0);

			Vector pos, angle;
			GetAttachment(0, pos, angle);

			angle = m_vGunAngle;

			angle.y = UTIL_AngleMod(pev->angles.y + m_vGunAngle.y);

			if (CMortarShell::CreateMortarShell(pos, angle, pActivator ? pActivator : this, m_velocity))
			{
				pev->sequence = LookupSequence("fire");
				pev->frame = 0;
				ResetSequenceInfo();
			}
			return;
		}
	}

	//Toggle AI active state
	if (ShouldToggle(useType, (pev->spawnflags & SF_MORTAR_ACTIVE) != 0))
	{
		pev->spawnflags ^= SF_MORTAR_ACTIVE;

		m_fireLast = 0;
		m_hEnemy = NULL;
	}
}

void COp4Mortar::AIUpdatePosition()
{
	if (fabs(m_vGunAngle.x - m_vIdealGunAngle.x) >= 3.0)
	{
		const float angle = UTIL_AngleDiff(m_vGunAngle.x, m_vIdealGunAngle.x);

		if (angle != 0)
		{
			const float absolute = fabs(angle);
			if (absolute <= 3.0)
				d_x = static_cast<int>(-absolute);
			else
				d_x = angle > 0 ? -3 : 3;
		}
	}

	const float yawAngle = UTIL_AngleMod(m_zeroYaw + m_vGunAngle.y);

	if (fabs(yawAngle - m_vIdealGunAngle.y) >= 3.0)
	{
		const float angle = UTIL_AngleDiff(yawAngle, m_vIdealGunAngle.y);

		if (angle != 0)
		{
			const float absolute = fabs(angle);
			if (absolute <= 3.0)
				d_y = static_cast<int>(-absolute);
			else
				d_y = angle > 0 ? -3 : 3;
		}
	}

	m_vGunAngle.x += d_x;
	m_vGunAngle.y += d_y;

	if (m_hmin > m_vGunAngle.y)
	{
		m_vGunAngle.y = m_hmin;
		d_y = 0;
	}

	if (m_vGunAngle.y > m_hmax)
	{
		m_vGunAngle.y = m_hmax;
		d_y = 0;
	}

	if (m_vGunAngle.x < 10.0)
	{
		m_vGunAngle.x = 10.0;
		d_x = 0;
	}
	else if (m_vGunAngle.x > 90.0)
	{
		m_vGunAngle.x = 90.0;
		d_x = 0;
	}

	if (0 != d_x || 0 != d_y)
	{
		PlaySound();
	}

	SetBoneController(0, m_vGunAngle.x);
	SetBoneController(1, m_vGunAngle.y);

	d_y = 0;
	d_x = 0;
}

//========================================================
// COp4MortarController
//========================================================

class COp4MortarController : public CBaseToggle
{
public:
	void Spawn();
	int Restore(CRestore &restore);
	int Save(CSave &save);
	void KeyValue(KeyValueData *pvkd);
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	int ObjectCaps() { return FCAP_CONTINUOUS_USE; }

	static TYPEDESCRIPTION m_SaveData[];
	int m_direction;
	int m_controller;
	float m_lastpush;
};

LINK_ENTITY_TO_CLASS(func_op4mortarcontroller, COp4MortarController)

TYPEDESCRIPTION	COp4MortarController::m_SaveData[] =
{
	DEFINE_FIELD(COp4MortarController, m_controller, FIELD_INTEGER),
	DEFINE_FIELD(COp4MortarController, m_direction, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE( COp4MortarController, CBaseToggle )

void COp4MortarController::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin(pev, pev->origin);

	SET_MODEL(ENT(pev), STRING(pev->model));
	m_direction = -1;
	m_lastpush = gpGlobals->time;
}

void COp4MortarController::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if(gpGlobals->time - m_lastpush > 0.5)
		m_direction = -m_direction;

	CBaseEntity* ent = UTIL_FindEntityByTargetname(NULL, STRING(pev->target));
	if (ent) {
		if (FClassnameIs(ent->pev, "op4mortar")) {
			COp4Mortar *Mortar = (COp4Mortar*)ent;
			Mortar->UpdatePosition(m_direction, m_controller);
		} else {
			ALERT(at_console, "Found %s, but it's not op4mortar!\n", STRING(pev->target));
		}
	}

	m_lastpush = gpGlobals->time;
}

void COp4MortarController::KeyValue(KeyValueData *pvkd)
{
	if(FStrEq(pvkd->szKeyName, "mortar_axis"))
	{
		m_controller = atoi(pvkd->szValue);
		pvkd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue(pvkd);
}
