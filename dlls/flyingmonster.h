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
// Base class for flying monsters.  This overrides the movement test & execution code from CBaseMonster

#pragma once

class CFlyingMonster : public CBaseMonster
{
public:
	int CheckLocalMove( const Vector& start, const Vector& end, CBaseEntity* target, float* distance ) override;
	BOOL FTriangulate( const Vector& start, const Vector& end, float distance, CBaseEntity* target, Vector* apex ) override;
	Activity GetStoppedActivity() override;
	void Killed( entvars_t* attacker, int gib ) override;
	void Stop() override;
	float ChangeYaw( int yawSpeed ) override;
	void HandleAnimEvent( MonsterEvent_t* event ) override;
	void MoveExecute( CBaseEntity* target, const Vector& direction, float interval ) override;
	void Move( float interval = 0.1f ) override;
	BOOL ShouldAdvanceRoute( float distance ) override;
	void SetFlyingMomentum( float momentum ) { m_momentum = momentum; }
	void SetFlyingFlapSound( const char* sound ) { m_pFlapSound = sound; }
	void SetFlyingSpeed( float speed ) { m_flightSpeed = speed; }
	float CeilingZ( const Vector& position );
	float FloorZ( const Vector& position );
	BOOL ProbeZ( const Vector& position, const Vector& probe, float* fraction );


	// UNDONE:  Save/restore this stuff!!!
protected:
	Vector		m_vecTravel;		// Current direction
	float		m_flightSpeed;		// Current flight speed (decays when not flapping or gliding)
	float		m_stopTime;			// Last time we stopped (to avoid switching states too soon)
	float		m_momentum;			// Weight for desired vs. momentum velocity
	const char*	m_pFlapSound;
	float		m_flLastZYawTime;	// Last frame time Z was changed when yaw was changed
};
