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
#if !OEM_BUILD && !HLDEMO_BUILD

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "hornet.h"
#include "gamerules.h"

#ifndef CLIENT_DLL
#define BOLT_AIR_VELOCITY	900
#define BOLT_WATER_VELOCITY	600


class CWave : public CBaseEntity
{
	void Spawn();
	void Precache(void);
	int  Classify(void);
	void EXPORT BubbleThink(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT ExplodeThink(void);

	int m_iTrail;
	int m_iSpriteTexture;
	int boltlife;

public:
	static CWave *BoltCreate(int type);
};
LINK_ENTITY_TO_CLASS(wandwave, CWave);

CWave *CWave::BoltCreate(int type)
{
	// Create a new entity with CCrossbowBolt private data
	CWave *pBolt = GetClassPtr((CWave *)NULL);
	pBolt->pev->classname = MAKE_STRING("wandwave");
	pBolt->Spawn();

	return pBolt;
}

void CWave::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_TRIGGER;
	pev->gravity = 0.8;
	pev->speed = 1800;
	SET_MODEL(ENT(pev), "sprites/shockwave.spr");
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0;
	pev->frame = 0;
	pev->framerate = 8;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 128;
	pev->rendercolor.z = 255;
	pev->scale = 1;
	pev->angles.x = 90;

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(-24, -24, -2), Vector(24, 24, 2));
	boltlife = 20;

	SetTouch(&CWave::BoltTouch);
	SetThink(&CWave::BubbleThink);
	pev->nextthink = gpGlobals->time + 0.05;
}


void CWave::Precache()
{
	m_iTrail = PRECACHE_MODEL("sprites/streak.spr");
	m_iSpriteTexture = PRECACHE_MODEL("sprites/shockwave.spr");
}


int	CWave::Classify(void)
{
	return	CLASS_NONE;
}

void CWave::BoltTouch(CBaseEntity *pOther)
{
	if (pOther->pev->takedamage)
	{

		TraceResult tr = UTIL_GetGlobalTrace();
		entvars_t	*pevOwner;

		pevOwner = VARS(pev->owner);

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage();

		if (pOther->edict() == pev->owner)
			return;
		pOther->TraceAttack(pevOwner, 2, pev->velocity.Normalize(), &tr, DMG_SONIC);

		//if (pOther->pev->size.y <= 90)
		//{
			pOther->pev->punchangle.x = 5;
			pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_forward * 100;
			pOther->pev->velocity = pOther->pev->velocity + gpGlobals->v_up * 50;
		//}
		pOther->pev->speed = pOther->pev->speed / 2;

		ApplyMultiDamage(pev, pevOwner);

	}

}

void CWave::BubbleThink(void)
{
	pev->nextthink = gpGlobals->time + 0.05;
	// blast circles
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 16);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 16 + 86 / .2); // reach damage radius over .3 seconds
	WRITE_SHORT(m_iSpriteTexture);
	WRITE_BYTE(0); // startframe
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(2); // life
	WRITE_BYTE(20);  // width
	WRITE_BYTE(50);   // noise

	WRITE_BYTE(255);
	WRITE_BYTE(206);
	WRITE_BYTE(255);

	WRITE_BYTE(35); //brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();
	entvars_t *pevOwner = NULL;
	if (pev->owner)
		pevOwner = VARS(pev->owner);

	boltlife--;
	if (boltlife <= 0)
	{
		SetTouch(NULL);
		SetThink(NULL);
		UTIL_Remove(this);
	}

}

class CSparkle : public CBaseEntity
{
	void Spawn(int type);
	void Precache(void);
	int  Classify(void);
	void EXPORT BoltTouch(CBaseEntity *pOther);
	void EXPORT BoltThink(void);
	int bolttype;
	int m_iTrail;

public:
	static CSparkle *BoltCreate(int type);
};
LINK_ENTITY_TO_CLASS(sparkle, CSparkle);

