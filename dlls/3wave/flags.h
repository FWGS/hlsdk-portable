/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
//=========================================================
// Flags
//=========================================================
#pragma once
#ifndef FLAGS_H
#define FLAGS_H

#define STEAL_SOUND 1
#define CAPTURE_SOUND 2
#define RETURN_SOUND 3

#define RED_FLAG_STOLEN 1
#define BLUE_FLAG_STOLEN 2
#define RED_FLAG_CAPTURED 3
#define BLUE_FLAG_CAPTURED 4
#define RED_FLAG_RETURNED_PLAYER 5
#define BLUE_FLAG_RETURNED_PLAYER 6
#define RED_FLAG_RETURNED 7
#define BLUE_FLAG_RETURNED 8
#define RED_FLAG_LOST 9
#define BLUE_FLAG_LOST 10

#define RED_FLAG_STOLEN 1
#define BLUE_FLAG_STOLEN 2
#define RED_FLAG_DROPPED 3
#define BLUE_FLAG_DROPPED 4
#define RED_FLAG_ATBASE 5
#define BLUE_FLAG_ATBASE 6

// Standard Scoring
#define TEAM_CAPTURE_CAPTURE_BONUS 5 // what you get for capture
#define TEAM_CAPTURE_TEAM_BONUS 10 // what your team gets for capture
#define TEAM_CAPTURE_RECOVERY_BONUS 1 // what you get for recovery
#define TEAM_CAPTURE_FLAG_BONUS 0 // what you get for picking up enemy flag
#define TEAM_CAPTURE_FRAG_CARRIER_BONUS 2 // what you get for fragging a enemy flag carrier
#define TEAM_CAPTURE_FLAG_RETURN_TIME 40 // seconds until auto return
 
// bonuses
#define TEAM_CAPTURE_CARRIER_DANGER_PROTECT_BONUS  2 // bonus for fraggin someone
// who has recently hurt your flag carrier
#define TEAM_CAPTURE_CARRIER_PROTECT_BONUS 1 // bonus for fraggin someone while
// either you or your target are near your flag carrier
#define TEAM_CAPTURE_FLAG_DEFENSE_BONUS 1 // bonus for fraggin someone while
// either you or your target are near your flag
#define TEAM_CAPTURE_RETURN_FLAG_ASSIST_BONUS 1 // awarded for returning a flag that causes a
// capture to happen almost immediately
#define TEAM_CAPTURE_FRAG_CARRIER_ASSIST_BONUS 2 // award for fragging a flag carrier if a
// capture happens almost immediately

// Radius
#define TEAM_CAPTURE_TARGET_PROTECT_RADIUS 550 // the radius around an object being
// defended where a target will be worth extra frags
#define TEAM_CAPTURE_ATTACKER_PROTECT_RADIUS 550 // the radius around an object being
// defended where an attacker will get extra frags when making kills
        
// timeouts
#define TEAM_CAPTURE_CARRIER_DANGER_PROTECT_TIMEOUT 4
#define TEAM_CAPTURE_CARRIER_FLAG_SINCE_TIMEOUT 2
#define TEAM_CAPTURE_FRAG_CARRIER_ASSIST_TIMEOUT 6
#define TEAM_CAPTURE_RETURN_FLAG_ASSIST_TIMEOUT 4

extern const char *GetTeamName( int team );

class CItemFlag : public CBaseEntity
{
public:
	void Spawn();
	BOOL Dropped;
	float m_flDroppedTime;
	void EXPORT FlagThink();

private:
	void Precache();
	void Capture( CBasePlayer *pPlayer, int iTeam );
	void ResetFlag( int iTeam );
	void Materialize();
	void EXPORT FlagTouch( CBaseEntity *pOther );
	// BOOL MyTouch( CBasePlayer *pPlayer );
};

class CCarriedFlag : public CBaseEntity
{
public:
	void Spawn(); 
	CBasePlayer *Owner;
	int m_iOwnerOldVel;

private:
	void Precache();
	void EXPORT FlagThink();
};

#endif // FLAGS_H
