
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
#include	"gamerules.h"
#include	"customentity.h"

//=========================================================
// Shockrifle projectile
//=========================================================
class CShock : public CBaseAnimating
{
public:
	void Spawn(void);
	void Precache();

	static void Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT FlyThink();

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void CreateEffects();
	void ClearEffects();

	CBeam *m_pBeam;
	CBeam *m_pNoise;
	CSprite *m_pSprite;
};

LINK_ENTITY_TO_CLASS(shock_beam, CShock)

TYPEDESCRIPTION	CShock::m_SaveData[] =
{
	DEFINE_FIELD(CShock, m_pBeam, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pNoise, FIELD_CLASSPTR),
	DEFINE_FIELD(CShock, m_pSprite, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CShock, CBaseAnimating)

void CShock::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("shock_beam");
	SET_MODEL(ENT(pev), "models/shock_effect.mdl");
	UTIL_SetOrigin(pev, pev->origin);

	if ( g_pGameRules->IsMultiplayer() )
		pev->dmg = gSkillData.plrDmgShockroachMultiplayer;
	else
		pev->dmg = gSkillData.plrDmgShockroachSingleplayer;
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));

	CreateEffects();
	SetThink( &CShock::FlyThink );
	pev->nextthink = gpGlobals->time;
}

void CShock::Precache()
{
	PRECACHE_MODEL("sprites/flare3.spr");
	PRECACHE_MODEL("sprites/lgtning.spr");
	PRECACHE_MODEL("models/shock_effect.mdl");
	PRECACHE_SOUND("weapons/shock_impact.wav");
}

void CShock::FlyThink()
{
	if (pev->waterlevel == 3)
	{
		entvars_t *pevOwner = VARS(pev->owner);
		const int iVolume = RANDOM_FLOAT(0.8f, 1);
		const int iPitch = RANDOM_FLOAT(80, 110);
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/shock_impact.wav", iVolume, ATTN_NORM, 0, iPitch);
		RadiusDamage(pev->origin, pev, pevOwner ? pevOwner : pev, pev->dmg * 3, 144, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
		ClearEffects();
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.05;
	}
}

void CShock::Shoot(entvars_t *pevOwner, const Vector angles, const Vector vecStart, const Vector vecVelocity)
{
	CShock *pShock = GetClassPtr((CShock *)NULL);
	pShock->Spawn();

	UTIL_SetOrigin(pShock->pev, vecStart);
	pShock->pev->velocity = vecVelocity;
	pShock->pev->owner = ENT(pevOwner);
	pShock->pev->angles = angles;

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

	ClearEffects();
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
	}
	else
	{
		ClearMultiDamage();
		entvars_t *pevOwner = VARS(pev->owner);
		int damageType = DMG_ENERGYBEAM;
		if (pOther->pev->deadflag == DEAD_DEAD)
		{
			damageType |= DMG_ALWAYSGIB;
		}
		else
		{
			CBaseMonster* pMonster = pOther->MyMonsterPointer();
			if (pMonster)
				pMonster->GlowShellOn( Vector( 0, 220, 255 ), .5f );
		}
		pOther->TraceAttack(pev, pev->dmg, pev->velocity.Normalize(), &tr, damageType );
		ApplyMultiDamage(pev, pevOwner ? pevOwner : pev);
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
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CShock::CreateEffects()
{
	m_pSprite = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, FALSE );
	m_pSprite->SetAttachment( edict(), 0 );
	m_pSprite->pev->scale = 0.4;
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 170, kRenderFxNoDissipation );
	m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
	m_pSprite->pev->flags |= FL_SKIPLOCALHOST;

	m_pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pBeam)
	{
		m_pBeam->EntsInit( entindex(), entindex() );
		m_pBeam->SetStartAttachment( 1 );
		m_pBeam->SetEndAttachment( 2 );
		m_pBeam->SetBrightness( 190 );
		m_pBeam->SetScrollRate( 20 );
		m_pBeam->SetNoise( 20 );
		m_pBeam->SetFlags( BEAM_FSHADEOUT );
		m_pBeam->SetColor( 0, 255, 255 );
		m_pBeam->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pBeam->RelinkBeam();
	}

	m_pNoise = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if (m_pNoise)
	{
		m_pNoise->EntsInit( entindex(), entindex() );
		m_pNoise->SetStartAttachment( 1 );
		m_pNoise->SetEndAttachment( 2 );
		m_pNoise->SetBrightness( 190 );
		m_pNoise->SetScrollRate( 20 );
		m_pNoise->SetNoise( 65 );
		m_pNoise->SetFlags( BEAM_FSHADEOUT );
		m_pNoise->SetColor( 255, 255, 173 );
		m_pNoise->pev->spawnflags = SF_BEAM_TEMPORARY;
		m_pNoise->RelinkBeam();
	}
}

void CShock::ClearEffects()
{
	UTIL_Remove( m_pBeam );
	m_pBeam = NULL;

	UTIL_Remove( m_pNoise );
	m_pNoise = NULL;

	UTIL_Remove( m_pSprite );
	m_pSprite = NULL;
}
