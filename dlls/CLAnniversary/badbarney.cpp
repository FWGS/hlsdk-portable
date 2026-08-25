#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"weapons.h"

#define		SQUID_SPRINT_DIST	256 // how close the squid has to get before starting to sprint and refusing to swerve

int			   iBarneyPlasmaSprite;


//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_SQUID_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_SQUID_SMELLFOOD,
	SCHED_SQUID_SEECRAB,
	SCHED_SQUID_EAT,
	SCHED_SQUID_SNIFF_AND_EAT,
	SCHED_SQUID_WALLOW,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_SQUID_HOPTURN = LAST_COMMON_TASK + 1,
};

//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CBarneyPlasma : public CBaseEntity
{
public:
	void Spawn(int type);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, int type);
	void Touch(CBaseEntity *pOther);
	void EXPORT Animate(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
	int type;
};

LINK_ENTITY_TO_CLASS(barneyplasma, CBarneyPlasma);

TYPEDESCRIPTION	CBarneyPlasma::m_SaveData[] =
{
	DEFINE_FIELD(CBarneyPlasma, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CBarneyPlasma, CBaseEntity);

void CBarneyPlasma::Spawn(int type)
{
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->classname = MAKE_STRING("bolt");

	if (type == 0)
	{
		pev->solid = SOLID_BBOX;
		pev->rendermode = kRenderTransAdd;
		pev->rendercolor.x = 108;
		pev->rendercolor.y = 108;
		pev->rendercolor.z = 255;
		pev->renderamt = 225;
		pev->renderfx = kRenderFxNoDissipation;
		pev->frame = 0;
		pev->scale = 0.3;
		UTIL_SetSize(pev, Vector(-0.1, -0.1, -0.1), Vector(0.1, 0.1, 0.1));
	}
	else
	{
		pev->solid = SOLID_BBOX;
		pev->rendermode = kRenderTransAdd;
		pev->rendercolor.x = 0;
		pev->rendercolor.y = 0;
		pev->rendercolor.z = 255;
		pev->renderamt = 255;
		pev->renderfx = kRenderFxNoDissipation;
		pev->frame = 0;
		pev->scale = 1.1;
		UTIL_SetSize(pev, Vector(-0.1, -0.1, -0.1), Vector(0.1, 0.1, 0.1));
	}
	SET_MODEL(ENT(pev), "sprites/nhth1.spr");


	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CBarneyPlasma::Animate(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_ELIGHT);
	WRITE_SHORT(entindex() + 0x4000);		// entity, attachment
	WRITE_COORD(pev->origin.x);		// origin
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(256);	// radius
	WRITE_BYTE(106);	// R
	WRITE_BYTE(106);	// G
	WRITE_BYTE(255);	// B
	WRITE_BYTE(5);	// life * 10
	WRITE_COORD(255); // decay
	MESSAGE_END();

	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
}

void CBarneyPlasma::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, int type)
{
	CBarneyPlasma *pSpit = GetClassPtr((CBarneyPlasma *)NULL);
	pSpit->Spawn(type);

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink(&CBarneyPlasma::Animate);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CBarneyPlasma::Touch(CBaseEntity *pOther)
{
	TraceResult tr;

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND(ENT(pev), CHAN_ITEM, "buttons/spark4.wav", 1, ATTN_NORM); break;
	case 1:	EMIT_SOUND(ENT(pev), CHAN_ITEM, "buttons/spark5.wav", 1, ATTN_NORM); break;
	case 2:	EMIT_SOUND(ENT(pev), CHAN_ITEM, "buttons/spark6.wav", 1, ATTN_NORM); break;
	}

	if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
	{
		UTIL_Sparks(pev->origin);
	}

	if (type == 0)
	{
		if (pOther->pev->takedamage)
		{
			pOther->TakeDamage(pev, pev, gSkillData.bullsquidDmgSpit, DMG_SHOCK);
		}
	}
	else
	{
		if (pOther->pev->takedamage)
		{
			pOther->TakeDamage(pev, pev, gSkillData.bullsquidDmgSpit*1.5, DMG_SHOCK);
		}
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD(pev->origin.x);	// Send to PAS because of the sound
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE(30); // scale * 10
		WRITE_BYTE(15); // framerate
		WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();
		RadiusDamage(pev->origin, pev, pev, 50, 150, CLASS_NONE, DMG_ENERGYBEAM);
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "debris/beamstart14.wav", 1, ATTN_NORM);
	}
	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		BBARNEY_SMACK1		( 1 )
