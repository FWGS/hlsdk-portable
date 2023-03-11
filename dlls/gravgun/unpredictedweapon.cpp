#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "unpredictedweapon.h"

BOOL CBasePlayerWeaponU::UseDecrement()
{
	return FALSE;
}

float CBasePlayerWeaponU::UTIL_WeaponTimeBase()
{
	return gpGlobals->time;
}

TYPEDESCRIPTION	CBasePlayerWeaponU::m_SaveData[] =
{

	DEFINE_FIELD( CBasePlayerWeapon, m_flNextPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flNextSecondaryAttack, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayerWeapon, m_flTimeWeaponIdle, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CBasePlayerWeaponU, CBasePlayerWeapon )
