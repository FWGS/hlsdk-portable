/***
*
*	Copyright (c) 2001, Valve LLC. All rights reserved.
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
/*

===== ctf_items.cpp ========================================================

This contains the Flag entity information for the Half-Life : Opposing force CTF Gamemode.

*/
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"skill.h"
#include	"game.h"
#include	"items.h"
#include	"ctf_gamerules.h"
#include	"ctf_powerups.h"


extern int gmsgRuneStatus;
extern bool g_bSpawnedRunes;
edict_t *RuneSelectSpawnPoint(void);

static const char* g_pszRuneClassName[] =
{
	IT_CTF_ACCELERATOR_CLASSNAME,
	IT_CTF_BACKPACK_CLASSNAME,
	IT_CTF_LONGJUMP_CLASSNAME,
	IT_CTF_PORTABLEHEV_CLASSNAME,
	IT_CTF_REGEN_CLASSNAME,
};

const char* GetRuneClass(int index)
{
	return g_pszRuneClassName[index];
}

LINK_ENTITY_TO_CLASS(info_ctfspawn_powerup, CPowerupCTFBase);

LINK_ENTITY_TO_CLASS(item_ctfaccelerator, CPowerupAccelerator);
LINK_ENTITY_TO_CLASS(item_ctfbackpack, CPowerupBackpack);
LINK_ENTITY_TO_CLASS(item_ctflongjump, CPowerupJumppack);
LINK_ENTITY_TO_CLASS(item_ctfportablehev, CPowerupPorthev);
LINK_ENTITY_TO_CLASS(item_ctfregeneration, CPowerupRegen);

/***************************************
****************************************
	RUNES
****************************************
***************************************/

void CPowerupCTFBase::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "team_no"))
	{
		m_teamNo = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue(pkvd);
}


static const char* SelectRuneRandom( int* pRuneFlag )
{
	switch (RANDOM_LONG(0, ARRAYSIZE(g_pszRuneClassName) - 1))
	{
	case 0:  *pRuneFlag = IT_CTF_ACCELERATOR_FLAG;  return "models/w_accelerator.mdl";
	case 1:  *pRuneFlag = IT_CTF_BACKPACK_FLAG;		return "models/w_backpack.mdl";
	case 2:  *pRuneFlag = IT_CTF_REGEN_FLAG;		return "models/w_health.mdl";
	case 3:	 *pRuneFlag = IT_CTF_LONGJUMP_FLAG;		return "models/w_jumppack.mdl";
	case 4:  *pRuneFlag = IT_CTF_PORTABLEHEV_FLAG;	return "models/w_porthev.mdl";
	default: *pRuneFlag = -1;						return NULL;
	}
}

void CPowerupCTFBase::Spawn(void)
{
	if (GetRuneFlag() < 0)
	{
		int flag;
		char* modelname = (char*)SelectRuneRandom(&flag);

		if (modelname != NULL && flag > 0)
		{
			char* printname = NULL;

			if (flag & IT_CTF_ACCELERATOR_FLAG)
				printname = "Accelerator";
			else if (flag & IT_CTF_BACKPACK_FLAG)
				printname = "Backpack";
			else if (flag & IT_CTF_LONGJUMP_FLAG)
				printname = "Long jump";
			else if (flag & IT_CTF_PORTABLEHEV_FLAG)
				printname = "Portable HEV";
			else 
				printname = "Regeneration";

			m_iszPrintName = ALLOC_STRING(printname);
		}


		SET_MODEL(ENT(pev), (char*)SelectRuneRandom(&m_iRuneFlag) );
	}
	else
	{
		SET_MODEL(ENT(pev), (char*)GetRuneModel());
		m_iRuneFlag = GetRuneFlag();
	}

	if (m_iRuneFlag < 0 || !STRING(pev->model))
	{
		ALERT(at_error, "Could not properly setup rune entity. Removing...\n");
		UTIL_Remove(this);
	}

#if 1
	ALERT(at_console, "Spawning rune %d with model %s.\n", m_iRuneFlag, STRING(pev->model));
#endif

	m_iszPrintName = MAKE_STRING("Unknown");

	m_bTouchable = FALSE;

	dropped = false;

	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;

	vec3_t forward, right, up;

	UTIL_SetSize(pev, Vector(-15, -15, -15), Vector(15, 15, 15));

	pev->angles.z = pev->angles.x = 0;
	pev->angles.y = RANDOM_LONG(0, 360);

	//If we got an owner, it means we are either dropping the flag or diying and letting it go.
	if (pev->owner)
		g_engfuncs.pfnAngleVectors(pev->owner->v.angles, forward, right, up);
	else
		g_engfuncs.pfnAngleVectors(pev->angles, forward, right, up);

	UTIL_SetOrigin(pev, pev->origin);

	pev->velocity = (forward * 400) + (up * 200);

	if (pev->owner == NULL)
	{
		pev->origin.z += 16;
		pev->velocity.z = 300;
	}

	pev->owner = NULL;

	SetTouch(&CPowerupCTFBase::RuneTouch);

	pev->nextthink = gpGlobals->time + 1;
	SetThink(&CPowerupCTFBase::MakeTouchable);
}

