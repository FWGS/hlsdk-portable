
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
// shock - projectile shot from shockrifles.
//=========================================================

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

#define SHOCK_BEAM_LENGTH		64
#define SHOCK_BEAM_LENGTH_HALF	SHOCK_BEAM_LENGTH * 0.5f

#define SHOCK_BEAM_WIDTH		50

//=========================================================
// Shockrifle projectile
//=========================================================
class CShock : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT ShockThink(void);

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateBeam(const Vector& start, const Vector& end, int width);
	void ClearBeam();
	void UpdateBeam(const Vector& start, const Vector& end);
	void ComputeBeamPositions(Vector* pos1, Vector* pos2);

	CBeam *m_pBeam;
	Vector m_vecBeamStart, m_vecBeamEnd;
};

LINK_ENTITY_TO_CLASS(shock, CShock);

TYPEDESCRIPTION	CShock::m_SaveData[] =
{
	DEFINE_ARRAY(CShock, m_pBeam, FIELD_CLASSPTR, 1),
	DEFINE_FIELD(CShock, m_vecBeamStart, FIELD_POSITION_VECTOR),
	DEFINE_FIELD(CShock, m_vecBeamEnd, FIELD_POSITION_VECTOR),
};

IMPLEMENT_SAVERESTORE(CShock, CBaseEntity);

void CShock::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("shock");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 170;
	pev->renderfx = kRenderFxNoDissipation;

	SET_MODEL(ENT(pev), "sprites/flare3.spr");
	pev->frame = 0;
	pev->scale = 0.4;

	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	// Make beam NULL to avoid assertions.
	m_pBeam = 0;

	ComputeBeamPositions(&m_vecBeamStart, &m_vecBeamEnd);

	// Create the beam.
	//CreateBeam(m_vecBeamStart, m_vecBeamEnd, SHOCK_BEAM_WIDTH);

	SetThink(&CShock::ShockThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CShock::ShockThink(void)
{
	pev->nextthink  = gpGlobals->time + 0.01f;

	ComputeBeamPositions(&m_vecBeamStart, &m_vecBeamEnd);

	// Update the beam.
	UpdateBeam(m_vecBeamStart, m_vecBeamEnd);
}

void CShock::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CShock *pShock = GetClassPtr((CShock *)NULL);
	pShock->Spawn();

	UTIL_SetOrigin(pShock->pev, vecStart);
	pShock->pev->velocity = vecVelocity;
	pShock->pev->owner = ENT(pevOwner);

	pShock->SetThink(&CShock::ShockThink);
	pShock->pev->nextthink = gpGlobals->time;
}

void CShock::Touch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner)
		return;

	TraceResult tr = UTIL_GetGlobalTrace( );
	int		iPitch, iVolume;

	// Lower the volume if touched entity is not a player.
	iVolume = (!pOther->IsPlayer())
		? RANDOM_FLOAT(0.4f, 0.5f)
		: RANDOM_FLOAT(0.8f, 1);

	iPitch = RANDOM_FLOAT(80, 110);

	// splat sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/shock_impact.wav", iVolume, ATTN_NORM, 0, iPitch);
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 8 );		// radius * 0.1
		WRITE_BYTE( 0 );		// r
		WRITE_BYTE( 255 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 10 );		// time * 10
		WRITE_BYTE( 10 );		// decay * 0.1
	MESSAGE_END( );

	ClearBeam();
	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0, 2));

		int iContents = UTIL_PointContents(pev->origin);

		// Create sparks 
		if (iContents != CONTENTS_WATER)
		{
			UTIL_Sparks(tr.vecEndPos);
		}
		UTIL_Remove(this);
	}
	else
	{
		ClearMultiDamage();
		entvars_t *pevOwner = VARS(pev->owner);
		float damage = gSkillData.monDmgShockroach;
		if (pevOwner && (pevOwner->flags | FL_CLIENT))
			damage = gSkillData.plrDmgShockroach;
		pOther->TraceAttack(pev, damage, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM );
		ApplyMultiDamage(pev, pev);
		if (pOther->IsPlayer() && (UTIL_PointContents(pev->origin) != CONTENTS_WATER))
		{
			const Vector position = tr.vecEndPos;
			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOther->pev );
				WRITE_BYTE( TE_SPARKS );
				WRITE_COORD( position.x );
				WRITE_COORD( position.y );
				WRITE_COORD( position.z );
			MESSAGE_END();
		}
	}
	UTIL_Remove(this);
}

//=========================================================
// Purpose:
//=========================================================
void CShock::CreateBeam(const Vector& start, const Vector& end, int width)
{
	if (m_pBeam)
	{
		ClearBeam();
	}

	m_pBeam = CBeam::BeamCreate("sprites/lgtning.spr", width);
	if (!m_pBeam)
		return;

	m_pBeam->PointsInit(start, end);
	m_pBeam->SetColor(140, 255, 220);
	m_pBeam->SetBrightness(RANDOM_LONG(24, 25) * 10);
	m_pBeam->SetFrame(0);
	m_pBeam->SetScrollRate(10);
	m_pBeam->SetNoise(40);
	m_pBeam->SetFlags(SF_BEAM_SHADEIN | SF_BEAM_SHADEOUT);
}


//=========================================================
// Purpose:
//=========================================================
void CShock::ClearBeam()
{
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
	}
}

void CShock::UpdateBeam(const Vector& start, const Vector& end)
{
	if (!m_pBeam)
	{
		// Create the beam if not already created.
		CreateBeam(start, end, SHOCK_BEAM_WIDTH);
	}
	else
	{
		m_pBeam->SetStartPos(start);
		m_pBeam->SetEndPos(end);
		m_pBeam->RelinkBeam();
	}
}

void CShock::ComputeBeamPositions(Vector* pos1, Vector* pos2)
{
	const Vector vel = pev->velocity.Normalize();
	UTIL_MakeVectors( pev->angles );

	/* Little aside so beam and sprite won't blend into white ball when looking right into shock beam direction.
	 * This should replicate the Opposing Force behavior
	 */
	const Vector origin = pev->origin - gpGlobals->v_right * 6;
	*pos1 = origin + (vel *  SHOCK_BEAM_LENGTH_HALF);
	*pos2 = origin + (vel * -SHOCK_BEAM_LENGTH_HALF/2);
}
