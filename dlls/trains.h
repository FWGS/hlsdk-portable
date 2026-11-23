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

#pragma once

#include "cbase.h"

// Tracktrain spawn flags
#define SF_TRACKTRAIN_NOPITCH		0x0001
#define SF_TRACKTRAIN_NOCONTROL		0x0002
#define SF_TRACKTRAIN_FORWARDONLY	0x0004
#define SF_TRACKTRAIN_PASSABLE		0x0008

#define SF_PATH_DISABLED			0x0001
#define SF_PATH_FIREONCE			0x0002
#define SF_PATH_ALTREVERSE		0x0004
#define SF_PATH_DISABLE_TRAIN		0x0008
#define SF_PATH_ALTERNATE			0x8000

#define SF_CORNER_WAITFORTRIG		0x0001
#define SF_CORNER_TELEPORT		0x0002
#define SF_CORNER_FIREONCE		0x0004

//#define PATH_SPARKLE_DEBUG		1	// This makes a particle effect around path_track entities for debugging
class CPathTrack : public CPointEntity
{
public:
	void Spawn() override;
	void Activate() override;
	void KeyValue( KeyValueData* data ) override;
	void SetPrevious( CPathTrack* previous );
	void Link();
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	CPathTrack* ValidPath( CPathTrack* path, int testFlag );
	void Project( CPathTrack* start, CPathTrack* end, Vector* origin, float distance );
	static CPathTrack* Instance( edict_t* entity );
	CPathTrack* LookAhead( Vector* origin, float distance, int move );
	CPathTrack* Nearest( Vector origin );
	CPathTrack* GetNext();
	CPathTrack* GetPrevious();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	float		m_length;
	string_t	m_altName;
	CPathTrack*	m_pnext;
	CPathTrack*	m_pprevious;
	CPathTrack*	m_paltpath;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFuncTrackTrain : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void Blocked( CBaseEntity* other ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	void KeyValue( KeyValueData* data ) override;
	void EXPORT Next();
	void EXPORT Find();
	void EXPORT NearestPath();
	void EXPORT DeadEnd();
	void NextThink( float thinkTime, BOOL alwaysThink );
	void SetTrack( CPathTrack* track ) { m_ppath = track->Nearest( pev->origin ); }
	void SetControls( entvars_t* controls );
	BOOL OnControls( entvars_t* entity ) override;
	void StopSound();
	void UpdateSound();
	static CFuncTrackTrain* Instance( edict_t* entity );
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override;
	void OverrideReset() override;
	static TYPEDESCRIPTION m_SaveData[];

	CPathTrack*	m_ppath;
	float		m_length;
	float		m_height;
	float		m_speed;
	float		m_dir;
	float		m_startSpeed;
	Vector		m_controlMins;
	Vector		m_controlMaxs;
	int			m_soundPlaying;
	int			m_sounds;
	float		m_flVolume;
	float		m_flBank;
	float		m_oldSpeed;

private:
	unsigned short m_usAdjustPitch;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