CSparkle *CSparkle::BoltCreate(int type)
{
	// Create a new entity with CCrossbowBolt private data
	CSparkle *pBolt = GetClassPtr((CSparkle *)NULL);
	pBolt->pev->classname = MAKE_STRING("sparkle");
	pBolt->Spawn(type);
	pBolt->bolttype = type;
	return pBolt;
}

void CSparkle::Spawn(int type)
{
	Precache();
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_BBOX;
	SET_MODEL(ENT(pev), "sprites/muz4.spr");
	pev->rendermode = kRenderTransColor;
	pev->renderamt = 255;
	pev->gravity = 0.1;
	pev->frame = 0;
	pev->framerate = 8;
	pev->health = 50;
	if (type == 0)
	{
		pev->scale = 0.3;
		UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	}
	else if (type == 1)
	{
		pev->scale = 0.2;
		UTIL_SetSize(pev, Vector(-0.2, -0.2, -0.2), Vector(0.2, 0.2, 0.2));
	}
	else
	{
		pev->scale = 0.8;
		UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));
	}
	// rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex());	// entity
	WRITE_SHORT(m_iTrail);	// model
	WRITE_BYTE(15); // life
	WRITE_BYTE(3);  // width
	WRITE_BYTE(245);   // r, g, b
	WRITE_BYTE(45);   // r, g, b
	WRITE_BYTE(215);   // r, g, b
	WRITE_BYTE(255);	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = rand() % 255;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	SetTouch(&CSparkle::BoltTouch);
	SetThink(&CSparkle::BoltThink);
	pev->nextthink = gpGlobals->time + 0.2;
}

void CSparkle::Precache()
{
	PRECACHE_MODEL("sprites/muz7.spr");
	m_iTrail = PRECACHE_MODEL("sprites/muz4.spr");
	PRECACHE_SOUND("buttons/spark4.wav");
	PRECACHE_SOUND("buttons/spark5.wav");
	PRECACHE_SOUND("buttons/spark6.wav");
	UTIL_PrecacheOther("monster_rat");
}


int	CSparkle::Classify(void)
{
	return	CLASS_NONE;
}

void CSparkle::BoltThink(void)
{
	pev->nextthink = gpGlobals->time + 0.1;
	pev->health--;
	if (pev->health <= 0)
	{
		SetTouch(NULL);
		SetThink(NULL);
		UTIL_Remove(this);
	}
}

