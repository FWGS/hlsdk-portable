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
	void	Spawn(void);
	void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

private:
	void	PlayMP3(CBaseEntity* pClient, const char* song);
};

LINK_ENTITY_TO_CLASS(trigger_command, CTriggerCommand);

void CTriggerCommand::Spawn(void)
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
}

void CTriggerCommand::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!pActivator || !pActivator->IsNetClient())
		return;

	const char* command = STRING(pev->netname);

	if (!command || !*command)
		return;

	// ALERT(at_console, "%s with command \"%s\"\n", STRING(pev->classname), command);

	char* str = NULL;

	if ((str = (char*)strstr(command, "playmp3")) != NULL)
	{
		int pchlen = 0;
		int extlen = 3; // "mp3" excluding NULL terminator.
		int ideallen = 0;

		char* pch = NULL, *lastpch = NULL;
		char* song = NULL;

		pch = strtok(str, " .");

		while (pch)
		{
			pchlen = strlen(pch);
			ideallen = (pchlen <= extlen) ? pchlen : extlen;

			if (strncmp(pch, "mp3", sizeof(char) * ideallen) == 0)
			{
				pch = NULL;
			}
			else
			{
				lastpch = pch;
				pch = strtok(NULL, " .");
			}
		}

		song = lastpch;

		PlayMP3(pActivator, song);
	}

	UTIL_Remove(this);
}

void CTriggerCommand::PlayMP3(CBaseEntity* pClient, const char* song)
{
	ASSERT(pClient != NULL);

	char cmd[128];
	sprintf(cmd, "play sound/mp3/%s.mp3\n", song);

	CLIENT_COMMAND(ENT(pClient->pev),cmd);
}
