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
#include "basemonster.h"

#define SF_GARBAGE_RANDOMYAW	(1U << 3)

// garbage's bodygroups
#define	STRAINEDPAPER1		0
#define	STRAINEDPAPER2		1
#define STRAINEDPAPER3		2
#define	GLASS			4
#define	SANDWICH		5
#define	APPLE			6
#define	BANANA			7
#define	MAGAZINE		8
#define	BROWNPAPER		9
#define	BLUECAN			11

class CGarbage : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	int Classify();
};

LINK_ENTITY_TO_CLASS( monster_garbage, CGarbage )

int CGarbage::Classify()
{
	return CLASS_INSECT;
}

void CGarbage::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	m_bloodColor = DONT_BLEED;
	m_MonsterState = MONSTERSTATE_NONE;

	pev->health = 80000;	// !!!
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	pev->effects = 0;
	pev->sequence = 0;

	if( FBitSet( pev->spawnflags, SF_GARBAGE_RANDOMYAW ) )
	{
		pev->angles = Vector( 0, RANDOM_LONG( 0, 360 ), 0 );
	}
}

void CGarbage::Precache()
{
	int iRand;

	switch( pev->body )
	{
		default:
		case 0:
			iRand = RANDOM_LONG( 0, 8 );
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			switch( iRand )
			{
				case 1:
					pev->body = STRAINEDPAPER1;
					break;
				case 2:
				case 3:
					pev->body = STRAINEDPAPER2;
					break;
				case 4:
					pev->body = GLASS;
					break;
				case 5:
				case 6:
				case 7:
					pev->body = iRand + 1;
					break;
				default:
					pev->body = BROWNPAPER;
					break;
			}
			break;
		case 1:
			pev->body = RANDOM_LONG( 0, 2 );
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			break;
		case 2:
			iRand = RANDOM_LONG( 4, 8 );
			if( iRand == 8 )
			{
				if( RANDOM_LONG( 0, 1 ) )
				{
					pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
					pev->body = 11;
				}
				else
				{
					pev->model = MAKE_STRING( "models/can.mdl" );
					pev->body = 0;
					pev->skin = RANDOM_LONG( 0, 5 );
				}
			}
			else
			{
				pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
				pev->body = iRand;
			}
			break;
		case 3:
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			pev->body = RANDOM_LONG( 8, 9 );
			break;
		case 4:
			iRand = RANDOM_LONG( 0, 6 );
			if( iRand == 6 )
			{
				pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
				pev->body = BLUECAN;
			}
			else
			{
				pev->model = MAKE_STRING( "models/can.mdl" );
				pev->body = 0;
				pev->skin = iRand;
			}
			break;
		case 5:
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			pev->body = STRAINEDPAPER1;
			break;
		case 6: 
		case 7:
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			pev->body = STRAINEDPAPER2;
			break;
		case 8:
		case 9:
		case 10:
		case 11:
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			pev->body -= 4;
			break;
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
			pev->model = MAKE_STRING( "models/can.mdl" );
			pev->skin = pev->body - 12;
			pev->body = 0;
			break;
		case 18:
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			pev->body = BLUECAN;
			break;
		case 19:
		case 20:
			pev->model = MAKE_STRING( "models/garbagegibs.mdl" );
			pev->body -= 11;
			break;
	}
	PRECACHE_MODEL( STRING( pev->model ) );
}