void CSparkle::BoltTouch(CBaseEntity *pOther)
{
	//if (FClassnameIs(pOther->pev, "sparkle"))
	//	return;

	SetTouch(NULL);
	SetThink(NULL);
	UTIL_Remove(this);
	if (pOther)
	{
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
		if (pOther->edict() == pev->owner)
			return;
		if (bolttype == 1)
		{
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->origin + Vector(0, 0, -32), ignore_monsters, ENT(pev), &tr);
			MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
			WRITE_BYTE(TE_EXPLOSION);		// This makes a dynamic light and the explosion sprites/sound
			WRITE_COORD(pev->origin.x);	// Send to PAS because of the sound
			WRITE_COORD(pev->origin.y);
			WRITE_COORD(pev->origin.z);
			WRITE_SHORT(g_sModelIndexFireball);
			WRITE_BYTE(15); // scale * 10
			WRITE_BYTE(30); // framerate
			WRITE_BYTE(TE_EXPLFLAG_NONE);
			MESSAGE_END();
			RadiusDamage(pev->origin, pev, pev, 40, 128, CLASS_NONE, DMG_ENERGYBEAM);
		}
		if (pOther->pev->takedamage && !pOther->IsBSPModel() && !FClassnameIs(pOther->pev, "monster_ginastreamer") && !FClassnameIs(pOther->pev, "monster_oetker") && !FClassnameIs(pOther->pev, "monster_georgedroid") && !FClassnameIs(pOther->pev, "monster_varg"))
		{
			if (bolttype == 2)
			{
				CBaseMonster *pMonster = pOther->MyMonsterPointer();
				if (pMonster)
				{
					pOther->Killed(pev, 1);
					pOther->SetThink(&CBaseEntity::SUB_Remove);
					CBaseEntity *pReplacement = Create("monster_rat", pOther->pev->origin, pOther->pev->angles);
					pReplacement->pev->angles.y = RANDOM_LONG(1, 360);
					MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
					WRITE_BYTE(TE_IMPLOSION);
					WRITE_COORD(pReplacement->pev->origin.x);
					WRITE_COORD(pReplacement->pev->origin.y);
					WRITE_COORD(pReplacement->pev->origin.z);
					WRITE_BYTE(40);  // radius
					WRITE_BYTE(25); // count
					WRITE_BYTE(5); // life
					MESSAGE_END();
				}
			}

			TraceResult tr = UTIL_GetGlobalTrace();
			entvars_t	*pevOwner;
			pevOwner = VARS(pev->owner);
			// UNDONE: this needs to call TraceAttack instead
			ClearMultiDamage();
			if (bolttype == 0)
			{
				CBaseMonster *pMonster = pOther->MyMonsterPointer();
				if (pMonster)
				{
					if (pMonster->IsAlive() || pMonster->pev->health > 0)
					{
						if (!FClassnameIs(pMonster->pev, "monster_ginastreamer") && !FClassnameIs(pMonster->pev, "monster_varg") && !FClassnameIs(pMonster->pev, "monster_georgedroid") && !FClassnameIs(pMonster->pev, "monster_oetker"))
						{
							pMonster->AddShockEffect(251, 45, 215, 16, 1);
							pMonster->frozen = 25;
						}
					}
				}
				pOther->TraceAttack(pevOwner, 1, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM);
			}
			else
			{
				pOther->TraceAttack(pevOwner, 24, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM);
			}
			ApplyMultiDamage(pev, pevOwner);
			pev->velocity = Vector(0, 0, 0);
		}
		else
		{
			TraceResult tr;
			UTIL_TraceLine(pev->origin, pev->origin + pev->velocity.Normalize() * 64, ignore_monsters, ENT(pev), &tr);
			// Pull out of the wall a bit
			if (tr.flFraction != 1.0)
			{
				pev->origin = tr.vecEndPos + (tr.vecPlaneNormal * (pev->dmg - 24) * 0.02);
			}

			// draw decal
			if (!(pev->spawnflags))
			{
				if (bolttype == 1)
				{
					UTIL_DecalTrace(&tr, 11);
				}
			}
	}
	}
}


#endif


#define SPELL_SPARKLE 0
#define SPELL_BARRAGE 1
#define SPELL_SHOCKWAVE 2
#define SPELL_CHUMTOAD 3

enum hgun_e
{
	WAND_IDLE1 = 0,
	WAND_SPELL1,
	WAND_SPELL2,
	WAND_SPELL3,
	WAND_HOLSTER,
	WAND_DRAW
};

enum firemode_e
{
	FIREMODE_TRACK = 0,
	FIREMODE_FAST
};

LINK_ENTITY_TO_CLASS( weapon_hornetgun, CHgun )

BOOL CHgun::IsUseable( void )
{
	return TRUE;
}

void CHgun::Spawn()
{
	Precache();
	m_iId = WEAPON_HORNETGUN;
	SET_MODEL( ENT( pev ), "models/w_wand.mdl" );

	m_iDefaultAmmo = HIVEHAND_DEFAULT_GIVE;
	m_iFirePhase = 0;

	FallInit();// get ready to fall down.
}

void CHgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_wand.mdl" );
	PRECACHE_MODEL( "models/w_wand.mdl" );
	PRECACHE_MODEL( "models/p_hgun.mdl" );

	PRECACHE_SOUND("weapons/wand1.wav"); //sparkle
	PRECACHE_SOUND("weapons/wand2.wav"); //barrage
	PRECACHE_SOUND("weapons/wand3.wav"); //push force
	PRECACHE_SOUND("weapons/wand4.wav"); //chumtoad
	PRECACHE_MODEL("sprites/shockwave.spr");

	UTIL_PrecacheOther("sparkle");
	UTIL_PrecacheOther("wandwave");

	m_usHornetFire = PRECACHE_EVENT( 1, "events/firehornet.sc" );

	UTIL_PrecacheOther( "hornet" );
}

int CHgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
#if !CLIENT_DLL
		if( g_pGameRules->IsMultiplayer() )
		{
			// in multiplayer, all hivehands come full. 
			pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = HORNET_MAX_CARRY;
		}
#endif
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CHgun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hornets";
	p->iMaxAmmo1 = HORNET_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_HORNETGUN;
	p->iFlags = ITEM_FLAG_NOAUTOSWITCHEMPTY | ITEM_FLAG_NOAUTORELOAD;
	p->iWeight = HORNETGUN_WEIGHT;

	return 1;
}

BOOL CHgun::Deploy()
{
	return DefaultDeploy( "models/v_wand.mdl", "models/p_hgun.mdl", WAND_DRAW, "hive" );
}

void CHgun::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( WAND_HOLSTER );

	//!!!HACKHACK - can't select hornetgun if it's empty! no way to get ammo for it, either.
	if( !m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] )
	{
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 1;
	}
}

void CHgun::PrimaryAttack()
{
	Reload();

	switch (m_pPlayer->wandspelltype)
	{
	case SPELL_SPARKLE:
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 5)
			return;

		SparkleAttack();
		SendWeaponAnim(WAND_SPELL2);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/wand1.wav", 1, ATTN_NORM);
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] -= 5;
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + 1.0;
	}
	break;
	case SPELL_BARRAGE:
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 20)
			return;

		BarrageAttack();
		SendWeaponAnim(WAND_SPELL1);
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/wand2.wav", 1, ATTN_NORM);
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] -= 20;
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + 1.5;
	}
	break;
	case SPELL_SHOCKWAVE:
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 30)
			return;

		ShockwaveAttack();
		SendWeaponAnim(WAND_SPELL3);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/wand3.wav", 1, ATTN_NORM);
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] -= 30;
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + 1.0;
	}
	break;
	case SPELL_CHUMTOAD:
	{
		if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 100)
			return;

		ChumtoadAttack();
		SendWeaponAnim(WAND_SPELL3);
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/wand4.wav", 1, ATTN_NORM);
		m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()] = 0;
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + 2.0;
	}
	break;
	}

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);

	return;
}

void CHgun::SparkleAttack()
{
	Vector anglesAim = m_pPlayer->pev->v_angle;
	UTIL_MakeVectors(anglesAim);
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_up * -5 + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16;
	Vector vecDir = gpGlobals->v_forward;
#ifndef CLIENT_DLL
	UTIL_ScreenShake(m_pPlayer->pev->origin, 10.0, 1.0, 0.1, 1);
	CSparkle *pBolt = CSparkle::BoltCreate(0);
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();
	pBolt->pev->velocity = vecDir * 1400;
	pBolt->pev->speed = 600;
#endif
	return;
}

void CHgun::BarrageAttack()
{
	Vector anglesAim = m_pPlayer->pev->v_angle;
	UTIL_MakeVectors(anglesAim);
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 20;
	Vector vecDir = gpGlobals->v_forward - gpGlobals->v_right * 0.45;
	int speed = 250;
#ifndef CLIENT_DLL
	UTIL_ScreenShake(m_pPlayer->pev->origin, 10.0, 1.0, 0.1, 1);
	for (int i = 0; i < 8; i++)
	{
		CSparkle *pBolt = CSparkle::BoltCreate(1);
		pBolt->pev->origin = vecSrc;
		vecSrc = vecSrc + gpGlobals->v_right * 1.5 + gpGlobals->v_up * RANDOM_LONG(-10,10);
		pBolt->pev->angles = anglesAim;
		pBolt->pev->owner = m_pPlayer->edict();
		pBolt->pev->velocity = vecDir * (600 + RANDOM_LONG(-50,50));
		vecDir = vecDir + gpGlobals->v_right * 0.12;
		pBolt->pev->speed = speed;
		speed = speed*0.95;
		pBolt->pev->gravity = 0.1;
	}
#endif
	return;
}