#define		BBARNEY_SMACK2		( 2 )
#define		BBARNEY_SHOOT1		( 3 )
#define		BBARNEY_SHOOT2		( 4 )

class CBadBarney : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	void SetYawSpeed(void);
	int  ISoundMask(void);
	int  Classify(void);
	void HandleAnimEvent(MonsterEvent_t *pEvent);
	void IdleSound(void);
	void PainSound(void);
	void DeathSound(void);
	void AlertSound(void);
	void AttackSound(void);
	void StartTask(Task_t *pTask);
	void RunTask(Task_t *pTask);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);
	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckRangeAttack2(float flDot, float flDist);
	void RunAI(void);
	BOOL FValidateHintType(short sHint);
	Schedule_t *GetSchedule(void);
	Schedule_t *GetScheduleOfType(int Type);
	int TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
	int IRelationship(CBaseEntity *pTarget);
	int IgnoreConditions(void);
	MONSTERSTATE GetIdealState(void);

	float LastKicked;

	int	Save(CSave &save);
	int Restore(CRestore &restore);

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL m_fCanThreatDisplay;// this is so the squid only does the "I see a headcrab!" dance one time. 

	float m_flLastHurtTime;// we keep track of this, because if something hurts a squid, it will forget about its love of headcrabs for a while.
	float m_flNextSpitTime;// last time the bullsquid used the spit attack.
};
LINK_ENTITY_TO_CLASS(monster_badbarney, CBadBarney);

TYPEDESCRIPTION	CBadBarney::m_SaveData[] =
{
	DEFINE_FIELD(CBadBarney, m_fCanThreatDisplay, FIELD_BOOLEAN),
	DEFINE_FIELD(CBadBarney, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CBadBarney, m_flNextSpitTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CBadBarney, CBaseMonster);

//=========================================================
// IgnoreConditions 
//=========================================================
int CBadBarney::IgnoreConditions(void)
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if (gpGlobals->time - m_flLastHurtTime <= 20)
	{
		// haven't been hurt in 20 seconds, so let the squid care about stink. 
		iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
	}

	if (m_hEnemy != NULL)
	{
		if (FClassnameIs(m_hEnemy->pev, "monster_headcrab"))
		{
			// (Unless after a tasty headcrab)
			iIgnore = bits_COND_SMELL | bits_COND_SMELL_FOOD;
		}
	}


	return iIgnore;
}

//=========================================================
// IRelationship - overridden for bullsquid so that it can
// be made to ignore its love of headcrabs for a while.
//=========================================================
int CBadBarney::IRelationship(CBaseEntity *pTarget)
{
	if (gpGlobals->time - m_flLastHurtTime < 5 && FClassnameIs(pTarget->pev, "monster_headcrab"))
	{
		// if squid has been hurt in the last 5 seconds, and is getting relationship for a headcrab, 
		// tell squid to disregard crab. 
		return R_NO;
	}

	return CBaseMonster::IRelationship(pTarget);
}

