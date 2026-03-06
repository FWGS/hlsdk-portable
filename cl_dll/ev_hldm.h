//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma once
#if !defined ( EV_HLDMH )
#define EV_HLDMH

// bullet types
typedef	enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_556MM, // m4a1
	BULLET_PLAYER_45ACP, // smg
	BULLET_PLAYER_14MM, // m82a1
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_44, // 44desert_eagle
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM
}Bullet;

enum glock_e
{
	GLOCK_IDLE1 = 0,
	GLOCK_IDLE2,
	GLOCK_IDLE3,
	GLOCK_SHOOT,
	GLOCK_SHOOT_EMPTY,
	GLOCK_RELOAD,
	GLOCK_RELOAD_NOT_EMPTY,
	GLOCK_DRAW,
	GLOCK_HOLSTER,
	GLOCK_ADD_SILENCER
};

enum deagle_e {
	DEAGLE_IDLE = 0,
	DEAGLE_SHOOT,
	DEAGLE_SHOOT_EMPTY,
	DEAGLE_RELOAD,
	DEAGLE_RELOAD_NOT_EMPTY,
	DEAGLE_DRAW,
	DEAGLE_DRAW_DRY,
	DEAGLE_HOLSTER,
	DEAGLE_HOLSTER_DRY,
};

enum shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP,
	SHOTGUN_START_RELOAD,
	SHOTGUN_DRAW,
	SHOTGUN_HOLSTER,
	SHOTGUN_IDLE4,
	SHOTGUN_IDLE_DEEP
};

enum m4a1_e
{
	M4A1_IDLE1 = 0,
	M4A1_DEPLOY,
	M4A1_HOLSTER,
	M4A1_FIRE1,
	M4A1_RELOAD,
	M4A1_RELOAD_NOT_EMPTY,
	M4A1_LAUNCH_GREN,
	M4A1_LAST_GREN,
};

enum sniperrifle_e
{
	SNIPERRIFLE_SLOWIDLE = 0,
	SNIPERRIFLE_FIRE1,
	SNIPERRIFLE_FIRE2,
	SNIPERRIFLE_RELOAD,
	SNIPERRIFLE_RELOAD2,
	SNIPERRIFLE_DRAW,
	SNIPERRIFLE_DRAW_EMPTY,
	SNIPERRIFLE_HOLSTER,
	SNIPERRIFLE_HOLSTER_EMPTY,
};

enum smg_e
{
	SMG_LONGIDLE = 0,
	SMG_IDLE1,
	SMG_RELOAD,
	SMG_RELOAD_NOT_EMPTY,
	SMG_DEPLOY,
	SMG_FIRE1,
	SMG_FIRE2,
	SMG_HOLSTER,
	SMG_SILENCER_ADD,
	SMG_SILENCER_REMOVE,
};

enum python_e
{
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
};

#define	GAUSS_PRIMARY_CHARGE_VOLUME	256// how loud gauss is while charging
#define GAUSS_PRIMARY_FIRE_VOLUME	450// how loud gauss is when discharged

enum gauss_e
{
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

enum
{
	SMOKE_WALLPUFF = 0,
};

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType, char cTextureType = 0, bool isSky = false );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );
void EV_HLDM_MuzzleFlash(vec3_t pos, float amount);
#endif // EV_HLDMH
