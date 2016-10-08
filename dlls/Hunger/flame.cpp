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
#include	"flame.h"
#include	"weapons.h"

LINK_ENTITY_TO_CLASS(flame, CFlame);

TYPEDESCRIPTION	CFlame::m_SaveData[] =
{
	DEFINE_FIELD(CFlame, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CFlame, CBaseEntity);

void CFlame::Spawn(void)
{
	pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;
	pev->effects = EF_DIMLIGHT;

	SET_MODEL(ENT(pev), "sprites/fthrow.spr");
	pev->frame = 0;
	pev->scale = RANDOM_FLOAT(0.9f, 1.1f);
	pev->dmg = gSkillData.plrDmgFlame;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CFlame::Animate(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	pev->frame += 2;

	if (pev->frame)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = m_maxFrame;

			SetThink(&CFlame::SUB_Remove);
			pev->nextthink = gpGlobals->time;
		}
	}
}

void CFlame::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CFlame *pFlame = GetClassPtr((CFlame *)NULL);
	pFlame->Spawn();

	UTIL_SetOrigin(pFlame->pev, vecStart);
	pFlame->pev->velocity = vecVelocity;
	pFlame->pev->owner = ENT(pevOwner);
	pFlame->pev->flags |= EF_BRIGHTLIGHT; // Required to make flame glow.

	pFlame->SetThink(&CFlame::Animate);
	pFlame->pev->nextthink = gpGlobals->time + 0.1;
}

void CFlame::Touch(CBaseEntity *pOther)
{
	if (pOther->pev->takedamage)
	{
		pOther->TakeDamage(pev, pev, pev->dmg, DMG_BURN | DMG_NEVERGIB);
		SpawnBlood(pev->origin, pOther->BloodColor(), pev->dmg);
	}

	SetThink(&CFlame::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}