//=========================================================
// TakeDamage - overridden for bullsquid so we can keep track
// of how much time has passed since it was last injured
//=========================================================
int CBadBarney::TakeDamage(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{
	float flDist;
	Vector vecApex;

	// if the squid is running, has an enemy, was hurt by the enemy, hasn't been hurt in the last 3 seconds, and isn't too close to the enemy,
	// it will swerve. (whew).
	if (m_hEnemy != NULL && IsMoving() && pevAttacker == m_hEnemy->pev && gpGlobals->time - m_flLastHurtTime > 3)
	{
		flDist = (pev->origin - m_hEnemy->pev->origin).Length2D();

		if (flDist > SQUID_SPRINT_DIST)
		{
			flDist = (pev->origin - m_Route[m_iRouteIndex].vecLocation).Length2D();// reusing flDist. 

			if (FTriangulate(pev->origin, m_Route[m_iRouteIndex].vecLocation, flDist * 0.5, m_hEnemy, &vecApex))
			{
				InsertWaypoint(vecApex, bits_MF_TO_DETOUR | bits_MF_DONT_SIMPLIFY);
			}
		}
	}

	if (!FClassnameIs(pevAttacker, "monster_headcrab"))
	{
		// don't forget about headcrabs if it was a headcrab that hurt the squid.
		m_flLastHurtTime = gpGlobals->time;
	}

	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CBadBarney::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist >= 300 && flDist < 512 && flDot >= 0.5)
	{
		if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 128)
		{
			// don't try to spit at someone up really high or down really low.
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack2
//=========================================================
BOOL CBadBarney::CheckRangeAttack2(float flDot, float flDist)
{
	if (flDist >= 512)
	{
		if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
		{
			// don't try to spit at someone up really high or down really low.
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack1 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the tailwhip attack
//=========================================================
BOOL CBadBarney::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist < 100 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckMeleeAttack2 - bullsquid is a big guy, so has a longer
// melee range than most monsters. This is the bite attack.
// this attack will not be performed if the tailwhip attack
// is valid.
//=========================================================
BOOL CBadBarney::CheckMeleeAttack2(float flDot, float flDist)
{
	if (LastKicked >= gpGlobals->time)
		return FALSE;
	if (flDist >= 100 && flDist < 299 && flDot >= 0.7)
	{
		if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 64)
		{
			// don't try to spit at someone up really high or down really low.
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

//=========================================================
//  FValidateHintType 
//=========================================================
BOOL CBadBarney::FValidateHintType(short sHint)
{
	int i;

	static short sSquidHints[] =
	{
		HINT_WORLD_HUMAN_BLOOD,
	};

	for (i = 0; i < ARRAYSIZE(sSquidHints); i++)
	{
		if (sSquidHints[i] == sHint)
		{
			return TRUE;
		}
	}

	ALERT(at_aiconsole, "Couldn't validate hint type");
	return FALSE;
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CBadBarney::ISoundMask(void)
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBadBarney::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//=========================================================
// IdleSound 
//=========================================================
#define SQUID_ATTN_IDLE	(float)1.5
void CBadBarney::IdleSound(void)
{
	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_idle1.wav", 1, SQUID_ATTN_IDLE);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_idle2.wav", 1, SQUID_ATTN_IDLE);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_idle3.wav", 1, SQUID_ATTN_IDLE);
		break;
	case 3:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_idle4.wav", 1, SQUID_ATTN_IDLE);
		break;
	}
}

//=========================================================
// PainSound 
//=========================================================
void CBadBarney::PainSound(void)
{
	//SPECIAL HAPPENING
	if (RANDOM_LONG(0, 8) == 0) //SPECIAL HAPPENING
	{
		switch (RANDOM_LONG(0, 10))
		{
		case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain1.wav", 1, ATTN_NORM, 0, 100); break;
		case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain2.wav", 1, ATTN_NORM, 0, 100); break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain3.wav", 1, ATTN_NORM, 0, 100); break;
		case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain4.wav", 1, ATTN_NORM, 0, 100); break;
		case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain5.wav", 1, ATTN_NORM, 0, 100); break;
		case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain6.wav", 1, ATTN_NORM, 0, 100); break;
		case 6: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain7.wav", 1, ATTN_NORM, 0, 100); break;
		case 7: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain8.wav", 1, ATTN_NORM, 0, 100); break;
		case 8: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain9.wav", 1, ATTN_NORM, 0, 100); break;
		case 9: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain10.wav", 1, ATTN_NORM, 0, 100); break;
		case 10: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "generic/genericpain11.wav", 1, ATTN_NORM, 0, 100); break;
		}
		return;
	}
	//SPECIAL HAPPENING

	int iPitch = RANDOM_LONG(95, 105);

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_pain1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_pain2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_pain3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// AlertSound
//=========================================================
void CBadBarney::AlertSound(void)
{
	int iPitch = RANDOM_LONG(95, 105);

	switch (RANDOM_LONG(0, 5))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_alert1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_alert2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_alert3.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_alert4.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_alert5.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 5:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "badbarney/bba_alert6.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBadBarney::SetYawSpeed(void)
{
	pev->yaw_speed = 920;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CBadBarney::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch (pEvent->event)
	{
	case BBARNEY_SHOOT1:
	{
						   Vector	vecSpitOffset;
						   Vector	vecSpitDir;

						   UTIL_MakeVectors(pev->angles);

						   // !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
						   // we should be able to read the position of bones at runtime for this info.
						   vecSpitOffset = (gpGlobals->v_forward * 92 + gpGlobals->v_up * 64);
						   vecSpitOffset = (pev->origin + vecSpitOffset);
						   vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

						   vecSpitDir.x += RANDOM_FLOAT(-0.01, 0.01);
						   vecSpitDir.y += RANDOM_FLOAT(-0.01, 0.01);
						   vecSpitDir.z += RANDOM_FLOAT(-0.01, 0);


						   // do stuff for this event.
						   AttackSound();

						   CBarneyPlasma::Shoot(pev, vecSpitOffset, vecSpitDir * 1800, 0);
	}
		break;

	case BBARNEY_SHOOT2:
	{
						   Vector	vecSpitOffset;
						   Vector	vecSpitDir;

						   UTIL_MakeVectors(pev->angles);

						   // !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
						   // we should be able to read the position of bones at runtime for this info.
						   vecSpitOffset = (gpGlobals->v_forward * 100 + gpGlobals->v_up * 64);
						   vecSpitOffset = (pev->origin + vecSpitOffset);
						   vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

						   vecSpitDir.x += RANDOM_FLOAT(-0.01, 0.01);
						   vecSpitDir.y += RANDOM_FLOAT(-0.01, 0.01);
						   vecSpitDir.z += RANDOM_FLOAT(-0.01, 0);


						   // do stuff for this event.
						   EMIT_SOUND(ENT(pev), CHAN_WEAPON, "debris/beamstart11.wav", 1, ATTN_NORM);

						   CBarneyPlasma::Shoot(pev, vecSpitOffset, vecSpitDir * 1600, 1);
	}
		break;

	case BBARNEY_SMACK1:
	{
						   // SOUND HERE!
						   CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.bullsquidDmgBite * 0.6, DMG_SLASH);
						   if (pHurt)
						   {
							   //pHurt->pev->punchangle.z = -15;
							   //pHurt->pev->punchangle.x = -45;
							   if (pHurt->IsBSPModel() == false)
							   {
								   pHurt->pev->punchangle.z = -20;
								   pHurt->pev->punchangle.x = -20;
								   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 225;
							   }
							   switch (RANDOM_LONG(0, 2))
							   {
							   case 0:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT(95, 105));
								   break;
							   case 1:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT(95, 105));
								   break;
							   case 2:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT(95, 105));
								   break;
							   }
						   }
	}
		break;

	case BBARNEY_SMACK2:
	{
						   LastKicked = gpGlobals->time + 4;
						   CBaseEntity *pHurt = CheckTraceHullAttack(70, gSkillData.bullsquidDmgWhip * 0.6, DMG_CLUB | DMG_ALWAYSGIB);
						   pev->velocity = pev->velocity + gpGlobals->v_forward * 620;
						   pev->velocity = pev->velocity + gpGlobals->v_up * 200;
						   if (pHurt)
						   {
							   pHurt->pev->punchangle.z = -20;
							   pHurt->pev->punchangle.x = -20;
							   if (pHurt->IsBSPModel() == false)
							   {
								   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 600;
								   pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 50;
							   }
							   switch (RANDOM_LONG(0, 2))
							   {
							   case 0:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT(95, 105));
								   break;
							   case 1:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT(95, 105));
								   break;
							   case 2:
								   EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1, ATTN_NORM, 0, RANDOM_FLOAT(95, 105));
								   break;
							   }
						   }
	}
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void CBadBarney::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/badbarney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = 120;
	m_flFieldOfView = VIEW_FIELD_WIDE;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fCanThreatDisplay = TRUE;
	m_flNextSpitTime = gpGlobals->time;

	LastKicked = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBadBarney::Precache()
{
	PRECACHE_MODEL("models/badbarney.mdl");

	PRECACHE_MODEL("sprites/nhth1.spr");// spit projectile.

	iBarneyPlasmaSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	PRECACHE_SOUND("badbarney/bba_alert1.wav");
	PRECACHE_SOUND("badbarney/bba_alert2.wav");
	PRECACHE_SOUND("badbarney/bba_alert3.wav");
	PRECACHE_SOUND("badbarney/bba_alert4.wav");
	PRECACHE_SOUND("badbarney/bba_alert5.wav");
	PRECACHE_SOUND("badbarney/bba_alert6.wav");

	PRECACHE_SOUND("badbarney/bba_die1.wav");
	PRECACHE_SOUND("badbarney/bba_die2.wav");
	PRECACHE_SOUND("badbarney/bba_die3.wav");

	PRECACHE_SOUND("badbarney/bba_pain1.wav");
	PRECACHE_SOUND("badbarney/bba_pain2.wav");
	PRECACHE_SOUND("badbarney/bba_pain3.wav");

	PRECACHE_SOUND("badbarney/bba_idle1.wav");
	PRECACHE_SOUND("badbarney/bba_idle2.wav");
	PRECACHE_SOUND("badbarney/bba_idle3.wav");
	PRECACHE_SOUND("badbarney/bba_idle4.wav");

	PRECACHE_SOUND("badbarney/bba_melee1.wav");
	PRECACHE_SOUND("badbarney/bba_melee2.wav");
	PRECACHE_SOUND("badbarney/bba_shoot1.wav");

	PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");
	PRECACHE_SOUND("zombie/claw_strike3.wav");

	PRECACHE_SOUND("buttons/spark4.wav");
	PRECACHE_SOUND("buttons/spark5.wav");
	PRECACHE_SOUND("buttons/spark6.wav");
	PRECACHE_SOUND("debris/beamstart14.wav");
	PRECACHE_SOUND("debris/beamstart11.wav");

}

//=========================================================
// DeathSound
//=========================================================
void CBadBarney::DeathSound(void)
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_die1.wav", 1, ATTN_NORM);
		break;
	case 1:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_die2.wav", 1, ATTN_NORM);
		break;
	case 2:
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_die3.wav", 1, ATTN_NORM);
		break;
	}
}

