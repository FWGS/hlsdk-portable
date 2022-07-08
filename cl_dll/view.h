//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#if !defined ( VIEWH )
#define VIEWH 
#pragma once

#if !defined(M_PI)
#define M_PI		3.14159265358979323846  // matches value in gcc v2 math.h
#endif

#if !defined(M_PI_F)
#define M_PI_F		(float)M_PI
#endif

// Spectator flags
#define SPEC_IS_SPECTATOR		(1<<0)
#define SPEC_SMOOTH_ANGLES		(1<<1)
#define SPEC_SMOOTH_ORIGIN		(1<<2)

void V_StartPitchDrift( void );
void V_StopPitchDrift( void );
#endif // !VIEWH
