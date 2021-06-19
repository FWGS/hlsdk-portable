//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once
#if !defined(VGUI_LOADTGA_H)
#define VGUI_LOADTGA_H

#include "vgui_bitmaptga.h"


vgui::BitmapTGA* vgui_LoadTGA(char const *pFilename);
vgui::BitmapTGA* vgui_LoadTGANoInvertAlpha(char const *pFilename);
#endif // VGUI_LOADTGA_H
