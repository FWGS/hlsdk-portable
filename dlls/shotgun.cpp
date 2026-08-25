/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#include "decals.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	2100
#define BOLT_WATER_VELOCITY	1700

class CShotgunBolt : public CBaseEntity
{
	void Spawn(void);
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);

	int m_iTrail;

public:
	static CShotgunBolt *BoltCreate(void);
};
LINK_ENTITY_TO_CLASS(shotgun_bolt, CShotgunBolt);

CShotgunBolt *CShotgunBolt::BoltCreate(void)
{
	// Create a new entity with CShotgunBolt private data
	CShotgunBolt *pBolt = GetClassPtr((CShotgunBolt *)NULL);
	pBolt->pev->classname = MAKE_STRING("shotgun_bolt");
	pBolt->Spawn();

	return pBolt;
}

void CShotgunBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.6;

	SET_MODEL(ENT(pev), "models/crossbow_bolt.mdl");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-1, -1, -1), Vector(1, 1, 1));

	SetTouch(&CShotgunBolt::BoltTouch);
	SetThink(&CShotgunBolt::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.2;
}


void CShotgunBolt::Precache()
{
	PRECACHE_MODEL("models/crossbow_bolt.mdl");
	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");
	PRECACHE_SOUND("weapons/xbow_hit1.wav");
	PRECACHE_SOUND("fvox/beep.wav");
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
}


int	CShotgunBolt::Classify(void)
{
	return	CLASS_NONE;
}

void CShotgunBolt::BoltTouch(CBaseEntity *pOther)
{
	SetTouch(NULL);
	SetThink(NULL);

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if (pOther->IsPlayer())
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowClient / 4, pev->velocity.Normalize(), &tr, DMG_NEVERGIB);
		}
		else
		{
			pOther->TraceAttack(pevOwner, gSkillData.plrDmgCrossbowMonster / 4, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB);
		}
		if (pOther->pev->size.z <= 90)
			pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_forward * 50 + gpGlobals->v_up * 100;

		ApplyMultiDamage(pev, pevOwner);

		pev->velocity = Vector(0, 0, 0);
		// play body "thwack" sound
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod1.wav", 1, ATTN_NORM); break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/xbow_hitbod2.wav", 1, ATTN_NORM); break;
		}

		if (!g_pGameRules->IsMultiplayer())
		{
			Killed(pev, GIB_NEVER);
		}
	}
	else
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0, 7));

		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if (FClassnameIs(pOther->pev, "worldspawn"))
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin(pev, pev->origin - vecDir * 12);
			pev->angles = UTIL_VecToAngles(vecDir);
			pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector(0, 0, 0);
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG(0, 360);
			pev->nextthink = gpGlobals->time + 10.0;
		}

		if (UTIL_PointContents(pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(pev->origin);
		}
	}

	if (g_pGameRules->IsMultiplayer())
	{
		SetThink(&CShotgunBolt::ExplodeThink);
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CShotgunBolt::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
		return;

	UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 1);
}

void CShotgunBolt::ExplodeThink(void)
{
	int iContents = UTIL_PointContents(pev->origin);
	int iScale;

	pev->dmg = 40;
	iScale = 10;

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	if (iContents != CONTENTS_WATER)
	{
		WRITE_SHORT(g_sModelIndexFireball);
	}
	else
	{
		WRITE_SHORT(g_sModelIndexWExplosion);
	}
	WRITE_BYTE(iScale); // scale * 10
	WRITE_BYTE(15); // framerate
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();

	entvars_t *pevOwner;

	if (pev->owner)
		pevOwner = VARS(pev->owner);
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage(pev->origin, pev, pevOwner, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB);

	UTIL_Remove(this);
}
#endif


#ifndef CLIENT_DLL
class CSGSpit : public CBaseEntity
{
public:
	void Spawn(void);

	static void Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity *pOther);
	void EXPORT Animate(void);
	void Precache();

	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int  m_maxFrame;
	int iSquidSpitSprite;
};

LINK_ENTITY_TO_CLASS(shotgunspit, CSGSpit);

TYPEDESCRIPTION	CSGSpit::m_SaveData[] =
{
	DEFINE_FIELD(CSGSpit, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CSGSpit, CBaseEntity);

void CSGSpit::Spawn(void)
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("shotgunspit");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/tinyspit.spr");
	pev->frame = 0;
	pev->scale = 1;

	UTIL_SetSize(pev, Vector(-1, -1, -1), Vector(1, 1, 1));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}


void CSGSpit::Precache()
{
	iSquidSpitSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.
}

void CSGSpit::Animate(void)
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
}

