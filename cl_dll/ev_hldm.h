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
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe
	BULLET_PLAYER_50CAL, // minigun

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

enum eagel_e 
{
	EAGEL_IDLE1 = 0,
	EAGEL_FIDGET1,
	EAGEL_FIRE1,
	EAGEL_RELOAD,
	EAGEL_HOLSTER,
	EAGEL_DRAW,
	EAGEL_IDLE2,
	EAGEL_IDLE3
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

enum shotguna_e 
{
	SHOTGUNA_IDLE,
	SHOTGUNA_IDLE2,
	SHOTGUNA_IDLE3,
	SHOTGUNA_SHOOT,
	SHOTGUNA_RELOAD,
	SHOTGUNA_START_RELOAD,
	SHOTGUNA_END_RELOAD,
	SHOTGUNA_DRAW,
	SHOTGUNA_HOLSTER
};

enum mp5_e
{
	MP5_LONGIDLE = 0,
	MP5_IDLE1,
	MP5_LAUNCH,
	MP5_RELOAD,
	MP5_DEPLOY,
	MP5_FIRE1,
	MP5_FIRE2,
	MP5_FIRE3
};

enum ak47_e
{
	AK47_LONGIDLE = 0,
	AK47_IDLE1,
	AK47_GRENADE,
	AK47_RELOAD,
	AK47_DEPLOY_1,
	AK47_SHOOT_1,
	AK47_SHOOT_2,
	AK47_SHOOT_3
};

enum uzi_e
{
	UZI_LONGIDLE = 0,
	UZI_IDLE1,
	UZI_RELOAD,
	UZI_DEPLOY,
	UZI_FIRE1,
	UZI_FIRE2,
	UZI_FIRE3
};

enum minigun_e
{
	MINIGUN_LONGIDLE = 0,
	MINIGUN_IDLE1,
	MINIGUN_RELOAD,
	MINIGUN_DEPLOY,
	MINIGUN_FIRE1,
	MINIGUN_FIRE2,
	MINIGUN_FIRE3
};

enum mp41a_e
{
	MP41a_LONGIDLE = 0,
	MP41a_IDLE1,
	MP41a_LAUNCH,
	MP41a_RELOAD,
	MP41a_DEPLOY,
	MP41a_FIRE1,
	MP41a_FIRE2,
	MP41a_FIRE3
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

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );
bool EV_PointLineIntersect( vec3_t start, vec3_t end, vec3_t point, float rad, vec3_t intersection );
#endif // EV_HLDMH