//=========================================================
// AttackSound
//=========================================================
void CBadBarney::AttackSound(void)
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "badbarney/bba_shoot1.wav", 1, ATTN_NORM);
}


//========================================================
// RunAI - overridden for bullsquid because there are things
// that need to be checked every think.
//========================================================
void CBadBarney::RunAI(void)
{
	// first, do base class stuff
	CBaseMonster::RunAI();

	if (pev->skin != 0)
	{
		// close eye if it was open.
		pev->skin = 0;
	}

	if (RANDOM_LONG(0, 39) == 0)
	{
		pev->skin = 1;
	}

	if (m_hEnemy != NULL && m_Activity == ACT_RUN)
	{
		// chasing enemy. Sprint for last bit
		if ((pev->origin - m_hEnemy->pev->origin).Length2D() < SQUID_SPRINT_DIST)
		{
			pev->framerate = 1.25;
		}
	}

}

//========================================================
// AI Schedules Specific to this monster
//=========================================================

// primary range attack
Task_t	tlBarnRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK1, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slBarnRangeAttack1[] =
{
	{
		tlBarnRangeAttack1,
		ARRAYSIZE(tlBarnRangeAttack1),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Squid Range Attack1"
	},
};

//Secondary range attack
Task_t	tlBarnRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, (float)0 },
	{ TASK_RANGE_ATTACK2, (float)0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t	slBarnRangeAttack2[] =
{
	{
		tlBarnRangeAttack2,
		ARRAYSIZE(tlBarnRangeAttack2),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED |
		bits_COND_NO_AMMO_LOADED,
		0,
		"Squid Range Attack2"
	},
};


DEFINE_CUSTOM_SCHEDULES(CBadBarney)
{
	slBarnRangeAttack1,
		slBarnRangeAttack2
};

IMPLEMENT_CUSTOM_SCHEDULES(CBadBarney, CBaseMonster);

//=========================================================
// GetSchedule 
//=========================================================
Schedule_t *CBadBarney::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	{
							   if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
							   {
								   return GetScheduleOfType(SCHED_SQUID_HURTHOP);
							   }

							   if (HasConditions(bits_COND_SMELL_FOOD))
							   {
								   CSound		*pSound;

								   pSound = PBestScent();

								   if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
								   {
									   // scent is behind or occluded
									   return GetScheduleOfType(SCHED_SQUID_SNIFF_AND_EAT);
								   }

								   // food is right out in the open. Just go get it.
								   return GetScheduleOfType(SCHED_SQUID_EAT);
							   }

							   if (HasConditions(bits_COND_SMELL))
							   {
								   // there's something stinky. 
								   CSound		*pSound;

								   pSound = PBestScent();
								   if (pSound)
									   return GetScheduleOfType(SCHED_SQUID_WALLOW);
							   }

							   break;
	}
	case MONSTERSTATE_COMBAT:
	{
								// dead enemy
								if (HasConditions(bits_COND_ENEMY_DEAD))
								{
									// call base class, all code to handle dead enemies is centralized there.
									return CBaseMonster::GetSchedule();
								}

								if (HasConditions(bits_COND_NEW_ENEMY))
								{
									if (m_fCanThreatDisplay && IRelationship(m_hEnemy) == R_HT)
									{
										// this means squid sees a headcrab!
										m_fCanThreatDisplay = FALSE;// only do the headcrab dance once per lifetime.
										return GetScheduleOfType(SCHED_SQUID_SEECRAB);
									}
									else
									{
										return GetScheduleOfType(SCHED_WAKE_ANGRY);
									}
								}

								if (HasConditions(bits_COND_SMELL_FOOD))
								{
									CSound		*pSound;

									pSound = PBestScent();

									if (pSound && (!FInViewCone(&pSound->m_vecOrigin) || !FVisible(pSound->m_vecOrigin)))
									{
										// scent is behind or occluded
										return GetScheduleOfType(SCHED_SQUID_SNIFF_AND_EAT);
									}

									// food is right out in the open. Just go get it.
									return GetScheduleOfType(SCHED_SQUID_EAT);
								}

								if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
								{
									return GetScheduleOfType(SCHED_RANGE_ATTACK1);
								}

								if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
								{
									return GetScheduleOfType(SCHED_RANGE_ATTACK2);
								}

								if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
								{
									return GetScheduleOfType(SCHED_MELEE_ATTACK1);
								}

								if (HasConditions(bits_COND_CAN_MELEE_ATTACK2))
								{
									return GetScheduleOfType(SCHED_MELEE_ATTACK2);
								}

								return GetScheduleOfType(SCHED_CHASE_ENEMY);

								break;
	}
	}

	return CBaseMonster::GetSchedule();
}

