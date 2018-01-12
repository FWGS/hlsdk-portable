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

#define SF_BODYPART_RANDOMYAW	(1U << 3)

class CBodyPart : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	int Classify();
};

LINK_ENTITY_TO_CLASS( monster_bodypart, CBodyPart )

int CBodyPart::Classify()
{
	return CLASS_INSECT;
}

void CBodyPart::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), STRING( pev->model ) );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

	m_bloodColor = BLOOD_COLOR_RED;
	m_MonsterState = MONSTERSTATE_NONE;

	pev->health = 80000;	// !!!
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	pev->effects = 0;
	pev->sequence = 0;

	if( FBitSet( pev->spawnflags, SF_BODYPART_RANDOMYAW ) )
	{
		pev->angles = Vector( 0, RANDOM_LONG( 0, 360 ), 0 );
	}
}

void CBodyPart::Precache()
{
	int iRand;

	switch( pev->body )
	{
		default:
		case 0:
			iRand = RANDOM_LONG( 0, 8 );
			if( iRand == 8 )
			{
				pev->model = MAKE_STRING( "models/gib_hgrunt.mdl" );
				pev->body = 0;
			}
			else
			{
				pev->model = MAKE_STRING( "models/hgibs.mdl" );
				pev->body = ( iRand == 7 ) ? RANDOM_LONG( 7, 10 ) : iRand;
			}
			break;
		case 1:
			pev->model = MAKE_STRING( "models/hgibs.mdl" );
			iRand = RANDOM_LONG( 0, 7 );
			pev->body = ( iRand == 7 ) ? RANDOM_LONG( 7, 10 ) : iRand;
			break;
		case 2:
			pev->model = MAKE_STRING( "models/gib_hgrunt.mdl" );
			pev->body = 0;
			break;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
			pev->model = MAKE_STRING( "models/hgibs.mdl" );
			pev->body -= 3;
			break;
	}
	PRECACHE_MODEL( STRING( pev->model ) );
}