void CSGSpit::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity)
{
	CSGSpit *pSpit = GetClassPtr((CSGSpit *)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink(&CSGSpit::Animate);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CSGSpit::Touch(CBaseEntity *pOther)
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT(90, 110);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}

	if (!pOther->pev->takedamage)
	{

		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0, 1));

		// make some flecks
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(tr.vecEndPos.x);	// pos
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_COORD(tr.vecPlaneNormal.x);	// dir
		WRITE_COORD(tr.vecPlaneNormal.y);
		WRITE_COORD(tr.vecPlaneNormal.z);
		WRITE_SHORT(iSquidSpitSprite);	// model
		WRITE_BYTE(9);			// count
		WRITE_BYTE(30);			// speed
		WRITE_BYTE(155);			// noise ( client will divide by 100 )
		MESSAGE_END();
	}
	else
	{
		UTIL_MakeVectors(pev->angles);
		Vector spitdir = ((pOther->pev->origin + pOther->pev->view_ofs)).Normalize();

		// make some flecks
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(pev->origin.x);	// pos
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(spitdir.x);	// dir
		WRITE_COORD(spitdir.y);
		WRITE_COORD(spitdir.z);
		WRITE_SHORT(iSquidSpitSprite);	// model
		WRITE_BYTE(12);			// count
		WRITE_BYTE(80);			// speed
		WRITE_BYTE(155);			// noise ( client will divide by 100 )
		MESSAGE_END();
		//pOther->TakeDamage(pev, pev, gSkillData.bullsquidDmgSpit, DMG_ACID);
	}
	RadiusDamage(pev->origin, pev, pev, 25, 50, CLASS_NONE, DMG_GENERIC);

	SetThink(&CBaseEntity::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}
#endif

void CShotgun::AcidEffect(void)
{
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 10 + gpGlobals->v_forward * 18;
	Vector vecAiming = gpGlobals->v_forward;

	/*TraceResult tr;
	UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	#ifndef CLIENT_DLL
	UTIL_DecalTrace(&tr, DECAL_SPIT1 + RANDOM_LONG(0, 1));
	// spew the spittle temporary ents.
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
	WRITE_BYTE(TE_SPRITE_SPRAY);
	WRITE_COORD(tr.vecEndPos.x);	// pos
	WRITE_COORD(tr.vecEndPos.y);
	WRITE_COORD(tr.vecEndPos.z);
	WRITE_COORD(tr.vecPlaneNormal.x);	// dir
	WRITE_COORD(tr.vecPlaneNormal.y);
	WRITE_COORD(tr.vecPlaneNormal.z);
	WRITE_SHORT(SGAcidSprite);	// model
	WRITE_BYTE(30);			// count
	WRITE_BYTE(55);			// speed
	WRITE_BYTE(105);			// noise ( client will divide by 100 )
	MESSAGE_END();
	#endif

	if (tr.pHit)
	{
	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
	if (pEntity != NULL && pEntity->pev->takedamage)
	{
	pEntity->TakeDamage(pev, pev, 8, DMG_POISON);

	//pEntity->TraceAttack( pev, 8, vecAim, &tr, DMG_ENERGYBEAM );
	}
	}*/
}

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00 )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP,
	SHOTGUN_SMASH
};

LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun )

BOOL CShotgun::IsUseable(void)
{
	return TRUE;
}

void CShotgun::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL( ENT( pev ), "models/w_shotgun.mdl" );

	m_iDefaultAmmo = SHOTGUN_MAX_CLIP;

	FallInit();// get ready to fall
}

void CShotgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_shotgun.mdl" );
	PRECACHE_MODEL( "models/w_shotgun.mdl" );
	PRECACHE_MODEL( "models/p_shotgun.mdl" );

	UTIL_PrecacheOther("shotgun_bolt");
	UTIL_PrecacheOther("shotgunspit");

	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );// shotgun shell
	SGAcidSprite = PRECACHE_MODEL("sprites/tinyspit.spr");// client side spittle.

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND("weapons/shotgun_slap.wav");
	PRECACHE_SOUND("weapons/shotgun_flame.wav");

	PRECACHE_SOUND("debris/zap1.wav");
	PRECACHE_SOUND("debris/zap3.wav");
	PRECACHE_SOUND("debris/zap8.wav");

	PRECACHE_SOUND("bullchicken/bc_acid1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit1.wav");
	PRECACHE_SOUND("bullchicken/bc_spithit2.wav");

	PRECACHE_SOUND( "weapons/dbarrel1.wav" );//shotgun
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );//shotgun

	PRECACHE_SOUND( "weapons/reload1.wav" );	// shotgun reload
	PRECACHE_SOUND( "weapons/reload3.wav" );	// shotgun reload

	//PRECACHE_SOUND( "weapons/sshell1.wav" );	// shotgun reload - played on client
	//PRECACHE_SOUND( "weapons/sshell3.wav" );	// shotgun reload - played on client
	
	PRECACHE_SOUND( "weapons/357_cock1.wav" ); // gun empty sound
	PRECACHE_SOUND( "weapons/scock1.wav" );	// cock gun

	m_usSingleFire = PRECACHE_EVENT( 1, "events/shotgun1.sc" );
	m_usDoubleFire = PRECACHE_EVENT( 1, "events/shotgun2.sc" );
}

int CShotgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CShotgun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "sgunammo";
	p->iMaxAmmo1 = SHOTGUN_MAX_CLIP;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = NULL;
	p->iSlot = 2;
	p->iPosition = 1;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}

BOOL CShotgun::Deploy()
{
	return DefaultDeploy( "models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun" );
}

void CShotgun::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	//!!!HACKHACK - can't select hornetgun if it's empty! no way to get ammo for it, either.
	if (!m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}

void CShotgun::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.15f );
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		Reload();
		if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0 )
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	/*0 Regular
	1 Crossbow darts
	2 Acid spray
	3 Flames
	4 Sparks
	5 Trash*/

	SendWeaponAnim(SHOTGUN_FIRE);
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM, 0, 93 + RANDOM_LONG(0, 0x1f));

	switch (m_pPlayer->shotgunfiretype)
	{
	case 0: FireDarts(); break;
	case 1: FireAcid(); break;
	case 2: FireFlames(); break;
	case 3: FireSparks(); break;
	//case 4: FireSparks(); break;
	//case 5: FireTrash(); break;
	}

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSingleFire, 0.0, g_vecZero, g_vecZero, 0, 0, 0, 0, 0, 0 );

	//if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
	//	m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	//if( m_iClip != 0 )
		m_flPumpTime = gpGlobals->time + 0.05f;

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.35f );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.35f;
	if( m_iClip != 0 )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;
	else
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.35f;
	m_fInSpecialReload = 0;
}

void CShotgun::FireTrash(void)
{

}

void CShotgun::FireFlames(void)
{
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	for (int i = 0; i < 3; i++)
	{
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 2048 + gpGlobals->v_right * RANDOM_LONG(-96, 96) + gpGlobals->v_up * RANDOM_LONG(-96, 96), dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);		// This makes a dynamic light and the explosion sprites/sound
		WRITE_COORD(tr.vecEndPos.x);	// Send to PAS because of the sound
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE(5); // scale * 10
		WRITE_BYTE(25); // framerate
		WRITE_BYTE(TE_EXPLFLAG_NOSOUND);
		MESSAGE_END();
#endif
		RadiusDamage(tr.vecEndPos, m_pPlayer->pev, m_pPlayer->pev, 20, 50, CLASS_NONE, DMG_BLAST);
		UTIL_DecalTrace(&tr, RANDOM_LONG(37,39));
	}
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/shotgun_flame.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));
}

void CShotgun::FireSparks(void)
{
	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;

	for (int i = 0; i < 5; i++)
	{
		UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 2048 + gpGlobals->v_right * RANDOM_LONG(-96, 96) + gpGlobals->v_up * RANDOM_LONG(-96, 96), dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);
#ifndef CLIENT_DLL
		UTIL_Sparks(tr.vecEndPos);
#endif
		RadiusDamage(tr.vecEndPos, m_pPlayer->pev, m_pPlayer->pev, 10, 50, CLASS_NONE, DMG_BLAST);
		Create("spark_dmgshower", tr.vecEndPos, -tr.vecPlaneNormal, NULL);
	}
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "debris/zap1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f)); break;
	case 1: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "debris/zap3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f)); break;
	case 2: EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_VOICE, "debris/zap8.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f)); break;
	}
	
}

void CShotgun::FireAcid(void)
{
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	AcidEffect();
#ifndef CLIENT_DLL
	CSGSpit::Shoot(pev, m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 32 + gpGlobals->v_up * RANDOM_LONG(-10, 10), gpGlobals->v_forward * RANDOM_LONG(1400, 1500));
	CSGSpit::Shoot(pev, m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 32 + gpGlobals->v_up * RANDOM_LONG(-10, 10) - gpGlobals->v_right * 20, gpGlobals->v_forward * RANDOM_LONG(1400,1500));
	CSGSpit::Shoot(pev, m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 32 + gpGlobals->v_up * RANDOM_LONG(-10, 10) + gpGlobals->v_right * 20, gpGlobals->v_forward * RANDOM_LONG(1400, 1500));
#endif
	return;
}


