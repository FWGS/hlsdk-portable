#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"
#include "monsters.h"
#include "player.h"
#include "soundent.h"

class CMortarShell : public CGrenade
{
public:
	void Precache();
	void BurnThink();
	void MortarExplodeTouch(CBaseEntity *pOther);
	void Spawn();
	void FlyThink();
	int Save(CSave &save);
	int Restore(CRestore &restore);
	static CMortarShell *CreateMortarShell(Vector p_VecOrigin, Vector p_VecAngles, CBaseEntity *pOwner, int velocity);

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
	SET_MODEL(ENT(pev), "models/mortarshell.mdl");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_BOUNCE;
	pev->classname = MAKE_STRING("mortar_shell");
	SetThink(&CMortarShell::BurnThink);
	SetTouch(&CMortarShell::MortarExplodeTouch);

	pev->dmg = gSkillData.plrDmgRPG*2;
	pev->nextthink = gpGlobals->time + 0.1;
	m_flIgniteTime = gpGlobals->time;
	m_iSoundedOff = FALSE;
}

void CMortarShell::MortarExplodeTouch(CBaseEntity *pOther)
{
	TraceResult tr;
	Vector vecSpot;

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	vecSpot = pev->origin + Vector( 0, 0, 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, dont_ignore_glass, ENT(pev), &tr );

	// Pull out of the wall a bit
	if( tr.flFraction != 1.0 )
	{
		pev->origin = tr.vecEndPos + ( tr.vecPlaneNormal * ( pev->dmg - 24 ) * 0.6 );
	}

	int iContents = UTIL_PointContents( pev->origin );

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );		// This makes a dynamic light and the explosion sprites/sound
	WRITE_COORD( pev->origin.x );	// Send to PAS because of the sound
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	if( iContents != CONTENTS_WATER )
	{
		WRITE_SHORT( g_sModelIndexFireball );
	}
	else
	{
		WRITE_SHORT( g_sModelIndexWExplosion );
	}
	WRITE_BYTE( (pev->dmg - 50) * 5); // scale * 10
	WRITE_BYTE( 10 ); // framerate
	WRITE_BYTE( TE_EXPLFLAG_NONE );
	MESSAGE_END();

	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage( pev, pevOwner, pev->dmg, CLASS_NONE, DMG_BLAST );

	if( RANDOM_FLOAT( 0, 1 ) < 0.5 )
	{
		UTIL_DecalTrace( &tr, 11 );
	}
	else
	{
		UTIL_DecalTrace( &tr, 12 );
	}

	switch( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM );
		break;
	}

	pev->effects |= EF_NODRAW;
	SetThink( &CGrenade::Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;

	if( iContents != CONTENTS_WATER )
	{
		int sparkCount = RANDOM_LONG( 0, 3 );
		for( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
	}
}


void CMortarShell::BurnThink()
{
	UTIL_VecToAngles(pev->velocity);
	pev->angles = pev->velocity;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(110);
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

	if(!((m_flIgniteTime + 0.2) >= gpGlobals->time))
		SetThink(&CMortarShell::FlyThink);

	pev->nextthink = gpGlobals->time + 0.01;
}

void CMortarShell::FlyThink()
{
	pev->angles = UTIL_VecToAngles(pev->velocity);
	pev->angles.x -= 90;

	if(pev->velocity.z < 20 && !m_iSoundedOff)
	{
		m_iSoundedOff = TRUE;
		EMIT_SOUND_DYN(ENT(pev), 2, "weapons/ofmortar.wav", 1, 0, 0, 100);
	}

	pev->nextthink = gpGlobals->time + 0.1;
}

CMortarShell *CMortarShell::CreateMortarShell(Vector p_VecOrigin, Vector p_VecAngles, CBaseEntity *pOwner, int velocity)
{
	CMortarShell *rocket = GetClassPtr( (CMortarShell *)NULL );
	rocket->Spawn();

	rocket->pev->gravity = 1;
	UTIL_SetOrigin( rocket->pev, p_VecOrigin );
	rocket->pev->angles = UTIL_VecToAngles(p_VecAngles);
	UTIL_MakeVectors(p_VecAngles);
	rocket->pev->velocity = rocket->pev->velocity - gpGlobals->v_forward * velocity;
	if (pOwner)
		rocket->pev->owner = ENT(pOwner->pev);

	return rocket;
}

#define SF_MORTAR_ACTIVE (1 << 0)
#define SF_MORTAR_LINE_OF_SIGHT (1 << 4)
#define SF_MORTAR_CAN_CONTROL (1 << 5)

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

	float m_FireDelay;
	BOOL m_tracking;
	float m_minRange;
	float m_maxRange;
	float m_lastupdate;
	float m_zeroYaw;
	float m_lastFire;
	float m_trackDelay;
	float m_flExplodeTime;
	int m_hmax;
	int m_hmin;
	int d_x;
	int d_y;
	int m_velocity;
	int m_iEnemyType;
	int m_iUpdateTime;
	Vector m_vGunAngle;
	Vector m_vIdealGunVector;
	Vector m_vIdealGunAngle;
};

