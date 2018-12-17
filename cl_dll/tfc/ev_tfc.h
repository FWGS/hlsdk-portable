//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

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

	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM
}Bullet;

void EV_HLDM_GunshotDecalTrace( pmtrace_t *pTrace, char *decalName );
void EV_HLDM_DecalGunshot( pmtrace_t *pTrace, int iBulletType );
int EV_HLDM_CheckTracer( int idx, float *vecSrc, float *end, float *forward, float *right, int iBulletType, int iTracerFreq, int *tracerCount );
void EV_HLDM_FireBullets( int idx, float *forward, float *right, float *up, int cShots, float *vecSrc, float *vecDirShooting, float flDistance, int iBulletType, int iTracerFreq, int *tracerCount, float flSpreadX, float flSpreadY );
void EV_TFC_NailTouch( struct tempent_s *ent, pmtrace_t *ptr );
void EV_TFC_Explode( float *org, int dmg, pmtrace_t *pTrace, qboolean bDontSpark );
void EV_TFC_BloodDrips( float *vecOrigin, signed int iDamage, long double height );
void EV_TFC_TraceAttack( int idx, float *vecDir, pmtrace_t *ptr, float flDamage );
long double EV_TFC_WaterLevel( float *position, float minz, float maxz );
void EV_TFC_RailDie( struct particle_s *particle );
int EV_TFC_IsAlly( int idx1, int idx2 );
void EV_TFC_TranqNailTouch( tempent_s *ent, pmtrace_t *ptr );
void EV_TFC_PlayAxeSound( int idx, int classid, float *origin, int iSoundType, float fSoundData );
int EV_TFC_AxeHit(int idx, float *origin, float *forward, float *right, int entity, float *vecDir, pmtrace_t *ptr);
int EV_TFC_Medkit(int idx, float *origin, float *forward, float *right, int entity, float *vecDir, pmtrace_t *ptr);
tempent_s* EV_TFC_CreateGib(float *origin, float *attackdir, int multiplier, int ishead);

extern cvar_t *cl_gibcount;
extern cvar_t *cl_giblife;
extern cvar_t *cl_gibvelscale;
extern cvar_t *cl_localblood;

#endif // EV_HLDMH
