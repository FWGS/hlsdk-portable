/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// monsterstate.cpp - base class monster functions for 
// controlling core AI.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "nodes.h"
#include "monsters.h"
#include "animation.h"
#include "saverestore.h"
#include "soundent.h"

//=========================================================
// SetState
//=========================================================
void CBaseMonster::SetState( MONSTERSTATE State )
{
/*
	if( State != m_MonsterState )
	{
		ALERT( at_aiconsole, "State Changed to %d\n", State );
	}
*/
	switch( State )
	{

	// Drop enemy pointers when going to idle
	case MONSTERSTATE_IDLE:
		if( m_hEnemy != NULL )
		{
			m_hEnemy = NULL;// not allowed to have an enemy anymore.
			ALERT( at_aiconsole, "Stripped\n" );
		}
		break;
	default:
		break;
	}

	m_MonsterState = State;
	m_IdealMonsterState = State;
}