//=========================================================
// GetScheduleOfType
//=========================================================
Schedule_t* CBadBarney::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_RANGE_ATTACK1:
		//return &slBarnRangeAttack1[ 0 ];
		break;
	case SCHED_RANGE_ATTACK2:
		return &slBarnRangeAttack2[0];
		break;
	}

	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.  OVERRIDDEN for bullsquid because it needs to
// know explicitly when the last attempt to chase the enemy
// failed, since that impacts its attack choices.
//=========================================================
void CBadBarney::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_MELEE_ATTACK2:
	{
							   switch (RANDOM_LONG(0, 4))
							   {
							   case 0:
								   EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_melee1.wav", 1, ATTN_NORM);
								   break;
							   case 1:
								   EMIT_SOUND(ENT(pev), CHAN_VOICE, "badbarney/bba_melee2.wav", 1, ATTN_NORM);
								   break;
							   case 2:
								   break;
							   case 3:
								   break;
							   case 4:
								   break;
							   }

							   CBaseMonster::StartTask(pTask);
							   break;
	}
	case TASK_SQUID_HOPTURN:
	{
							   SetActivity(ACT_HOP);
							   MakeIdealYaw(m_vecEnemyLKP);
							   break;
	}
	case TASK_GET_PATH_TO_ENEMY:
	{
								   if (BuildRoute(m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy))
								   {
									   m_iTaskStatus = TASKSTATUS_COMPLETE;
								   }
								   else
								   {
									   ALERT(at_aiconsole, "GetPathToEnemy failed!!\n");
									   TaskFail();
								   }
								   break;
	}
	default:
	{
			   CBaseMonster::StartTask(pTask);
			   break;
	}
	}
}