void CHgun::ShockwaveAttack()
{
	TraceResult tr;

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors(anglesAim);
	anglesAim.x = -anglesAim.x;
	Vector vecSrc = m_pPlayer->GetGunPosition() - gpGlobals->v_up * 20;
	Vector vecDir[3];
	vecDir[0] = gpGlobals->v_forward - gpGlobals->v_right * 0.3;
	vecDir[1] = gpGlobals->v_forward;
	vecDir[2] = gpGlobals->v_forward + gpGlobals->v_right * 0.3;
#ifndef CLIENT_DLL
	CWave *pBolt[3];
	for (int i = 0; i<3; i++)
	{
		pBolt[i] = CWave::BoltCreate(0);
		pBolt[i]->pev->origin = vecSrc;
		pBolt[i]->pev->gravity = 0.1;
		pBolt[i]->pev->friction = 0.1;
		pBolt[i]->pev->angles = anglesAim;
		pBolt[i]->pev->owner = m_pPlayer->edict();
		pBolt[i]->pev->velocity = vecDir[i] * 650;
		pBolt[i]->pev->speed = 650;
	}
#endif
	return;
}

void CHgun::ChumtoadAttack()
{
	Vector anglesAim = m_pPlayer->pev->v_angle;
	UTIL_MakeVectors(anglesAim);
	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_up * -5 + gpGlobals->v_right * 2 + gpGlobals->v_forward * 16;
	Vector vecDir = gpGlobals->v_forward;
#ifndef CLIENT_DLL
	UTIL_ScreenShake(m_pPlayer->pev->origin, 10.0, 1.0, 0.1, 1);
	CSparkle *pBolt = CSparkle::BoltCreate(2);
	pBolt->pev->origin = vecSrc;
	pBolt->pev->angles = anglesAim;
	pBolt->pev->owner = m_pPlayer->edict();
	pBolt->pev->velocity = vecDir * 450;
	pBolt->pev->speed = 450;
	pBolt->pev->gravity = 0.1;
#endif
	return;
}

void CHgun::SecondaryAttack( void )
{
	Reload();

	if (m_pPlayer->wandspelltype >= 4)
		m_pPlayer->wandspelltype = 0;
	else
		m_pPlayer->wandspelltype++;

	switch (m_pPlayer->wandspelltype)
	{
	case SPELL_SPARKLE: UTIL_CenterPrintAll("Spell: Sparkle (Mana cost: 5)"); break;
	case SPELL_BARRAGE: UTIL_CenterPrintAll("Spell: Mini Barrage (Mana cost: 20)"); break;
	case SPELL_SHOCKWAVE: UTIL_CenterPrintAll("Spell: Pushing Shockwave (Mana cost: 30)"); break;
	case SPELL_CHUMTOAD: UTIL_CenterPrintAll("Spell: Turn to Rat (Mana cost: 100)"); break;
	}

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
}

void CHgun::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= HORNET_MAX_CARRY )
		return;

	while( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < HORNET_MAX_CARRY && m_flRechargeTime < gpGlobals->time )
	{
		/*float flRechargeTimePause = 0.5f;
#if CLIENT_DLL
		if( bIsMultiplayer() )
#else
		if( g_pGameRules->IsMultiplayer() )
#endif
			flRechargeTimePause = 0.3f;*/

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]++;
		m_flRechargeTime = gpGlobals->time + 0.15f;
	}
}

void CHgun::WeaponIdle( void )
{
	Reload();

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	/*float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0.0f, 1.0f );
	if( flRand <= 0.75f )
	{
		iAnim = HGUN_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0f / 16.0f * 2.0f;
	}
	else if( flRand <= 0.875f )
	{
		iAnim = HGUN_FIDGETSWAY;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 40.0f / 16.0f;
	}
	else
	{
		iAnim = HGUN_FIDGETSHAKE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 35.0f / 16.0f;
	}*/
	iAnim = WAND_IDLE1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0f / 16.0f * 2.0f;
	SendWeaponAnim( iAnim );
}
#endif
