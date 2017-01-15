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
/*

===== gearbox_subs.cpp ========================================================

frequently used global functions

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"

class CPitwormSteamLock : public CBaseEntity
{
public:

};

LINK_ENTITY_TO_CLASS(info_pitworm_steam_lock, CBaseEntity);