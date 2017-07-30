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
#include "nodes.h"
#include "soundent.h"
#include "decals.h"
#include "effects.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "shall_map_fixes.h"

#if defined ( SHALL_MAPFIXES )

#define RANDOM_ANGLE() Vector( 0, RANDOM_LONG(1, 6) * RANDOM_LONG(1, 6) * RANDOM_LONG(1, 10), 0 )

static CBaseEntity* CreateItemAtPosition(char* classname, const Vector& position, const Vector& angles)
{
	CBaseEntity* item = CBaseEntity::Create(classname, position, angles, NULL);
	if (item != NULL)
	{
		item->Precache();
		item->Spawn();
	}

	return item;
}

static CBaseEntity* CreateHealthKitAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("item_healthkit", position, angles);
}

static CBaseEntity* CreateBatteryAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("item_battery", position, angles);
}

static CBaseEntity* CreateWeaponCrowbarAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("weapon_crowbar", position, angles);
}

static CBaseEntity* CreateWeaponMP5AtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("weapon_9mmAR", position, angles);
}

static CBaseEntity* CreateWeaponShotgunAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("weapon_shotgun", position, angles);
}

static CBaseEntity* CreateWeapon357AtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("weapon_357", position, angles);
}

static CBaseEntity* CreateWeaponSatchelAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("weapon_satchel", position, angles);
}

static CBaseEntity* CreateWeaponTripmineAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("weapon_tripmine", position, angles);
}

static CBaseEntity* CreateMP5AmmoAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("ammo_9mmAR", position, angles);
}

static CBaseEntity* CreateMP5GrenadeAmmoAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("ammo_ARGrenades", position, angles);
}

static CBaseEntity* CreateShotgunAmmoAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("ammo_buckshot", position, angles);
}

static CBaseEntity* Create357AmmoAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("ammo_357", position, angles);
}

static CBaseEntity* Create9mmAmmoBoxAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("ammo_9mmbox", position, angles);
}

enum CrossbowPlacement
{
	Normal = 0,
	Realistic = 1,
};

static CBaseEntity* CreateWeaponCrossbowAtPosition(const Vector& position, const Vector& angles, CrossbowPlacement placement = CrossbowPlacement::Normal)
{
	CBaseEntity* crossbow = CreateItemAtPosition("weapon_crossbow", position, angles);

	if(crossbow != NULL)
		crossbow->pev->sequence = (int)placement;

	return crossbow;
}

static CBaseEntity* CreateCrossbowAmmoAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("ammo_crossbow", position, angles);
}

static CBaseEntity* Create9mmBoxAmmoAtPosition(const Vector& position, const Vector& angles)
{
	return CreateItemAtPosition("ammo_9mmbox", position, angles);
}

class MapFixPatchSpawnPosition
{
public:
	void ApplyFixToPlayer(CBasePlayer* player);
};

class MapFixPatchAddWeaponsAndAmmo
{
public:
	void ApplyFix();

protected:
	void SpawnWeaponsAndAmmoAtSpawn(float floorZ);
	void SpawnWeaponsAndAmmoAtBeginningOfSecondSection(float floorZ);
	void SpawnWeaponsAndAmmoAtBeginningOfThirdSection(float floorZ);
	void SpawnWeaponsAndAmmoAtBeginningOfFourthSection(float floorZ);
	void SpawnWeaponsAndAmmoAtBeginningOfFifthSection(float floorZ);
};

class MapFixCornAddWeaponsAndAmmo
{
public:
	void ApplyFix();
	void SpawnCrowbarAtSpawn(float floorZ);
	void SpawnHealthAndBattery(float floorZ);
};

class MapFixShipAddWeaponsAndAmmo
{
public:
	void ApplyFix();
	void SpawnHealthAndBatteryAtLevel1(float floorZ);
	void SpawnHealthNearExit(float floorZ);
};

class MapFixWoodsAddWeaponsAndAmmo
{
public:
	void ApplyFix();
	void SpawnHealth(float floorZ);
};

