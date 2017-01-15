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
// friendly grunt - torch
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"animation.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"effects.h"
#include	"customentity.h"
#include	"fgrunt.h"

#if 0
class CHTorch : public CFGrunt
{
public:

	void Spawn(void);
	void Precache(void);


	int		Save(CSave &save);
	int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	int		m_iTemp;
};

LINK_ENTITY_TO_CLASS(monster_human_torch_ally, CHTorch);

TYPEDESCRIPTION	CHTorch::m_SaveData[] =
{
	DEFINE_FIELD(CHTorch, m_iTemp, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CHTorch, CFGrunt);
#endif