void CPowerupCTFBase::RuneRespawn(void)
{
	edict_t *pentSpawnSpot;
	vec3_t vOrigin;

	pentSpawnSpot = RuneSelectSpawnPoint();
	vOrigin = VARS(pentSpawnSpot)->origin;

	UTIL_SetOrigin(pev, vOrigin);

	if (dropped)
		UTIL_LogPrintf("\"<-1><><>\" triggered triggered \"Respawn_ResistRune\"\n");

	Spawn();
}

void CPowerupCTFBase::MakeTouchable(void)
{
	m_bTouchable = TRUE;
	pev->nextthink = gpGlobals->time + 120; // if no one touches it in two minutes,
	// respawn it somewhere else, so inaccessible 
	// ones will come 'back'
	SetThink(&CPowerupCTFBase::RuneRespawn);
}



void CPowerupCTFBase::RuneTouch(CBaseEntity *pOther)
{
	//No toucher?
	if (!pOther)
		return;

	//Not a player?
	if (!pOther->IsPlayer())
		return;

	//DEAD?!
	if (pOther->pev->health <= 0)
		return;

	//Spectating?
	if (pOther->pev->movetype == MOVETYPE_NOCLIP)
		return;

	// If only a specific team can use this power up,
	// ensure toucher is allowed to.
	if (m_teamNo > 0)
	{
		// Not in the same team, return.
		if (pOther->pev->team != m_teamNo - 1)
			return;
	}
/*
	//Only one per customer
	if (pPlayer->m_iRuneStatus)
	{
		ClientPrint(pOther->pev, HUD_PRINTCENTER, "You already have a rune!\n");
		return;
	}
*/
	if (!m_bTouchable)
		return;

	//pPlayer->m_iRuneStatus = m_iRuneFlag; //Add me the rune flag

	ClientPrint(pOther->pev, HUD_PRINTCENTER, "You got the rune of %s!\n", STRING(m_iszPrintName));

#if 0
	UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Found_Rune\"\n",
		STRING(pOther->pev->netname),
		GETPLAYERUSERID(pOther->edict()),
		GETPLAYERAUTHID(pOther->edict()),
		pPlayer->m_szTeamName);
#endif

	if (RANDOM_LONG(0, 1) == 1)
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/ammopickup1.wav", 1, ATTN_NORM);
	else
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/ammopickup2.wav", 1, ATTN_NORM);

	//Update my client side rune hud thingy.
/*	MESSAGE_BEGIN(MSG_ONE, gmsgRuneStatus, NULL, pOther->pev);
	WRITE_BYTE(pPlayer->m_iRuneStatus);
	MESSAGE_END();
*/
	//And Remove this entity
	UTIL_Remove(this);
}



/***************************************
****************************************
RUNES
****************************************
***************************************/

/*----------------------------------------------------------------------
The Rune Game modes

Rune 1 - Earth Magic
resistance
Rune 2 - Black Magic
strength
Rune 3 - Hell Magic
haste
Rune 4 - Elder Magic
regeneration

----------------------------------------------------------------------*/

BOOL IsRuneSpawnPointValid(CBaseEntity *pSpot)
{
#if 0
	CBaseEntity *ent = NULL;

	while ((ent = UTIL_FindEntityInSphere(ent, pSpot->pev->origin, 128)) != NULL)
	{
		//Try not to spawn it near other runes.
		if (!strcmp(STRING(ent->pev->classname), "item_rune1") ||
			!strcmp(STRING(ent->pev->classname), "item_rune2") ||
			!strcmp(STRING(ent->pev->classname), "item_rune3") ||
			!strcmp(STRING(ent->pev->classname), "item_rune4"))
			return FALSE;
	}
#endif

	return TRUE;
}

edict_t *RuneSelectSpawnPoint(void)
{
	CBaseEntity *pSpot;

	pSpot = NULL;

	// Randomize the start spot
	for (int i = RANDOM_LONG(1, 5); i > 0; i--)
		pSpot = UTIL_FindEntityByClassname(pSpot, CTF_POWERUP_SPAWN_ENTITY_CLASSNAME);
	if (!pSpot)  // skip over the null point
		pSpot = UTIL_FindEntityByClassname(pSpot, CTF_POWERUP_SPAWN_ENTITY_CLASSNAME);

	CBaseEntity *pFirstSpot = pSpot;

	do
	{
		if (pSpot)
		{
			if (IsRuneSpawnPointValid(pSpot))
			{
				if (pSpot->pev->origin == Vector(0, 0, 0))
				{
					pSpot = UTIL_FindEntityByClassname(pSpot, CTF_POWERUP_SPAWN_ENTITY_CLASSNAME);
					continue;
				}
				// if so, go to pSpot
				goto ReturnSpot;
			}

		}
		// increment pSpot
		pSpot = UTIL_FindEntityByClassname(pSpot, CTF_POWERUP_SPAWN_ENTITY_CLASSNAME);
	} while (pSpot != pFirstSpot); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if (pSpot)
		goto ReturnSpot;

ReturnSpot:
	if (!pSpot)
	{
		ALERT(at_error, "RuneSelectSpawnPoint: no %s on level\n", CTF_POWERUP_SPAWN_ENTITY_CLASSNAME);
		return INDEXENT(0);
	}
	return pSpot->edict();
}