class CChangeLevelToGrave : public CBaseEntity
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn(void);
	int	ObjectCaps(void) { return CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	void EXPORT WaitAndAppendToMultiManager(void);
	void EXPORT TeleportPlayerToChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT TeleportThink(void);

protected:
	CBaseEntity* FindTriggerByTarget(const char* triggerTarget);
	CBaseEntity* FindMultiManagerByName(const char* nextmap);
	void AddThisEntityToMultiManagerOutputList();

private:
	CBaseEntity* m_multimanager;
};

LINK_ENTITY_TO_CLASS(shall_teleporttochangelevel, CChangeLevelToGrave);

TYPEDESCRIPTION CChangeLevelToGrave::m_SaveData[] =
{
	DEFINE_FIELD(CChangeLevelToGrave, m_multimanager, FIELD_CLASSPTR),
};
IMPLEMENT_SAVERESTORE(CChangeLevelToGrave, CBaseEntity);

class MapFixWitch
{
public:
	void ApplyFix();
	void SpawnHealthAndBatteryNearHouseEntry(float floorZ);
	void AppendMultiManagerEntryToTeleportPlayerToChangeLevelAfterLevelIsDone();
};

class MapFixGraveAddWeaponsAndAmmo
{
public:
	void ApplyFix();
	void SpawnHealthAndBatteryNextToCoffin(float floorZ);
};

class MapFixVamp
{
public:
	void ApplyFix();
	void CreateAndSetupTriggerToCallLift1();
	void CreateAndSetupTriggerToCallLift2();
	void CreateAndSetupTriggerToCallLift3();
	void SpawnHealthAtHighestTowerBeforeLongJump(float floorZ);
protected:
	CBaseEntity* CreateLiftTrigger(const Vector& origin, const Vector& angles, float liftHeight);
};

class MapFixHellAddWeaponsAndAmmo
{
public:
	void ApplyFix();
	void SpawnHealthAmmoWeaponsAtLightNearSpawn(float floorZ);
	void SpawnHealthAmmoWeaponsAtPositionBeforeFinalBattle(float floorZ);
};

class MapFixes
{
public:

	void ApplyMapFixPatch();
	void ApplyMapFixCorn();
	void ApplyMapFixShip();
	void ApplyMapFixWoods();
	void ApplyMapFixWitch();
	void ApplyMapFixGrave();
	void ApplyMapFixVamp();
	void ApplyMapFixHell();
	void FixupMapPatchPlayerSpawnPosition(CBasePlayer* player);

	MapFixPatchSpawnPosition* GetPatchSpawnPositionSingleton();
	MapFixPatchAddWeaponsAndAmmo* GetPatchAddWeaponsAndAmmoSingleton();
	MapFixCornAddWeaponsAndAmmo* GetCornAddWeaponsAndAmmoSingleton();
	MapFixShipAddWeaponsAndAmmo* GetShipAddWeaponsAndAmmoSingleton();
	MapFixWoodsAddWeaponsAndAmmo* GetWoodsAddWeaponsAndAmmoSingleton();
	MapFixWitch* GetWitchSingleton();
	MapFixGraveAddWeaponsAndAmmo* GetGraveAddWeaponsAndAmmoSingleton();
	MapFixVamp* GetVampSingleton();
	MapFixHellAddWeaponsAndAmmo* GetHellAddWeaponsAndAmmoSingleton();
};

void MapFixPatchSpawnPosition::ApplyFixToPlayer( CBasePlayer* player )
{
	player->pev->velocity = player->pev->avelocity = g_vecZero;
	player->pev->angles = player->pev->v_angle = Vector(0, 90, 0);

	UTIL_SetOrigin(player->pev, Vector(2910, -2332, -410));
}

void MapFixPatchAddWeaponsAndAmmo::ApplyFix()
{
	// spawn
	SpawnWeaponsAndAmmoAtSpawn(-444);

	// beginning of second section (pumpkin field #1).
	SpawnWeaponsAndAmmoAtBeginningOfSecondSection(-446);

	// beginning of third section (square bales with cabin and vampire).
	SpawnWeaponsAndAmmoAtBeginningOfThirdSection(2);

	// beginning of fourth section (pumpkin field #2).
	SpawnWeaponsAndAmmoAtBeginningOfFourthSection(514);

	// beginning of fifth section (big mommas).
	SpawnWeaponsAndAmmoAtBeginningOfFifthSection(1024);
}

