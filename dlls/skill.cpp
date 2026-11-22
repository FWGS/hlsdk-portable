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
//=========================================================
// skill.cpp - code for skill level concerns
//=========================================================

#include "extdll.h"
#include "util.h"
#include "skill.h"

skilldata_t gSkillData;

float GetSkillCvar( const char* name )
{
	char buffer[64];
	sprintf( buffer, "%s%d", name, gSkillData.iSkillLevel );
	float value = CVAR_GET_FLOAT( buffer );
	if ( value <= 0 )
		ALERT( at_console, "\n\n** GetSkillCVar Got a zero for %s **\n\n", buffer );
	return value;
}
