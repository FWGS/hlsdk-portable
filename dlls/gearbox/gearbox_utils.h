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

#ifndef GEARBOX_UTILS_H
#define GEARBOX_UTILS_H

extern const char*	UTIL_GetActivityNameBySequence(const int sequence);
extern const char*	UTIL_GetEntityActivityName(struct entvars_s* pev);
extern const char*	UTIL_GetEntityActivityName(struct edict_s* ent);
extern const char*	UTIL_GetEntityActivityName(CBaseEntity* pEntity);
extern void			UTIL_PrintActivity(struct entvars_s* pev, const ALERT_TYPE alert_type = at_console);
extern void			UTIL_PrintActivity(struct edict_s* ent,	const ALERT_TYPE alert_type = at_console);
extern void			UTIL_PrintActivity(CBaseEntity* pEntity, const ALERT_TYPE alert_type = at_console);

#endif // GEARBOX_UTILS_H