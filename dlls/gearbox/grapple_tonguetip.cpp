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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "grapple_tonguetip.h"

LINK_ENTITY_TO_CLASS(grapple_tonguetip, CGrappleTonguetip);

TYPEDESCRIPTION	CGrappleTonguetip::m_SaveData[] =
{
	DEFINE_FIELD(CGrappleTonguetip, m_pMyGrappler, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CGrappleTonguetip, CBaseEntity);

//=========================================================
// Purpose: Spawn
//=========================================================
void CGrappleTonguetip::Spawn(void)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING("grapple_tonguetip");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	pev->gravity = 0.01;

	SET_MODEL(ENT(pev), "models/v_bgrap_tonguetip.mdl");

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	SetTouch(&CGrappleTonguetip::TipTouch);
}

//=========================================================
// Purpose: CreateTip
//=========================================================
CGrappleTonguetip* CGrappleTonguetip::CreateTip(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CGrappleTonguetip* pTonguetip = GetClassPtr((CGrappleTonguetip *)NULL);
	pTonguetip->Spawn();

	UTIL_SetOrigin(pTonguetip->pev, vecStart);
	pTonguetip->pev->velocity = vecVelocity;
	pTonguetip->pev->owner = ENT(pevOwner);
	pTonguetip->m_pMyGrappler = GetClassPtr((CGrapple*)pevOwner);
	pTonguetip->SetThink(&CGrappleTonguetip::FlyThink);
	pTonguetip->pev->nextthink = gpGlobals->time + 0.1;

	return pTonguetip;
}

//=========================================================
// Purpose: FlyThink
//=========================================================
void CGrappleTonguetip::FlyThink(void)
{
	ALERT(at_console, "FlyThink\n");

	pev->nextthink = gpGlobals->time + 0.1f;
}

//=========================================================
// Purpose: HitThink
//=========================================================
void CGrappleTonguetip::HitThink(void)
{
	ALERT(at_console, "HitThink\n");

	pev->nextthink = gpGlobals->time + 0.1f;
}

//=========================================================
// Purpose: TipTouch
//=========================================================
void CGrappleTonguetip::TipTouch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner || (ENT(pOther->pev) == VARS(pev->owner)->owner))
		return;

	ALERT(at_console, "TipTouch\n");

	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);

	pev->velocity = Vector(0, 0, 0);

	int content = UTIL_PointContents(tr.vecEndPos);
	int hitFlags = pOther->pev->flags;

	m_pMyGrappler->m_fTipHit	= TRUE;
	m_pMyGrappler->m_iHitFlags	= hitFlags;

	if (hitFlags & (FL_CLIENT | FL_MONSTER))
	{
		// Set player attached flag.
		if (pOther->IsPlayer())
			((CBasePlayer*)pOther)->m_afPhysicsFlags |= PFLAG_ATTACHED;

		pev->movetype = MOVETYPE_FOLLOW;
		pev->aiment = ENT(pOther->pev);

		m_pMyGrappler->OnTongueTipHitEntity(pOther);
	}
	else
	{
		pev->velocity = Vector(0, 0, 0);
		pev->movetype = MOVETYPE_NONE;
		pev->gravity = 0.0f;

		m_pMyGrappler->OnTongueTipHitSurface(tr.vecEndPos);
	}

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/bgrapple_impact.wav", 1, ATTN_NORM, 0, 100);

	SetTouch( NULL );
	SetThink(&CGrappleTonguetip::HitThink);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CGrappleTonguetip::PreRemoval(void)
{
	if (pev->aiment != NULL)
	{
		CBaseEntity* pEnt = GetClassPtr((CBaseEntity*)VARS(pev->aiment));
		if (pEnt && pEnt->IsPlayer())
		{
			// Remove attached flag of the target entity.
			((CBasePlayer*)pEnt)->m_afPhysicsFlags &= ~PFLAG_ATTACHED;
		}
	}

	CBaseEntity::PreRemoval();
}