void VectorScale(const float *in, float scale, float *out)
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void G_ProjectSource(vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result)
{
	result[0] = point[0] + forward[0] * distance[0] + right[0] * distance[1];
	result[1] = point[1] + forward[1] * distance[0] + right[1] * distance[1];
	result[2] = point[2] + forward[2] * distance[0] + right[2] * distance[1] + distance[2];
}

#define VectorSet(v, x, y, z)	(v[0]=(x), v[1]=(y), v[2]=(z))

void DropRune(CBasePlayer *pPlayer)
{
	TraceResult tr;

	// do they even have a rune?
	if (pPlayer->m_iRuneStatus == 0)
		return;

	// Make Sure there's enough room to drop the rune here
	// This is so hacky ( the reason why we are doing this), and I hate it to death.
	UTIL_MakeVectors(pPlayer->pev->v_angle);
	Vector vecSrc = pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;
	UTIL_TraceHull(vecSrc, vecEnd, dont_ignore_monsters, human_hull, ENT(pPlayer->pev), &tr);

	if (tr.flFraction != 1)
	{
		ClientPrint(pPlayer->pev, HUD_PRINTCENTER, "Not enough room to drop the rune here.");
		return;
	}

	CBaseEntity *pRune = NULL;
	char * runeName;

	if (pPlayer->m_iRuneStatus == IT_CTF_ACCELERATOR_FLAG)
	{
		pRune = CBaseEntity::Create(IT_CTF_ACCELERATOR_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());
		runeName = "AcceleratorRune";

		if (pRune)
			((CPowerupAccelerator*)pRune)->dropped = true;
	}
	else if (pPlayer->m_iRuneStatus == IT_CTF_BACKPACK_FLAG)
	{
		pRune = CBaseEntity::Create(IT_CTF_BACKPACK_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());
		runeName = "BackpackRune";

		if (pRune)
			((CPowerupBackpack*)pRune)->dropped = true;
	}
	else if (pPlayer->m_iRuneStatus == IT_CTF_REGEN_FLAG)
	{
		pRune = CBaseEntity::Create(IT_CTF_REGEN_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());
		runeName = "RegenRune";

		if (pRune)
			((CPowerupRegen*)pRune)->dropped = true;
	}
	else if (pPlayer->m_iRuneStatus == IT_CTF_LONGJUMP_FLAG)
	{
		pRune = CBaseEntity::Create(IT_CTF_LONGJUMP_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());
		runeName = "JumppackRune";

		if (pRune)
			((CPowerupJumppack*)pRune)->dropped = true;
	}
	else if (pPlayer->m_iRuneStatus == IT_CTF_PORTABLEHEV_FLAG)
	{
		pRune = CBaseEntity::Create(IT_CTF_PORTABLEHEV_CLASSNAME, pPlayer->pev->origin, pPlayer->pev->angles, pPlayer->edict());
		runeName = "PorthevRune";

		if (pRune)
			((CPowerupPorthev*)pRune)->dropped = true;
	}
	else
	{
		runeName = "Unknown";
	}

	pPlayer->m_iRuneStatus = 0;

	UTIL_LogPrintf("\"%s<%i><%s><%s>\" triggered \"Dropped_%s\"\n",
		STRING(pPlayer->pev->netname),
		GETPLAYERUSERID(pPlayer->edict()),
		GETPLAYERAUTHID(pPlayer->edict()),
		pPlayer->m_szTeamName,
		runeName);

	MESSAGE_BEGIN(MSG_ONE, gmsgRuneStatus, NULL, pPlayer->pev);
	WRITE_BYTE(pPlayer->m_iRuneStatus);
	MESSAGE_END();
}


/*
================
SpawnRunes
spawn all the runes
self is the entity that was created for us, we remove it
================
*/
void SpawnRunes(void)
{
	if (g_bSpawnedRunes)
		return;

	edict_t *pentSpawnSpot = NULL;

	for (int i = 0; i < ARRAYSIZE(g_pszRuneClassName); i++)
	{
		pentSpawnSpot = RuneSelectSpawnPoint();
		CBaseEntity::Create((char*)g_pszRuneClassName[i], VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles, NULL);
	}

	g_bSpawnedRunes = TRUE;
}
