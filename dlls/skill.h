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
// skill.h - skill level concerns
//=========================================================
#pragma once
#if !defined(SKILL_H)
#define SKILL_H

struct skilldata_t
{
	int iSkillLevel; // game skill level

	// Monster Health & Damage
	float agruntHealth;
	float agruntDmgPunch;

	float agruntPoisonHealth;
	float agruntPoisonDmgPunch;

	float agruntPreHealth;
	float agruntPreDmgPunch;

	float apacheHealth;

	float barneyHealth;
	
	float otisHealth;

	float bigmommaHealthFactor;		// Multiply each node's health by this
	float bigmommaDmgSlash;			// melee attack damage
	float bigmommaDmgBlast;			// mortar attack damage
	float bigmommaRadiusBlast;		// mortar attack radius

	float twinGonarchHealthFactor; // Multiply each node's health by this
	float twinGonarchDmgSlash;		// melee attack damage
	float twinGonarchDmgBlast;		// mortar attack damage
	float twinGonarchRadiusBlast;	// mortar attack radius

	float bgargHealth;
	float bgargDmgSlash;
	float bgargDmgFire;

	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;

	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;

	float hassassinHealth;

	float headcrabHealth;
	float headcrabDmgBite;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;

	float massassinHealth;
	float massassinDmgKick;
	float massassinGrenadeSpeed;

	float sawGunnerHealth;
	float sawGunnerDmgKick;
	float sawGunnerGrenadeSpeed;

	float houndeyeHealth;
	float houndeyeDmgBlast;

	float houndeyeExpHealth;
	float houndeyeExpDmgBlast;
	float houndeyeExpDmgRadius;

	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;

	float slaveEmpHealth;
	float slaveEmpDmgZap;
	float slaveEmpDmgSonic;

	float ichthyosaurHealth;
	float ichthyosaurDmgShake;

	float leechHealth;
	float leechDmgBite;

	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;

	float controllerExtHealth;
	float controllerExtDmgZap;
	float controllerExtSpeedBall;
	float controllerExtDmgBall;

	float nihilanthHealth;
	float nihilanthZap;

	float scientistHealth;

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

	float zombieBarneyHealth;
	float zombieBarneyDmgOneSlash;
	float zombieBarneyDmgBothSlash;	
	
	float zombieSoldierHealth;
	float zombieSoldierDmgOneSlash;
	float zombieSoldierDmgBothSlash;

	float zombieHEVHealth;
	float zombieHEVDmgOneSlash;
	float zombieHEVDmgBothSlash;

	float zombieWorkerHealth;
	float zombieWorkerDmgOneSlash;
	float zombieWorkerDmgBothSlash;

	float gonomeHealth;
	float gonomeDmgOneSlash;
	float gonomeDmgGuts;
	float gonomeDmgOneBite;

	float turretHealth;
	float miniturretHealth;
	float sentryHealth;

	// Player Weapons
	float plrDmgCrowbar;
	float plrDmgKnife;
	float plrDmg9MM;
	float plrDmg357;
	float plrDmgEagle;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgCrossbowClient;
	float plrDmgCrossbowMonster;
	float plrDmgRPG;
	float plrDmgGauss;
	float plrDmgEgonNarrow;
	float plrDmgEgonWide;
	float plrDmgHornet;
	float plrDmgHandGrenade;
	float plrDmgSatchel;
	float plrDmgTripmine;
	float plrDmg556;
	float plrDmg762;
	
	// weapons shared by monsters
	float monDmg9MM;
	float monDmg556;
	float monDmg762;
	float monDmgMP5;
	float monDmg12MM;
	float monDmgHornet;
	float monDmgHornetAlt;

	// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;

	// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;

	// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
};

extern	DLL_GLOBAL	skilldata_t	gSkillData;
float GetSkillCvar( const char *pName );

extern DLL_GLOBAL int		g_iSkillLevel;

#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3
#endif // SKILL_H
