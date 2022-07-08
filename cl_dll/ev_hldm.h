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
	BULLET_PLAYER_BUCKSHOT_DOUBLE, // modif de Julien
	BULLET_PLAYER_CROWBAR, // crowbar swipe

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,

	//modif de Julien
	BULLET_PLAYER_M16,
	BULLET_PLAYER_SNIPER,
	BULLET_PLAYER_IRGUN,

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

enum shotgun_e
{
	SHOTGUN_IDLE = 0,
	SHOTGUN_IDLE_DEEP,
	SHOTGUN_DRAW,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_START_RELOAD,
	SHOTGUN_RELOAD,
	SHOTGUN_PUMP
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

//modif de Julien
enum m16_e
{
	M16_LONGIDLE = 0,
	M16_IDLE1,
	M16_RELOAD,
	M16_DEPLOY,
	M16_FIRE1,
};

//modif de Julien
enum fsniper_e
{
	FSNIPER_LONGIDLE = 0,
	FSNIPER_IDLE1,
	FSNIPER_RELOAD,
	FSNIPER_DEPLOY,
	FSNIPER_FIRE1,
	FSNIPER_FIDGET,
};

// modif de Julien


enum irgun_e {
	IRGUN_FIDGET = 0,
	IRGUN_RELOAD_EMPTY,
	IRGUN_IDLE1,
	IRGUN_DRAW,
	IRGUN_FIRE1,
	IRGUN_IDLE_IR,
	IRGUN_FIRE_EMPTY,
	IRGUN_RELOAD
};

//modif de Julien

enum python_e
{
	/*
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_HOLSTER,
	PYTHON_DRAW,
	PYTHON_IDLE2,
	PYTHON_IDLE3
	*/
	PYTHON_DRAW = 0,
	PYTHON_IDLE1,
	PYTHON_IDLE2,
	PYTHON_IDLE3,
	PYTHON_FIRE1,
	PYTHON_FIRE_EMPTY,
	PYTHON_RELOAD_EMPTY,
	PYTHON_RELOAD

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
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY, int iTraverseMur = 0 );
void EV_HLDM_FireBulletsVect( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float *vecSpread, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, int iTraverseMur = 0 ){ //Julien's code uses old version of this function that took vector definitions like this, so we need to convert the Vector vec3_t into floats. This way we can keep using definitions, modif de Roy
	float flSpreadX = vecSpread[0];
	float flSpreadY = vecSpread[1];
	return EV_HLDM_FireBullets(idx,forward,right,up,cShots,vecSrc,vecDirShooting,flDistance,iBulletType,iTracerFreq,tracerCount,flSpreadX,flSpreadY,iTraverseMur);
}
#endif // EV_HLDMH
