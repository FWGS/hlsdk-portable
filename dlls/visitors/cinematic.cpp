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
#include "game.h"
#include "cinematic.h"

TYPEDESCRIPTION CCinematicCamera::m_SaveData[] =
{
	DEFINE_ARRAY( CCinematicCamera, m_vecOrigin, FIELD_FLOAT, 3 ),
	DEFINE_FIELD( CCinematicCamera, m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( CCinematicCamera, m_hPlayer, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CCinematicCamera, CBaseEntity )

BOOL CCinematicCamera::IsCinematic()
{
	return cinematics.value;
}

CCinematicCamera *CCinematicCamera::CreateCinematicCamera()
{
	CCinematicCamera *pCam = GetClassPtr( (CCinematicCamera *)NULL );
	pCam->Spawn();
	return pCam;
}

void CCinematicCamera::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	pev->renderamt = 0;
	pev->rendermode = kRenderTransAlpha;
	SET_MODEL( ENT( pev ), "models/crossbow_bolt.mdl" );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
}

void CCinematicCamera::SetOrigin( const Vector &p_vecOrigin )
{
	m_vecOrigin = p_vecOrigin;
}

void CCinematicCamera::SetTarget( CBaseEntity *pEntity )
{
	m_hTarget = pEntity;
}

void CCinematicCamera::SetPlayer( CBaseEntity *pEntity )
{
	m_hPlayer = pEntity;
}

void CCinematicCamera::SetViewOnTarget()
{
	if( m_hTarget != 0 )
	{
		UTIL_SetOrigin( pev, m_hTarget->pev->origin + m_vecOrigin );
		SET_VIEW( m_hPlayer->edict(), edict() );
	}
}

void CCinematicCamera::SetViewOnPlayer()
{
	if( IsCinematic() || FClassnameIs( m_hTarget->pev, "trigger_camera" ) || m_bIsDeathCamera )
	{
		UTIL_SetOrigin( pev, m_hTarget->pev->origin + m_vecOrigin );
		pev->angles = m_hTarget->pev->angles;
		if( m_hTarget->IsPlayer() && m_vecOrigin == g_vecZero )
			SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
		else
			SET_VIEW( m_hPlayer->edict(), edict() );
	}
	else
		SET_VIEW( m_hPlayer->edict(), m_hPlayer->edict() );
}
