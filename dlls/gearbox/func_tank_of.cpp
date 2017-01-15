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
#include "effects.h"
#include "weapons.h"
#include "explode.h"

#include "player.h"

#include "func_tank.h"


//=========================================================
// CFuncTankOF
//=========================================================
class CFuncTankOF : public CFuncTank
{
public:

};

LINK_ENTITY_TO_CLASS(func_tank_of, CFuncTankOF);

//=========================================================
// CFuncTankLaserOF
//=========================================================
class CFuncTankLaserOF: public CFuncTankLaser
{
public:
};

LINK_ENTITY_TO_CLASS(func_tanklaser_of, CFuncTankLaserOF);

//=========================================================
// CFuncTankRocketOF
//=========================================================

class CFuncTankRocketOF : public CFuncTankRocket
{
public:
};

LINK_ENTITY_TO_CLASS(func_tankrocket_of, CFuncTankRocketOF);

//=========================================================
// CFuncTankMortarOF
//=========================================================

class CFuncTankMortarOF : public CFuncTankMortar
{
public:
};

LINK_ENTITY_TO_CLASS(func_tankmortar_of, CFuncTankMortarOF);

//============================================================================
// FUNC TANK CONTROLS OF
//============================================================================

class CFuncTankControlsOF : public CFuncTankControls
{
public:

};

LINK_ENTITY_TO_CLASS(func_tankcontrols_of, CFuncTankControlsOF);