void MapFixPatchAddWeaponsAndAmmo::SpawnWeaponsAndAmmoAtSpawn(float floorZ)
{
	CreateWeaponMP5AtPosition(Vector(2966, -1718, floorZ), RANDOM_ANGLE());
	CreateWeaponShotgunAtPosition(Vector(2903, -1738, floorZ), RANDOM_ANGLE());
	CreateMP5AmmoAtPosition(Vector(2936, -1705, floorZ), RANDOM_ANGLE());
	CreateShotgunAmmoAtPosition(Vector(3011, -1707, floorZ), RANDOM_ANGLE());
	CreateShotgunAmmoAtPosition(Vector(2977, -1744, floorZ), RANDOM_ANGLE());
	CreateMP5AmmoAtPosition(Vector(2944, -1760, floorZ), RANDOM_ANGLE());
}

void MapFixPatchAddWeaponsAndAmmo::SpawnWeaponsAndAmmoAtBeginningOfSecondSection(float floorZ)
{
	CreateHealthKitAtPosition(Vector(-2972, -2694, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-3012, -2722, floorZ), RANDOM_ANGLE());
	CreateMP5AmmoAtPosition(Vector(-2967, -2726, floorZ), RANDOM_ANGLE());
	CreateShotgunAmmoAtPosition(Vector(-2995, -2754, floorZ), RANDOM_ANGLE());
}

