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
#ifndef SKILL_H
#define SKILL_H

struct skilldata_t
{
	int iSkillLevel; // game skill level

	// Monster Health & Damage
	float panicDuration;		// replace this for either small/med/large or individual monsters

	float agruntHealth;
	float agruntDmgPunch;

	float apacheHealth;

	float barneyHealth;

	float bigmommaHealthFactor;		// Multiply each node's health by this
	float bigmommaDmgSlash;			// melee attack damage
	float bigmommaDmgBlast;			// mortar attack damage
	float bigmommaRadiusBlast;		// mortar attack radius

	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;

	float cthonianHealth;
	float cthonianDmgBite;
	float cthonianDmgWhip;
	float cthonianDmgSpit;

	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;

	float hassassinHealth;

	float headcrabHealth;
	float headcrabDmgBite;

	float gangsterHealth;
	float gangsterDmgKick;
	float gangsterShotgunPellets;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;

	float cultistHealth;
	float cultistDmgKick;
	float cultistShotgunPellets;

	float houndeyeHealth;
	float houndeyeDmgBlast;

	float greatraceHealth;
	float greatraceDmgClaw;
	float greatraceDmgClawrake;
	float greatraceDmgZap;

	float yodanHealth;
	float yodanDmgClaw;
	float yodanDmgClawrake;
	float yodanDmgZap;

	float serpentmanHealth;
	float serpentmanDmgStaff;

	float nightgauntHealth;
	float nightgauntDmgSlash;

	float priestHealth;
	float priestDmgKnife;

	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;

	float ichthyosaurHealth;
	float ichthyosaurDmgShake;

	float leechHealth;
	float leechDmgBite;

	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;

	float nihilanthHealth;
	float nihilanthZap;

	float scientistHealth;

	float butlerHealth;

	float sirhenryHealth;
	float sirhenryDmgZap;
	float sirhenryDmgKnife;

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float formless_spawnHealth;
	float formless_spawnDmgAttack;

	float snakeHealth;
	float snakeDmgBite;

	float deeponeHealth;
	float deeponeDmgOneSlash;
	float deeponeDmgBothSlash;

	float shamblerHealth;
	float shamblerDmgOneSlash;
	float shamblerDmgBothSlash;

	float huntinghorrorHealth;
	float huntinghorrorDmgBite;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

	float ghoulHealth;
	float ghoulDmgOneSlash;
	float ghoulDmgBothSlash;

	float turretHealth;
	float miniturretHealth;
	float sentryHealth;

	// Player Weapons
	float plrDmgSwordCane;
	float plrDmgKnife;
	float plrDmgRevolver;
	float plrDmgShotgun;
	float plrDmgTommyGun;
	float plrDmgRifle;
	float plrDmgDynamite;
	float plrDmgMolotov;
	float plrDmgLightningGun;
	float plrDmgShrivellingNarrow;
	float plrDmgShrivellingWide;
	float plrDmgDrainLife;

	// OLD WEAPONS
	float plrDmgCrowbar;
	float plrDmg9MM;
	float plrDmg357;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgCrossbowClient;
	float plrDmgCrossbowMonster;
	float plrDmgRPG;
	float plrDmgGauss;
	float plrDmgEgonNarrow;
	float plrDmgEgonWide;
	float plrDmgHornet;
	float plrDmgHandGrenade;
	float plrDmgTripmine;
	float plrDmgSatchel;
	
	// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float monDmg12MM;
	float monDmgHornet;
	float monDmgLightningGun;
	float monDmgShrivelling;
	float monDmgDagger;

	// health/suit charge
	// float suitchargerCapacity;
	// float batteryCapacity;
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
