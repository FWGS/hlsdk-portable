/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"schedule.h"
#include	"nodes.h"
#include	"effects.h"
#include	"decals.h"
#include	"soundent.h"
#include	"game.h"
#include	"xenspit.h"

LINK_ENTITY_TO_CLASS(xensmallspit, CXenSmallSpit);

TYPEDESCRIPTION	CXenSmallSpit::m_SaveData[] =
{
	DEFINE_FIELD(CXenSmallSpit, m_iTrail, FIELD_INTEGER),
	DEFINE_FIELD(CXenSmallSpit, m_flCycle, FIELD_FLOAT),
	DEFINE_FIELD(CXenSmallSpit, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CXenSmallSpit, m_vecOldVelocity, FIELD_VECTOR),
};

IMPLEMENT_SAVERESTORE(CXenSmallSpit, CBaseEntity);

void CXenSmallSpit::Spawn(void)
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("xensmallspit");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/hotglow.spr");
	pev->frame = 0;
	pev->scale = 0.5;
	pev->gravity = 0;
	pev->dmg = gSkillData.plrDmgGauss;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	// trail
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE(  TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );		// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 10 ); // life
		WRITE_BYTE( 4 );  // width
		WRITE_BYTE( 161 );   // r, g, b
		WRITE_BYTE( 188 );   // r, g, b
		WRITE_BYTE( 0 );   // r, g, b
		WRITE_BYTE( 128 );	// brightness

	MESSAGE_END();

	m_pBeam = NULL;
	m_pParent = NULL;
}

void CXenSmallSpit::Precache()
{
	PRECACHE_MODEL("sprites/hotglow.spr");

	m_iTrail = PRECACHE_MODEL("sprites/laserbeam.spr");

	PRECACHE_MODEL("sprites/laserbeam.spr");
}

CXenSmallSpit* CXenSmallSpit::ShootStraight(entvars_t *pevOwner, Vector vecStart, Vector vecAngles, Vector vecVelocity)
{
	CXenSmallSpit *pSpit = GetClassPtr((CXenSmallSpit *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->angles = vecAngles;
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->m_vecOldVelocity = vecVelocity;

	pSpit->m_flCycle = 0.0f;
	pSpit->m_pParent = NULL;

	pSpit->SetThink(&CXenSmallSpit::StraightThink);
	pSpit->pev->nextthink = gpGlobals->time + 0.01;

	return pSpit;
}

CXenSmallSpit* CXenSmallSpit::ShootCycle(entvars_t *pevOwner, Vector vecStart, Vector vecAngles, Vector vecVelocity, CBaseEntity* pParent, float flCycle)
{
	CXenSmallSpit *pSpit = GetClassPtr((CXenSmallSpit *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->angles = vecAngles;
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);
	pSpit->m_vecOldVelocity = vecVelocity;

	pSpit->m_flCycle = flCycle;
	pSpit->m_pParent = pParent;

	pSpit->SetThink(&CXenSmallSpit::CycleThink);
	pSpit->pev->nextthink = gpGlobals->time + 0.01;

	return pSpit;
}

void CXenSmallSpit::Touch(CBaseEntity *pOther)
{
	if (m_pParent != NULL)
		return;

	if (FClassnameIs(pOther->pev, STRING(pev->classname)))
		return;

	// ALERT(at_console, " CXenSmallSpit::Touch START\n");

	TraceResult tr;

	RadiusDamage(pev->origin, pev, pev, pev->dmg, 80, CLASS_NONE, DMG_POISON | DMG_ALWAYSGIB);

	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0, 2));
	}

	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = NULL;
	}

	// light.
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(pev->origin.x);	// X
	WRITE_COORD(pev->origin.y);	// Y
	WRITE_COORD(pev->origin.z);	// Z
	WRITE_BYTE(8);		// radius * 0.1
	WRITE_BYTE(161);	// r
	WRITE_BYTE(188);	// g
	WRITE_BYTE(0);		// b
	WRITE_BYTE(10);		// time * 10
	WRITE_BYTE(2);		// decay * 0.1
	MESSAGE_END();

	m_pParent = NULL;

	SetThink(&CXenSmallSpit::SUB_Remove);
	SetTouch(NULL);
	pev->nextthink = gpGlobals->time;

	// ALERT(at_console, " CXenSmallSpit::Touch END\n");
}

void CXenSmallSpit::StraightThink(void)
{
	pev->nextthink = gpGlobals->time + 0.01f;

	m_flCycle += 0.02f;

	if (m_flCycle >= 1)
		m_flCycle = 0;

	pev->velocity.z = m_vecOldVelocity.z + sin(m_flCycle * 2 * M_PI) * 16;
}