//=========================================================
// RunTask
//=========================================================
void CBadBarney::RunTask(Task_t *pTask)
{
	switch (pTask->iTask)
	{
	case TASK_SQUID_HOPTURN:
	{
							   MakeIdealYaw(m_vecEnemyLKP);
							   ChangeYaw(pev->yaw_speed);

							   if (m_fSequenceFinished)
							   {
								   m_iTaskStatus = TASKSTATUS_COMPLETE;
							   }
							   break;
	}
	default:
	{
			   CBaseMonster::RunTask(pTask);
			   break;
	}
	}
}


//=========================================================
// GetIdealState - Overridden for Bullsquid to deal with
// the feature that makes it lose interest in headcrabs for 
// a while if something injures it. 
//=========================================================
MONSTERSTATE CBadBarney::GetIdealState(void)
{
	int	iConditions;

	iConditions = IScheduleFlags();

	// If no schedule conditions, the new ideal state is probably the reason we're in here.
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
		/*
		COMBAT goes to ALERT upon death of enemy
		*/
	{
								if (m_hEnemy != NULL && (iConditions & bits_COND_LIGHT_DAMAGE || iConditions & bits_COND_HEAVY_DAMAGE) && FClassnameIs(m_hEnemy->pev, "monster_headcrab"))
								{
									// if the squid has a headcrab enemy and something hurts it, it's going to forget about the crab for a while.
									m_hEnemy = NULL;
									m_IdealMonsterState = MONSTERSTATE_ALERT;
								}
								break;
	}
	}

	m_IdealMonsterState = CBaseMonster::GetIdealState();

	return m_IdealMonsterState;
}