LINK_ENTITY_TO_CLASS(op4mortar, COp4Mortar)

TYPEDESCRIPTION	COp4Mortar::m_SaveData[] =
{
	DEFINE_FIELD( COp4Mortar, m_tracking, FIELD_BOOLEAN ),
	DEFINE_FIELD( COp4Mortar, m_FireDelay, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_minRange, FIELD_FLOAT),
	DEFINE_FIELD( COp4Mortar, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_lastFire, FIELD_FLOAT),
	DEFINE_FIELD( COp4Mortar, m_trackDelay, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_hmax, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_hmin, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_velocity, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_iEnemyType, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_vGunAngle, FIELD_VECTOR ),
	DEFINE_FIELD(COp4Mortar, m_lastupdate, FIELD_FLOAT),
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
	float angle;

	Precache();
	UTIL_SetOrigin(pev, pev->origin);
	SET_MODEL(ENT(pev), "models/mortar.mdl");

	pev->sequence = LookupSequence("idle");
	ResetSequenceInfo();
	pev->frame = 0;
	pev->framerate = 1;
	m_tracking = FALSE;
	m_lastupdate = gpGlobals->time + 0.5;
	m_vGunAngle = Vector(0,0,0);
	m_iUpdateTime = 0;

	if(m_FireDelay < 0.5)
		m_FireDelay = 5;

	if(m_minRange == 0)
		m_minRange = 128;

	if(m_maxRange == 0)
		m_maxRange = 3072;

	InitBoneControllers();
	angle = pev->angles.y + 180;

	m_zeroYaw = UTIL_AngleMod(angle);
	m_hEnemy = NULL;

	if (pev->spawnflags & SF_MORTAR_ACTIVE)
		SetThink(&COp4Mortar::MortarThink);
	else
		SetThink(NULL);

	m_flExplodeTime = gpGlobals->time + 5;

	pev->nextthink = gpGlobals->time + 0.1;
}

void COp4Mortar::UpdatePosition(float direction, int controller)
{
	if(m_vGunAngle.y > 90)
		m_vGunAngle.y = 90;

	if(m_vGunAngle.y < -90)
		m_vGunAngle.y = -90;

	if(m_vGunAngle.x > 90)
		m_vGunAngle.x = 90;

	if(m_vGunAngle.x < 0)
		m_vGunAngle.x = 0;

	if(controller == 1)
		m_vGunAngle.y += direction/2;
	else
		m_vGunAngle.x += direction/2;

	if(m_iUpdateTime >= 15)
	{
		EMIT_SOUND_DYN(ENT(pev), 2, "player/pl_grate1.wav", 1, 0.8, 0, 100);
		m_iUpdateTime = 0;
	}

	SetBoneController(1, m_vGunAngle.y);
	SetBoneController(0, m_vGunAngle.x);

	m_lastupdate = gpGlobals->time + 0.1;
	m_iUpdateTime++;
}

void COp4Mortar::MortarThink()
{
	Vector pos, angle, vecTarget;

	if(m_fSequenceFinished)
	{
		if(pev->sequence != LookupSequence("idle"))
		{
			pev->frame = 0;
			pev->sequence = LookupSequence("idle");
			ResetSequenceInfo();
		}
	}

	DispatchAnimEvents();
	StudioFrameAdvance();

	pev->nextthink = gpGlobals->time + 0.1;

	if(pev->spawnflags & SF_MORTAR_ACTIVE)
	{
		if(m_hEnemy == 0 || !m_hEnemy->IsAlive())
		{
			m_hEnemy = FindTarget();
		}
	}

	if(m_hEnemy != 0)
	{
		vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmax.z);

		if((pev->origin - m_hEnemy->pev->origin).Length() <= m_maxRange)
		{
			GetAttachment(0, pos, angle);

			m_vIdealGunVector = VecCheckThrow(pev, pos, vecTarget, 700, 1);
			m_vIdealGunVector.x =- m_vIdealGunVector.x;
			m_vIdealGunVector.y =- m_vIdealGunVector.y;

			m_vIdealGunAngle = UTIL_VecToAngles(m_vIdealGunVector);

			m_trackDelay = gpGlobals->time;
		}
		AIUpdatePosition();
	}

	if(m_hEnemy != 0 && gpGlobals->time - m_lastFire > 5 && (m_hEnemy->pev->origin - pev->origin).Length() > 710)
	{
		EMIT_SOUND_DYN(ENT(pev), 2, "weapons/mortarhit.wav", 1, 0.8, 0, 100);
		UTIL_ScreenShake(pev->origin, 12, 100, 2, 1000);

		float speed = m_vIdealGunVector.Length();

		angle = m_vIdealGunAngle;

		if(speed > 0)
		{
			if(CMortarShell::CreateMortarShell(pev->origin, angle, this, floor(speed)))
			{
				pev->frame = 0;
				pev->sequence = LookupSequence("fire");
				ResetSequenceInfo();
				m_lastFire = gpGlobals->time;
			}
		}
	}
}