void CXenSmallSpit::CycleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.01f;

	m_flCycle += 0.02f;

	if (m_flCycle >= 1)
		m_flCycle = 0;
}





LINK_ENTITY_TO_CLASS(xenlargespit, CXenLargeSpit);

TYPEDESCRIPTION	CXenLargeSpit::m_SaveData[] =
{
	DEFINE_FIELD(CXenLargeSpit, m_iChildCount, FIELD_INTEGER),
	DEFINE_ARRAY(CXenLargeSpit, m_pChildren, FIELD_CLASSPTR, XENSPIT_MAX_PROJECTILES),
};

IMPLEMENT_SAVERESTORE(CXenLargeSpit, CBaseEntity);


void CXenLargeSpit::Spawn(void)
{
	Precache();

	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("xenlargespit");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/glow02.spr");
	pev->frame = 0;
	pev->scale = 0.5;
	pev->gravity = 0;
	pev->dmg = gSkillData.plrDmgGauss;

	pev->renderamt = 0;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	for (int i = 0; i < XENSPIT_MAX_PROJECTILES; i++)
		m_pChildren[i] = NULL;
}

void CXenLargeSpit::Precache()
{
	PRECACHE_MODEL("sprites/glow02.spr");
}

CXenLargeSpit* CXenLargeSpit::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecAngles, Vector vecVelocity)
{
	CXenLargeSpit *pSpit = GetClassPtr((CXenLargeSpit *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->angles = vecAngles;
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink(&CXenLargeSpit::CycleThink);
	pSpit->pev->nextthink = gpGlobals->time + 0.01;

	return pSpit;
}

void CXenLargeSpit::Touch(CBaseEntity *pOther)
{
	// ALERT(at_console, " CXenLargeSpit::Touch START\n");

	float damage = Q_max(pev->dmg, pev->dmg * m_iChildCount);

	RadiusDamage(pev->origin, pev, pev, damage, 110, CLASS_NONE, DMG_POISON | DMG_ALWAYSGIB);

	TraceResult tr;

	if (!pOther->pev->takedamage)
	{
		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0, 2));
	}

	for (int i = 0; i < m_iChildCount; i++)
	{
		if (m_pChildren[i])
		{
			UTIL_Remove(m_pChildren[i]);
			m_pChildren[i] = NULL;
		}
	}

	m_iChildCount = 0;

	// light.
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_DLIGHT);
	WRITE_COORD(pev->origin.x);	// X
	WRITE_COORD(pev->origin.y);	// Y
	WRITE_COORD(pev->origin.z);	// Z
	WRITE_BYTE(12);		// radius * 0.1
	WRITE_BYTE(161);	// r
	WRITE_BYTE(188);	// g
	WRITE_BYTE(0);		// b
	WRITE_BYTE(10);		// time * 10
	WRITE_BYTE(2);		// decay * 0.1
	MESSAGE_END();

	SetThink(&CXenLargeSpit::SUB_Remove);
	SetTouch(NULL);
	pev->nextthink = gpGlobals->time + 0.1f;

	// ALERT(at_console, " CXenLargeSpit::Touch START\n");
}

void CXenLargeSpit::CycleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1f;

	Vector src, forward, right, up, cross;

	forward = pev->velocity.Normalize();

	Vector horizontal = Vector(pev->velocity.x, pev->velocity.y, 0).Normalize();
	Vector direction = forward;

	float flDot = DotProduct(direction, horizontal);
	if (flDot < 0.5)
		cross = Vector(1, 0, 0);
	else
		cross = Vector(0, 0, 1);

	right = CrossProduct(direction, cross);
	up = CrossProduct(right, direction);

	for (int i = 0; i < m_iChildCount; i++)
	{
		CXenSmallSpit* pSpit = (CXenSmallSpit*)m_pChildren[i];
		if (pSpit)
		{
			float cs, sn, dist;
			cs = cos(pSpit->m_flCycle * 2 * M_PI);
			sn = sin(pSpit->m_flCycle * 2 * M_PI);
			dist = Q_max(2, 2 * m_iChildCount);

			// ALERT(at_console, "cs: %.2f. sn: %.2f\n", cs, sn);

			Vector target = (right * cs * dist) + (up * sn * dist);

			src = pev->origin + target * -2;
			pSpit->pev->origin = src;
			pSpit->pev->velocity = pev->velocity;
		}
	}
}


