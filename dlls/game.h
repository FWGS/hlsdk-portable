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
#pragma once
#if !defined(GAME_H)
#define GAME_H

extern void GameDLLInit( void );

extern cvar_t displaysoundlist;

// multiplayer server rules
extern cvar_t teamplay;
extern cvar_t fraglimit;
extern cvar_t timelimit;
extern cvar_t friendlyfire;
extern cvar_t falldamage;
extern cvar_t weaponstay;
extern cvar_t selfgauss;
extern cvar_t chargerfix;
extern cvar_t satchelfix;
extern cvar_t explosionfix;
extern cvar_t monsteryawspeedfix;
extern cvar_t corpsephysics;
extern cvar_t pushablemode;
extern cvar_t forcerespawn;
extern cvar_t flashlight;
extern cvar_t aimcrosshair;
extern cvar_t decalfrequency;
extern cvar_t teamlist;
extern cvar_t teamoverride;
extern cvar_t defaultteam;
extern cvar_t allowmonsters;

// Engine Cvars
extern cvar_t *g_psv_gravity;
extern cvar_t *g_psv_aim;
extern cvar_t *g_footsteps;
extern cvar_t *g_enable_cheats;

extern cvar_t *g_psv_developer;
#endif // GAME_H

// BMOD Begin - CVARs
extern cvar_t	bm_ver;
extern cvar_t	bm_url;
extern cvar_t	bm_bver;
extern cvar_t	bm_bname;
extern cvar_t	bm_burl;
//extern cvar_t	bm_plat;
//extern cvar_t	bm_trips;
extern cvar_t	bm_guns;
extern cvar_t	bm_ammo;
extern cvar_t	bm_g;
extern cvar_t	bm_dmg_messages;
extern cvar_t	bm_matchkills;
extern cvar_t	bm_freezetime;
extern cvar_t	bm_thrust;
extern cvar_t	bm_spawneffects;
extern cvar_t	bm_snarktrails;
extern cvar_t	bm_xbowtracers;

extern cvar_t	bm_spawnkilltime;
extern cvar_t	bm_maxspawnkills;

extern cvar_t	bm_typekills;
extern cvar_t	bm_maxtypekills;
extern cvar_t	bm_typecam;

extern cvar_t	bm_bantime;

extern cvar_t	bm_antispam;
extern cvar_t	bm_spamlimit;

extern cvar_t	bm_spawnmines;
extern cvar_t   bm_spawnsatchels;
extern cvar_t   bm_voting;
extern cvar_t   bm_votetime;
extern cvar_t   bm_maxtime;
extern cvar_t   bm_maxfrags;
// extern cvar_t	bm_cbmad;
// extern cvar_t	bm_cheatdetect;
extern cvar_t	bm_mods;
extern cvar_t	bm_cbar_mod;
extern cvar_t	bm_mp5_mod;
extern cvar_t	bm_shotty_mod;
extern cvar_t	bm_xbow_mod;
extern cvar_t	bm_rpg_mod;
extern cvar_t	bm_tau_mod;
extern cvar_t	bm_gluon_mod;
extern cvar_t	bm_hornet_mod;
extern cvar_t	bm_trip_mod;
extern cvar_t	bm_snarks_mod;
/*
extern cvar_t	bm_score_crowbar;
extern cvar_t	bm_score_throwncbar;
extern cvar_t	bm_score_9mm;
extern cvar_t	bm_score_357;
extern cvar_t	bm_score_mp5;
extern cvar_t	bm_score_shotgun;
extern cvar_t	bm_score_squidspit;
extern cvar_t	bm_score_zapgun;
extern cvar_t	bm_score_mp5grenade;
extern cvar_t	bm_score_gluon;
extern cvar_t	bm_score_tau;
extern cvar_t	bm_score_bolt;
extern cvar_t	bm_score_crossbow;
extern cvar_t	bm_score_satchel;
extern cvar_t	bm_score_handgrenade;
extern cvar_t	bm_score_rpg;
extern cvar_t	bm_score_snarks;
extern cvar_t	bm_score_tripmine;
*/
extern cvar_t	bm_map;
extern cvar_t	bm_nextmap;

extern cvar_t	bm_rune_rand;
extern cvar_t	bm_runemask;
extern cvar_t	bm_rune_cbar;
extern cvar_t	bm_rune_cbar_t;
extern cvar_t	bm_rune_cbar_r;
extern cvar_t	bm_rune_gren;
extern cvar_t	bm_rune_gren_t;
extern cvar_t	bm_rune_gren_r;
extern cvar_t	bm_rune_357;
extern cvar_t	bm_rune_357_t;
extern cvar_t	bm_rune_357_r;
extern cvar_t	bm_rune_health;
extern cvar_t	bm_rune_health_t;
extern cvar_t	bm_rune_health_r;
extern cvar_t	bm_rune_armor;
extern cvar_t	bm_rune_armor_t;
extern cvar_t	bm_rune_armor_r;
extern cvar_t	bm_rune_shotty;
extern cvar_t	bm_rune_shotty_t;
extern cvar_t	bm_rune_shotty_r;
// BMOD End - CVARs

// BMOD Begin - server commands
void BModCmd_AdminSay( void );
void BModCmd_AdminWhisper( void );
void BModCmd_ShowSpawns( void );
void BModCmd_SpeakAll( void );
void BModCmd_Create( void );
void BModCmd_Remove( void );
void BModCmd_Delete( void );
void BModCmd_Replace( void );
void BModCmd_Info( void );
void BModCmd_Llama( void );
void BModCmd_Unllama( void );
// BMOD End - server commands