CBaseEntity *COp4Mortar::FindTarget()
{
	CBaseEntity *pPlayer;
	Vector BarretEnd;
	Vector BarretAngle;
	Vector targetPosition;
	TraceResult tr;
	CBaseEntity *pIdealTarget = NULL;

	if((pPlayer = UTIL_FindEntityByClassname(0, "player")) == NULL )
		return NULL;

	m_pLink = 0;

	GetAttachment(0, BarretEnd, BarretAngle);

	float dist = (pPlayer->pev->origin - pev->origin).Length();

	if(pPlayer->IsAlive())
	{
		if(m_maxRange >= dist)
		{
			targetPosition = pPlayer->pev->origin + pev->view_ofs;
			UTIL_TraceLine(BarretEnd, targetPosition, ignore_monsters, dont_ignore_glass, ENT(pev), &tr);

			if((!(pev->spawnflags & SF_MORTAR_LINE_OF_SIGHT) || tr.pHit == ENT(pPlayer->pev)) && !m_iEnemyType)
			{
				pIdealTarget = pPlayer;
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
	if(strcmp(pvkd->szKeyName, "m_hmax"))
	{
		m_hmax = atoi(pvkd->szValue);
		pvkd->fHandled = 1;
	}
	else if(strcmp(pvkd->szKeyName, "m_hmin"))
	{
		m_hmin = atoi(pvkd->szValue);
		pvkd->fHandled = 1;
	}
	else if(strcmp(pvkd->szKeyName, "mortar_velocity"))
	{
		m_velocity = atoi(pvkd->szValue);
		pvkd->fHandled = 1;
	}
	else if(strcmp(pvkd->szKeyName, "mindist"))
	{
		m_minRange = atoi(pvkd->szValue);
		pvkd->fHandled = 1;
	}
	else if(strcmp(pvkd->szKeyName, "maxdist"))
	{
		m_maxRange = atoi(pvkd->szValue);
		pvkd->fHandled = 1;
	}
	else if(strcmp(pvkd->szKeyName, "enemytype"))
	{
		m_iEnemyType = atoi(pvkd->szValue);
		pvkd->fHandled = 1;
	}
	else if(strcmp(pvkd->szKeyName, "firedelay"))
	{
		m_FireDelay = atoi(pvkd->szValue);
		pvkd->fHandled = 1;
	}
	else
	{
		CBaseToggle::KeyValue(pvkd);
	}
}

void COp4Mortar::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SetThink(NULL);

	if((pev->spawnflags & SF_MORTAR_CAN_CONTROL)  && (pActivator->pev->origin - pev->origin).Length() < 170)
	{
		EMIT_SOUND_DYN(ENT(pev), 2, "weapons/mortarhit.wav", 1, 0, 0, 100);
		UTIL_ScreenShake(pev->origin, 12, 100, 2, 1000);

		Vector pos, angle;
		GetAttachment(0, pos, angle);
		angle = m_vGunAngle;
		float anglemod = pev->angles.y + m_vGunAngle.y;

		angle.y = UTIL_AngleMod(anglemod);

		if((CMortarShell::CreateMortarShell(pos, angle, this, 2000 - (m_vGunAngle.x * 12.25))) != NULL)
		{
			pev->frame = 0;
			pev->sequence = LookupSequence("fire");
			ResetSequenceInfo();
		}
	}

}

void COp4Mortar::AIUpdatePosition()
{
	if(m_hEnemy != 0 && (m_hEnemy->pev->origin - pev->origin).Length() < 710)
		return;

	if(m_vIdealGunAngle.x == 270 && m_vIdealGunAngle.y == 0)
	{
		m_vIdealGunAngle.x = 0;
		m_vIdealGunAngle.y = 0;
	}

	if(m_vIdealGunAngle.x <= 0)
		m_vIdealGunAngle.x = 0;

	if(m_vIdealGunAngle.x > 90)
		m_vIdealGunAngle.x = 90;

	if(m_vIdealGunAngle.y > 165 && m_vIdealGunAngle.y < 270)
		m_vIdealGunAngle.y = -90;

	else if(m_vIdealGunAngle.y > 90 && m_vIdealGunAngle.y < 165)
		m_vIdealGunAngle.y = 90;


	SetBoneController(0, m_vIdealGunAngle.x);
	SetBoneController(1, m_vIdealGunAngle.y);

	m_vGunAngle = m_vIdealGunAngle;
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
	float m_direction;
	float m_lastpush;
	int m_controller;
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
		pvkd->fHandled = 1;
	}
	else
		CBaseToggle::KeyValue(pvkd);
}