void MapFixPatchAddWeaponsAndAmmo::SpawnWeaponsAndAmmoAtBeginningOfThirdSection(float floorZ)
{
	CreateWeaponCrossbowAtPosition(Vector(917, -719, floorZ), RANDOM_ANGLE(), CrossbowPlacement::Realistic);
	Create9mmBoxAmmoAtPosition(Vector(946, -786, floorZ), RANDOM_ANGLE());
	CreateCrossbowAmmoAtPosition(Vector(888, -762, floorZ), RANDOM_ANGLE());
	CreateCrossbowAmmoAtPosition(Vector(915, -766, floorZ), RANDOM_ANGLE());
	CreateCrossbowAmmoAtPosition(Vector(903, -785, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(876, -794, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(854, -806, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(855, -840, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(899, -821, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(913, -844, floorZ), RANDOM_ANGLE());
}

void MapFixPatchAddWeaponsAndAmmo::SpawnWeaponsAndAmmoAtBeginningOfFourthSection(float floorZ)
{
	CreateWeapon357AtPosition(Vector(689, 2776, floorZ), RANDOM_ANGLE());
	CreateWeaponShotgunAtPosition(Vector(649, 2752, floorZ), RANDOM_ANGLE());
	Create357AmmoAtPosition(Vector(688, 2750, floorZ), RANDOM_ANGLE());
	CreateShotgunAmmoAtPosition(Vector(705, 2734, floorZ), RANDOM_ANGLE());
	CreateShotgunAmmoAtPosition(Vector(663, 2706, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(681, 2729, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(718, 2710, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(690, 2707, floorZ), RANDOM_ANGLE());
}

void MapFixPatchAddWeaponsAndAmmo::SpawnWeaponsAndAmmoAtBeginningOfFifthSection(float floorZ)
{
	CreateWeaponSatchelAtPosition(Vector(-2884, 551, floorZ), RANDOM_ANGLE());
	CreateWeaponSatchelAtPosition(Vector(-2846, 568, floorZ), RANDOM_ANGLE());
	CreateWeaponTripmineAtPosition(Vector(-2852, 536, floorZ), RANDOM_ANGLE());
	CreateWeaponTripmineAtPosition(Vector(-2830, 549, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-2893, 528, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-2838, 513, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-2859, 495, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-2867, 514, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-2881, 495, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-2902, 502, floorZ), RANDOM_ANGLE());
}

void MapFixCornAddWeaponsAndAmmo::ApplyFix()
{
	SpawnCrowbarAtSpawn(-430);
	SpawnHealthAndBattery(-430);
}

void MapFixCornAddWeaponsAndAmmo::SpawnCrowbarAtSpawn(float floorZ)
{
	CreateWeaponCrowbarAtPosition(Vector(3168, -2341, floorZ), RANDOM_ANGLE());
}

void MapFixCornAddWeaponsAndAmmo::SpawnHealthAndBattery(float floorZ)
{
	CreateBatteryAtPosition(Vector(2332, 2099, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(2307, 2126, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(2225, 2105, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2332, 2134, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2282, 2105, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2262, 2134, floorZ), RANDOM_ANGLE());

	CreateBatteryAtPosition(Vector(-205, 2359, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-204, 2331, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-172, 2358, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-173, 2334, floorZ), RANDOM_ANGLE());

	CreateBatteryAtPosition(Vector(-210, 1428, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-186, 1430, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-158, 1430, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-124, 1430, floorZ), RANDOM_ANGLE());
}

void MapFixShipAddWeaponsAndAmmo::ApplyFix()
{
	SpawnHealthAndBatteryAtLevel1(2080);
	SpawnHealthNearExit(2080);
}

void MapFixShipAddWeaponsAndAmmo::SpawnHealthAndBatteryAtLevel1(float floorZ)
{
	CreateBatteryAtPosition(Vector(-672, -159, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-672, -135, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-674, -208, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-672, -186, floorZ), RANDOM_ANGLE());

	CreateBatteryAtPosition(Vector(-205, 673, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(-179, 670, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-134, 672, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-106, 672, floorZ), RANDOM_ANGLE());

	CreateBatteryAtPosition(Vector(666, 109, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(669, 85, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(658, 188, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(653, 166, floorZ), RANDOM_ANGLE());
}

void MapFixShipAddWeaponsAndAmmo::SpawnHealthNearExit(float floorZ)
{
	CreateHealthKitAtPosition(Vector(140, 48, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(169, 53, floorZ), RANDOM_ANGLE());
}

void MapFixWoodsAddWeaponsAndAmmo::ApplyFix()
{
	SpawnHealth(-433);
}

void CChangeLevelToGrave::Spawn(void)
{
	m_multimanager = NULL;

	SetThink(&CChangeLevelToGrave::WaitAndAppendToMultiManager);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CChangeLevelToGrave::WaitAndAppendToMultiManager(void)
{
	if (m_multimanager == NULL)
		m_multimanager = FindMultiManagerByName("explm");

	if (m_multimanager == NULL)
		pev->nextthink = gpGlobals->time + 1.0;
	else
	{
		AddThisEntityToMultiManagerOutputList();
		SetUse(&CChangeLevelToGrave::TeleportPlayerToChangeLevel);
		SetThink(&CBaseEntity::SUB_DoNothing);
		pev->nextthink = gpGlobals->time + 0.2f;
	}
}

void CChangeLevelToGrave::TeleportPlayerToChangeLevel(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	SetThink(&CChangeLevelToGrave::TeleportThink);
	pev->nextthink = gpGlobals->time;
}

void CChangeLevelToGrave::TeleportThink(void)
{
	CBasePlayer* player = dynamic_cast<CBasePlayer*>(UTIL_FindEntityByClassname(NULL, "player"));
	if (player != NULL)
	{
		CBaseEntity* teleportDestination = UTIL_FindEntityByTargetname(NULL, "mx2");
		if (teleportDestination != NULL)
		{
			// Constantly teleport the player to the changelevel
			// volume in case the event was missed.
			player->pev->velocity = player->pev->avelocity = g_vecZero;
			UTIL_SetOrigin(player->pev, teleportDestination->pev->origin);
		}
	}

	pev->nextthink = gpGlobals->time + 2.0f;
}

CBaseEntity* CChangeLevelToGrave::FindTriggerByTarget(const char* triggerTarget)
{
	return UTIL_FindEntityByString(NULL, "target", triggerTarget);
}

CBaseEntity* CChangeLevelToGrave::FindMultiManagerByName(const char* targetname)
{
	return UTIL_FindEntityByTargetname(NULL, targetname);
}

void CChangeLevelToGrave::AddThisEntityToMultiManagerOutputList()
{
	// Setup targetname to allow this entity's use method to be called.
	pev->targetname = ALLOC_STRING("witchtograve_teleport");

	KeyValueData kvd;

	// Tell the multimanager to call this entity in 10 seconds.
	kvd.szKeyName = (char*)STRING(pev->targetname);
	kvd.szValue = "10";

	// Append new entry to multimanager.
	m_multimanager->KeyValue(&kvd);
}

void MapFixWitch::ApplyFix()
{
	AppendMultiManagerEntryToTeleportPlayerToChangeLevelAfterLevelIsDone();
	SpawnHealthAndBatteryNearHouseEntry(-446);
}

void MapFixWitch::SpawnHealthAndBatteryNearHouseEntry(float floorZ)
{
	CreateBatteryAtPosition(Vector(1440, -240, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(1465, -239, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(1493, -238, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(1521, -237, floorZ), RANDOM_ANGLE());
}

void MapFixWitch::AppendMultiManagerEntryToTeleportPlayerToChangeLevelAfterLevelIsDone()
{
	CBaseEntity* changeLevelTeleporter = CBaseEntity::Create("shall_teleporttochangelevel", g_vecZero, g_vecZero, NULL);
	if (changeLevelTeleporter != NULL)
	{
		changeLevelTeleporter->Spawn();
	}
}

void MapFixGraveAddWeaponsAndAmmo::ApplyFix()
{
	SpawnHealthAndBatteryNextToCoffin(-446);
}

void MapFixGraveAddWeaponsAndAmmo::SpawnHealthAndBatteryNextToCoffin(float floorZ)
{
	CreateBatteryAtPosition(Vector(2080, -1602, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(2085, -1561, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2079, -1679, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2082, -1639, floorZ), RANDOM_ANGLE());
}

void MapFixVamp::ApplyFix()
{
	SpawnHealthAtHighestTowerBeforeLongJump(2062);
	CreateAndSetupTriggerToCallLift1();
	CreateAndSetupTriggerToCallLift2();
	CreateAndSetupTriggerToCallLift3();
}

#define LIFT_HEIGHT 562

void MapFixVamp::CreateAndSetupTriggerToCallLift1()
{
	CBaseToggle* trigger = dynamic_cast<CBaseToggle*>(CreateLiftTrigger(Vector(-384, 0, -312), g_vecZero, LIFT_HEIGHT));

	if (trigger)
	{
		trigger->m_flWait = 1.0f;
		trigger->pev->target = ALLOC_STRING("lift1");
	}
}

void MapFixVamp::CreateAndSetupTriggerToCallLift2()
{
	CBaseToggle* trigger = dynamic_cast<CBaseToggle*>(CreateLiftTrigger(Vector(1408, 1024, 255), g_vecZero, LIFT_HEIGHT));

	if (trigger)
	{
		trigger->m_flWait = 1.0f;
		trigger->pev->target = ALLOC_STRING("lift2");
	}
}

void MapFixVamp::CreateAndSetupTriggerToCallLift3()
{
	CBaseToggle* trigger = dynamic_cast<CBaseToggle*>(CreateLiftTrigger(Vector(-1408, 1792, 200), g_vecZero, LIFT_HEIGHT));

	if (trigger)
	{
		trigger->m_flWait = 1.0f;
		trigger->pev->target = ALLOC_STRING("lift3");
	}
}

void MapFixVamp::SpawnHealthAtHighestTowerBeforeLongJump(float floorZ)
{
	CreateHealthKitAtPosition(Vector(-116, 2448, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-148, 2440, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-116, 2420, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-148, 2420, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-119, 2391, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-149, 2392, floorZ), RANDOM_ANGLE());
}

CBaseEntity* MapFixVamp::CreateLiftTrigger(const Vector& origin, const Vector& angles, float liftHeight)
{
	edict_t* pevCreate = CREATE_NAMED_ENTITY(MAKE_STRING("trigger_multiple"));
	if (pevCreate == NULL)
	{
		ALERT(at_console, "** Could not create trigger_multiple for lift!**\n");
		return NULL;
	}

	CBaseToggle* trigger = dynamic_cast<CBaseToggle*>(CBaseEntity::Instance(pevCreate));

	if (trigger == NULL)
	{
		ALERT(at_console, "** Could not retrieve trigger_multiple instance!**\n");
		return NULL;
	}

	trigger->pev->angles = angles;
	DispatchSpawn(trigger->edict());

	UTIL_SetOrigin(trigger->pev, origin);
	UTIL_SetSize(trigger->pev, Vector(-64, -64, 0), Vector( 64, 64, liftHeight ) );

	return trigger;
}

void MapFixWoodsAddWeaponsAndAmmo::SpawnHealth(float floorZ)
{
	CreateWeaponCrowbarAtPosition(Vector(-2515, -1190, floorZ), RANDOM_ANGLE());

	CreateHealthKitAtPosition(Vector(-2809, 2575, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(-2845, 2578, floorZ), RANDOM_ANGLE());

	CreateHealthKitAtPosition(Vector(1469, 2862, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(1452, 2885, floorZ), RANDOM_ANGLE());

	CreateHealthKitAtPosition(Vector(-2829, -4, floorZ), RANDOM_ANGLE());

	CreateHealthKitAtPosition(Vector(1971, -2564, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(1966, -2539, floorZ), RANDOM_ANGLE());
}

void MapFixHellAddWeaponsAndAmmo::ApplyFix()
{
	SpawnHealthAmmoWeaponsAtLightNearSpawn(-2558);
	SpawnHealthAmmoWeaponsAtPositionBeforeFinalBattle(-2524);
}

void MapFixHellAddWeaponsAndAmmo::SpawnHealthAmmoWeaponsAtLightNearSpawn(float floorZ)
{
	CreateWeaponMP5AtPosition(Vector(1397, 1263, floorZ), RANDOM_ANGLE());
	CreateWeaponShotgunAtPosition(Vector(1405, 1209, floorZ), RANDOM_ANGLE());
	CreateWeaponCrossbowAtPosition(Vector(1323, 1271, floorZ), RANDOM_ANGLE(), CrossbowPlacement::Realistic);
	CreateWeaponCrowbarAtPosition(Vector(1345, 1237, floorZ), RANDOM_ANGLE());
	CreateMP5AmmoAtPosition(Vector(1418, 1260, floorZ), RANDOM_ANGLE());
	CreateShotgunAmmoAtPosition(Vector(1412, 1301, floorZ), RANDOM_ANGLE());
	CreateCrossbowAmmoAtPosition(Vector(1337, 1298, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(1345, 1205, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(1343, 1184, floorZ), RANDOM_ANGLE());
}

void MapFixHellAddWeaponsAndAmmo::SpawnHealthAmmoWeaponsAtPositionBeforeFinalBattle(float floorZ)
{
	CreateWeapon357AtPosition(Vector(2469, -1492, floorZ), RANDOM_ANGLE());
	Create357AmmoAtPosition(Vector(2494, -1495, floorZ), RANDOM_ANGLE());
	CreateMP5GrenadeAmmoAtPosition(Vector(2436, -1470, floorZ), RANDOM_ANGLE());
	CreateMP5GrenadeAmmoAtPosition(Vector(2457, -1473, floorZ), RANDOM_ANGLE());
	Create9mmAmmoBoxAtPosition(Vector(2495, -1445, floorZ), RANDOM_ANGLE());

	CreateHealthKitAtPosition(Vector(2398, -1451, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2447, -1440, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2405, -1399, floorZ), RANDOM_ANGLE());
	CreateHealthKitAtPosition(Vector(2443, -1388, floorZ), RANDOM_ANGLE());

	CreateBatteryAtPosition(Vector(2420, -1434, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(2470, -1411, floorZ), RANDOM_ANGLE());
	CreateBatteryAtPosition(Vector(2479, -1470, floorZ), RANDOM_ANGLE());
}

MapFixPatchSpawnPosition* MapFixes::GetPatchSpawnPositionSingleton()
{
	static MapFixPatchSpawnPosition singleton;
	return &singleton;
}

MapFixPatchAddWeaponsAndAmmo* MapFixes::GetPatchAddWeaponsAndAmmoSingleton()
{
	static MapFixPatchAddWeaponsAndAmmo singleton;
	return &singleton;
}

MapFixCornAddWeaponsAndAmmo* MapFixes::GetCornAddWeaponsAndAmmoSingleton()
{
	static MapFixCornAddWeaponsAndAmmo singleton;
	return &singleton;
}

MapFixShipAddWeaponsAndAmmo* MapFixes::GetShipAddWeaponsAndAmmoSingleton()
{
	static MapFixShipAddWeaponsAndAmmo singleton;
	return &singleton;
}

MapFixWoodsAddWeaponsAndAmmo* MapFixes::GetWoodsAddWeaponsAndAmmoSingleton()
{
	static MapFixWoodsAddWeaponsAndAmmo singleton;
	return &singleton;
}

MapFixWitch* MapFixes::GetWitchSingleton()
{
	static MapFixWitch singleton;
	return &singleton;
}

MapFixGraveAddWeaponsAndAmmo* MapFixes::GetGraveAddWeaponsAndAmmoSingleton()
{
	static MapFixGraveAddWeaponsAndAmmo singleton;
	return &singleton;
}

MapFixVamp* MapFixes::GetVampSingleton()
{
	static MapFixVamp singleton;
	return &singleton;
}

MapFixHellAddWeaponsAndAmmo* MapFixes::GetHellAddWeaponsAndAmmoSingleton()
{
	static MapFixHellAddWeaponsAndAmmo singleton;
	return &singleton;
}

void MapFixes::ApplyMapFixPatch()
{
	GetPatchAddWeaponsAndAmmoSingleton()->ApplyFix();
}

void MapFixes::ApplyMapFixCorn()
{
	GetCornAddWeaponsAndAmmoSingleton()->ApplyFix();
}

void MapFixes::ApplyMapFixShip()
{
	GetShipAddWeaponsAndAmmoSingleton()->ApplyFix();
}

void MapFixes::ApplyMapFixWoods()
{
	GetWoodsAddWeaponsAndAmmoSingleton()->ApplyFix();
}

void MapFixes::ApplyMapFixWitch()
{
	GetWitchSingleton()->ApplyFix();
}

void MapFixes::ApplyMapFixGrave()
{
	GetGraveAddWeaponsAndAmmoSingleton()->ApplyFix();
}

void MapFixes::ApplyMapFixVamp()
{
	GetVampSingleton()->ApplyFix();
}

void MapFixes::ApplyMapFixHell()
{
	GetHellAddWeaponsAndAmmoSingleton()->ApplyFix();
}

static MapFixes* GetMapFixesSingleton()
{
	static MapFixes singleton;
	return &singleton;
}

void MapFixes_ApplyAllPossibleFixes()
{
	if (IsCurrentMap("corn"))
	{
		GetMapFixesSingleton()->ApplyMapFixCorn();
	}
	else if (IsCurrentMap("grave"))
	{
		GetMapFixesSingleton()->ApplyMapFixGrave();
	}
	else if (IsCurrentMap("hell"))
	{
		GetMapFixesSingleton()->ApplyMapFixHell();
	}
	else if (IsCurrentMap("patch"))
	{
		GetMapFixesSingleton()->ApplyMapFixPatch();
	}
	else if (IsCurrentMap("ship"))
	{
		GetMapFixesSingleton()->ApplyMapFixShip();
	}
	else if (IsCurrentMap("vamp"))
	{
		GetMapFixesSingleton()->ApplyMapFixVamp();
	}
	else if (IsCurrentMap("witch"))
	{
		GetMapFixesSingleton()->ApplyMapFixWitch();
	}
	else if (IsCurrentMap("woods"))
	{
		GetMapFixesSingleton()->ApplyMapFixWoods();
	}
}

#endif // defined ( SHALL_MAPFIXES )