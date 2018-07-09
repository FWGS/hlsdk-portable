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
#include "monsters.h"
#include "customentity.h"
#include "effects.h"
#include "weapons.h"

class CTriggerCommand : public CPointEntity
{
public:
	void	Spawn();
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

LINK_ENTITY_TO_CLASS( trigger_command, CTriggerCommand )

void CTriggerCommand::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
}

void CTriggerCommand::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	char cmd[64];

	if( !pActivator || !pActivator->IsPlayer() )
		return;

	if( !pev->netname )
		return;

	sprintf( cmd, "%s\n", STRING( pev->netname ) );
	CLIENT_COMMAND( ENT( pActivator->pev ), cmd );

	UTIL_Remove( this );
}
