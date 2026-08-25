#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


LINK_ENTITY_TO_CLASS(weapon_money, CMoney);


enum crowbar_e {
	MONEY_IDLE = 0,
	MONEY_FIDGET,
	MONEY_HOLSTER,
	MONEY_DRAW
};


void CMoney::Spawn()
{
	Precache();
	m_iId = WEAPON_MONEY;
	SET_MODEL(ENT(pev), "models/w_money.mdl");
	m_iClip = 1;

	FallInit();// get ready to fall down.
	m_iDefaultAmmo = 1;
}


void CMoney::Precache(void)
{
	PRECACHE_MODEL("models/v_money.mdl");
	PRECACHE_MODEL("models/w_money.mdl");
	PRECACHE_MODEL("models/p_crowbar.mdl");
	PRECACHE_SOUND("weapons/cash.wav");

	m_usMoney = PRECACHE_EVENT(1, "events/money.sc");
}

int CMoney::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "cash";
	p->iMaxAmmo1 = 200;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 3;
	p->iId = WEAPON_MONEY;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}



BOOL CMoney::Deploy()
{
	return DefaultDeploy("models/v_money.mdl", "models/p_crowbar.mdl", MONEY_DRAW, "money");
}

void CMoney::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(MONEY_HOLSTER);
}



void CMoney::PrimaryAttack()
{
	TraceResult tr;

	Vector anglesAim = m_pPlayer->pev->v_angle;
	UTIL_MakeVectors(anglesAim);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = gpGlobals->v_forward;

	UTIL_TraceLine(vecSrc, vecSrc + vecDir * 100, dont_ignore_monsters, m_pPlayer->edict(), &tr);

	if (tr.pHit)
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if (pEntity)
		{
			CBaseMonster *pMonster = pEntity->MyMonsterPointer();
			if (pMonster && FBitSet(pMonster->pev->spawnflags, SF_MONSTER_VENDOR))
			{
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/cash.wav", 1.0, ATTN_NORM);
				m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= pMonster->GetPaid(m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]);
			}
		}
	}

	SendWeaponAnim(MONEY_DRAW);
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

class CCashStack : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_money.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_money.mdl");
		PRECACHE_SOUND("weapons/cash.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(150, "cash", 250) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/cash.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_cashstack, CCashStack);

class CCash : public CBasePlayerAmmo
{
	void Spawn(void)
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_money.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache(void)
	{
		PRECACHE_MODEL("models/w_money.mdl");
		PRECACHE_SOUND("weapons/cash.wav");
	}
	BOOL AddAmmo(CBaseEntity *pOther)
	{
		int bResult = (pOther->GiveAmmo(50, "cash", 250) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/cash.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_cash, CCash);

