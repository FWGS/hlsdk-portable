/*
cz_fibercamera.h - CZ:DS fiber optic camera and info_camera views
Copyright (C) 2026 sergiopoverony
*/
#pragma once

struct ref_params_s;

void CzFiber_Init( void );
void CzFiber_VidInit( void );
bool CzFiber_IsOn( void );
bool CzFiber_CalcRefdef( struct ref_params_s *pparams ); // true: the view is the camera's
void CzFiber_Draw( float flTime );
