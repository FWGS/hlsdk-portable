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
#include "plane.h"

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CPlane::CPlane()
{
	m_fInitialized = FALSE;
}

//=========================================================
// InitializePlane - Takes a normal for the plane and a
// point on the plane and 
//=========================================================
void CPlane::InitializePlane( const Vector& normal, const Vector& point )
{
	m_vecNormal = normal;
	m_flDist = DotProduct( m_vecNormal, point );
	m_fInitialized = TRUE;
}

BOOL CPlane::PointInFront( const Vector& point )
{
	if ( !m_fInitialized )
	{
		return FALSE;
	}

	return DotProduct( m_vecNormal, point ) - m_flDist >= 0;
}