void CShotgun::FireDarts(void)
{
	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);

	anglesAim.x = -anglesAim.x;
	Vector vecSrc[5];
	vecSrc[0] = m_pPlayer->GetGunPosition();
	vecSrc[1] = m_pPlayer->GetGunPosition() + gpGlobals->v_right * 3;
	vecSrc[2] = m_pPlayer->GetGunPosition() + gpGlobals->v_right * 6;
	vecSrc[3] = m_pPlayer->GetGunPosition() - gpGlobals->v_right * 3;
	vecSrc[4] = m_pPlayer->GetGunPosition() - gpGlobals->v_right * 6;
	Vector vecDir[5];
	vecDir[0] = gpGlobals->v_forward;
	vecDir[1] = gpGlobals->v_forward + gpGlobals->v_right * 0.05f;
	vecDir[2] = gpGlobals->v_forward + gpGlobals->v_right * 0.1f;
	vecDir[3] = gpGlobals->v_forward - gpGlobals->v_right * 0.05f;
	vecDir[4] = gpGlobals->v_forward - gpGlobals->v_right * 0.1f;

#ifndef CLIENT_DLL
	CShotgunBolt *pBolt[5];
	for (int i = 0; i < 5; i++)
	{
		pBolt[i] = CShotgunBolt::BoltCreate();
		pBolt[i]->pev->origin = vecSrc[i] + gpGlobals->v_forward * RANDOM_LONG(-5, 5) + gpGlobals->v_up * RANDOM_LONG(-10, 10);
		pBolt[i]->pev->angles = anglesAim;
		pBolt[i]->pev->owner = m_pPlayer->edict();

		if (m_pPlayer->pev->waterlevel == 3)
		{
			pBolt[i]->pev->velocity = vecDir[i] * BOLT_WATER_VELOCITY;
			pBolt[i]->pev->speed = BOLT_WATER_VELOCITY;
		}
		else
		{
			pBolt[i]->pev->velocity = vecDir[i] * BOLT_AIR_VELOCITY;
			pBolt[i]->pev->speed = BOLT_AIR_VELOCITY;
		}
		pBolt[i]->pev->avelocity.z = 10;
	}
#endif
	return;
}

void CShotgun::SecondaryAttack(void)
{

	if (m_pPlayer->shotgunfiretype < 3)
		m_pPlayer->shotgunfiretype++;
	else
		m_pPlayer->shotgunfiretype = 0;
	

	SendWeaponAnim(SHOTGUN_SMASH);

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/shotgun_slap.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG(0, 0x1f));

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
	if( m_iClip != 0 )
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 6.0f;
	else
		m_flTimeWeaponIdle = 1.0f;

	m_fInSpecialReload = 0;
}

void CShotgun::Reload( void )
{
	//if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == SHOTGUN_MAX_CLIP )
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == SHOTGUN_MAX_CLIP )
		return;

	// don't reload until recoil is done
	if( m_flNextPrimaryAttack > UTIL_WeaponTimeBase() )
		return;

	// check to see if we're ready to reload
	if( m_fInSpecialReload == 0 )
	{
		SendWeaponAnim( SHOTGUN_START_RELOAD );
		m_fInSpecialReload = 1;
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.15f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.15f;
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.25f );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.25f;
		return;
	}
	else if( m_fInSpecialReload == 1 )
	{
		if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
			return;
		// was waiting for gun to move to side
		m_fInSpecialReload = 2;

		if( RANDOM_LONG( 0, 1 ) )
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload1.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );
		else
			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/reload3.wav", 1, ATTN_NORM, 0, 85 + RANDOM_LONG( 0, 0x1f ) );

		SendWeaponAnim( SHOTGUN_RELOAD );

		m_flNextReload = UTIL_WeaponTimeBase() + 0.15f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.15f;
	}
	else
	{
		// Add them to the clip
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] += 1;
		// m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 1;
		m_fInSpecialReload = 1;
	}
}

void CShotgun::ItemPostFrame( void )
{
	if( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
		m_flPumpTime = 0;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

void CShotgun::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		//if( m_iClip == 0 && m_fInSpecialReload == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		if( m_iClip == 0 && m_fInSpecialReload == 0 )
		{
			Reload();
		}
		else if( m_fInSpecialReload != 0 )
		{
			//if( m_iClip != SHOTGUN_MAX_CLIP && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
			if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] != 8)
			{
				Reload();
			}
			else
			{
				// reload debounce has timed out
				SendWeaponAnim( SHOTGUN_PUMP );
				
				// play cocking sound
				EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
				m_fInSpecialReload = 0;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5f;
			}
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if( flRand <= 0.8f )
			{
				iAnim = SHOTGUN_IDLE_DEEP;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 60.0f / 12.0f );// * RANDOM_LONG( 2, 5 );
			}
			else if( flRand <= 0.95f )
			{
				iAnim = SHOTGUN_IDLE;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 20.0f / 9.0f );
			}
			else
			{
				iAnim = SHOTGUN_IDLE4;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 20.0f / 9.0f );
			}
			SendWeaponAnim( iAnim );
		}
	}
}

class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_shotbox.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_shotbox.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if( pOther->GiveAmmo( AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY ) != -1 )
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_buckshot, CShotgunAmmo )
