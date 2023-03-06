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
#include "extdll.h"
#include "eiface.h"
#include "util.h"
#include "game.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "BMOD_boxmarker.h"

BOOL		g_fIsXash3D;

cvar_t displaysoundlist = {"displaysoundlist","0"};

// multiplayer server rules
cvar_t fragsleft	= { "mp_fragsleft","0", FCVAR_SERVER | FCVAR_UNLOGGED };	  // Don't spam console/log files/users with this changing
cvar_t timeleft		= { "mp_timeleft","0" , FCVAR_SERVER | FCVAR_UNLOGGED };	  // "      "

// multiplayer server rules
cvar_t teamplay		= { "mp_teamplay","0", FCVAR_SERVER };
cvar_t fraglimit	= { "mp_fraglimit","0", FCVAR_SERVER };
cvar_t timelimit	= { "mp_timelimit","0", FCVAR_SERVER };
cvar_t friendlyfire	= { "mp_friendlyfire","0", FCVAR_SERVER };
cvar_t falldamage	= { "mp_falldamage","0", FCVAR_SERVER };
cvar_t weaponstay	= { "mp_weaponstay","0", FCVAR_SERVER };
cvar_t selfgauss	= { "selfgauss", "1", FCVAR_SERVER };
cvar_t chargerfix	= { "chargerfix", "0", FCVAR_SERVER };
cvar_t satchelfix	= { "satchelfix", "0", FCVAR_SERVER };
cvar_t explosionfix	= { "explosionfix", "0", FCVAR_SERVER };
cvar_t monsteryawspeedfix	= { "monsteryawspeedfix", "1", FCVAR_SERVER };
cvar_t corpsephysics = { "corpsephysics", "0", FCVAR_SERVER };
cvar_t pushablemode = { "pushablemode", "0", FCVAR_SERVER };
cvar_t forcerespawn	= { "mp_forcerespawn","1", FCVAR_SERVER };
cvar_t flashlight	= { "mp_flashlight","0", FCVAR_SERVER };
cvar_t aimcrosshair	= { "mp_autocrosshair","1", FCVAR_SERVER };
cvar_t decalfrequency	= { "decalfrequency","30", FCVAR_SERVER };
cvar_t teamlist		= { "mp_teamlist","hgrunt;scientist", FCVAR_SERVER };
cvar_t teamoverride	= { "mp_teamoverride","1" };
cvar_t defaultteam	= { "mp_defaultteam","0" };
cvar_t allowmonsters	= { "mp_allowmonsters","0", FCVAR_SERVER };
cvar_t bhopcap		= { "mp_bhopcap", "1", FCVAR_SERVER };

cvar_t allow_spectators = { "allow_spectators", "0", FCVAR_SERVER };	// 0 prevents players from being spectators
cvar_t multibyte_only = { "mp_multibyte_only", "0", FCVAR_SERVER };

cvar_t mp_chattime	= { "mp_chattime","10", FCVAR_SERVER };

// Engine Cvars
cvar_t *g_psv_gravity = NULL;
cvar_t *g_psv_aim = NULL;
cvar_t *g_footsteps = NULL;
cvar_t *g_enable_cheats = NULL;

cvar_t *g_psv_developer;

//CVARS FOR SKILL LEVEL SETTINGS
// Agrunt
cvar_t	sk_agrunt_health1 = {"sk_agrunt_health1","0"};
cvar_t	sk_agrunt_health2 = {"sk_agrunt_health2","0"};
cvar_t	sk_agrunt_health3 = {"sk_agrunt_health3","0"};

cvar_t	sk_agrunt_dmg_punch1 = {"sk_agrunt_dmg_punch1","0"};
cvar_t	sk_agrunt_dmg_punch2 = {"sk_agrunt_dmg_punch2","0"};
cvar_t	sk_agrunt_dmg_punch3 = {"sk_agrunt_dmg_punch3","0"};

// Apache
cvar_t	sk_apache_health1 = {"sk_apache_health1","0"};
cvar_t	sk_apache_health2 = {"sk_apache_health2","0"};
cvar_t	sk_apache_health3 = {"sk_apache_health3","0"};

// Barney
cvar_t	sk_barney_health1 = {"sk_barney_health1","0"};
cvar_t	sk_barney_health2 = {"sk_barney_health2","0"};
cvar_t	sk_barney_health3 = {"sk_barney_health3","0"};

// Bullsquid
cvar_t	sk_bullsquid_health1 = {"sk_bullsquid_health1","0"};
cvar_t	sk_bullsquid_health2 = {"sk_bullsquid_health2","0"};
cvar_t	sk_bullsquid_health3 = {"sk_bullsquid_health3","0"};

cvar_t	sk_bullsquid_dmg_bite1 = {"sk_bullsquid_dmg_bite1","0"};
cvar_t	sk_bullsquid_dmg_bite2 = {"sk_bullsquid_dmg_bite2","0"};
cvar_t	sk_bullsquid_dmg_bite3 = {"sk_bullsquid_dmg_bite3","0"};

cvar_t	sk_bullsquid_dmg_whip1 = {"sk_bullsquid_dmg_whip1","0"};
cvar_t	sk_bullsquid_dmg_whip2 = {"sk_bullsquid_dmg_whip2","0"};
cvar_t	sk_bullsquid_dmg_whip3 = {"sk_bullsquid_dmg_whip3","0"};

cvar_t	sk_bullsquid_dmg_spit1 = {"sk_bullsquid_dmg_spit1","0"};
cvar_t	sk_bullsquid_dmg_spit2 = {"sk_bullsquid_dmg_spit2","0"};
cvar_t	sk_bullsquid_dmg_spit3 = {"sk_bullsquid_dmg_spit3","0"};


// Big Momma
cvar_t	sk_bigmomma_health_factor1 = {"sk_bigmomma_health_factor1","1.0"};
cvar_t	sk_bigmomma_health_factor2 = {"sk_bigmomma_health_factor2","1.0"};
cvar_t	sk_bigmomma_health_factor3 = {"sk_bigmomma_health_factor3","1.0"};

cvar_t	sk_bigmomma_dmg_slash1 = {"sk_bigmomma_dmg_slash1","50"};
cvar_t	sk_bigmomma_dmg_slash2 = {"sk_bigmomma_dmg_slash2","50"};
cvar_t	sk_bigmomma_dmg_slash3 = {"sk_bigmomma_dmg_slash3","50"};

cvar_t	sk_bigmomma_dmg_blast1 = {"sk_bigmomma_dmg_blast1","100"};
cvar_t	sk_bigmomma_dmg_blast2 = {"sk_bigmomma_dmg_blast2","100"};
cvar_t	sk_bigmomma_dmg_blast3 = {"sk_bigmomma_dmg_blast3","100"};

cvar_t	sk_bigmomma_radius_blast1 = {"sk_bigmomma_radius_blast1","250"};
cvar_t	sk_bigmomma_radius_blast2 = {"sk_bigmomma_radius_blast2","250"};
cvar_t	sk_bigmomma_radius_blast3 = {"sk_bigmomma_radius_blast3","250"};

// Gargantua
cvar_t	sk_gargantua_health1 = {"sk_gargantua_health1","0"};
cvar_t	sk_gargantua_health2 = {"sk_gargantua_health2","0"};
cvar_t	sk_gargantua_health3 = {"sk_gargantua_health3","0"};

cvar_t	sk_gargantua_dmg_slash1	= {"sk_gargantua_dmg_slash1","0"};
cvar_t	sk_gargantua_dmg_slash2	= {"sk_gargantua_dmg_slash2","0"};
cvar_t	sk_gargantua_dmg_slash3	= {"sk_gargantua_dmg_slash3","0"};

cvar_t	sk_gargantua_dmg_fire1 = {"sk_gargantua_dmg_fire1","0"};
cvar_t	sk_gargantua_dmg_fire2 = {"sk_gargantua_dmg_fire2","0"};
cvar_t	sk_gargantua_dmg_fire3 = {"sk_gargantua_dmg_fire3","0"};

cvar_t	sk_gargantua_dmg_stomp1	= {"sk_gargantua_dmg_stomp1","0"};
cvar_t	sk_gargantua_dmg_stomp2	= {"sk_gargantua_dmg_stomp2","0"};
cvar_t	sk_gargantua_dmg_stomp3	= {"sk_gargantua_dmg_stomp3","0"};


// Hassassin
cvar_t	sk_hassassin_health1 = {"sk_hassassin_health1","0"};
cvar_t	sk_hassassin_health2 = {"sk_hassassin_health2","0"};
cvar_t	sk_hassassin_health3 = {"sk_hassassin_health3","0"};


// Headcrab
cvar_t	sk_headcrab_health1 = {"sk_headcrab_health1","0"};
cvar_t	sk_headcrab_health2 = {"sk_headcrab_health2","0"};
cvar_t	sk_headcrab_health3 = {"sk_headcrab_health3","0"};

cvar_t	sk_headcrab_dmg_bite1 = {"sk_headcrab_dmg_bite1","0"};
cvar_t	sk_headcrab_dmg_bite2 = {"sk_headcrab_dmg_bite2","0"};
cvar_t	sk_headcrab_dmg_bite3 = {"sk_headcrab_dmg_bite3","0"};


// Hgrunt 
cvar_t	sk_hgrunt_health1 = {"sk_hgrunt_health1","0"};
cvar_t	sk_hgrunt_health2 = {"sk_hgrunt_health2","0"};
cvar_t	sk_hgrunt_health3 = {"sk_hgrunt_health3","0"};

cvar_t	sk_hgrunt_kick1 = {"sk_hgrunt_kick1","0"};
cvar_t	sk_hgrunt_kick2 = {"sk_hgrunt_kick2","0"};
cvar_t	sk_hgrunt_kick3 = {"sk_hgrunt_kick3","0"};

cvar_t	sk_hgrunt_pellets1 = {"sk_hgrunt_pellets1","0"};
cvar_t	sk_hgrunt_pellets2 = {"sk_hgrunt_pellets2","0"};
cvar_t	sk_hgrunt_pellets3 = {"sk_hgrunt_pellets3","0"};

cvar_t	sk_hgrunt_gspeed1 = {"sk_hgrunt_gspeed1","0"};
cvar_t	sk_hgrunt_gspeed2 = {"sk_hgrunt_gspeed2","0"};
cvar_t	sk_hgrunt_gspeed3 = {"sk_hgrunt_gspeed3","0"};

// Houndeye
cvar_t	sk_houndeye_health1 = {"sk_houndeye_health1","0"};
cvar_t	sk_houndeye_health2 = {"sk_houndeye_health2","0"};
cvar_t	sk_houndeye_health3 = {"sk_houndeye_health3","0"};

cvar_t	sk_houndeye_dmg_blast1 = {"sk_houndeye_dmg_blast1","0"};
cvar_t	sk_houndeye_dmg_blast2 = {"sk_houndeye_dmg_blast2","0"};
cvar_t	sk_houndeye_dmg_blast3 = {"sk_houndeye_dmg_blast3","0"};


// ISlave
cvar_t	sk_islave_health1 = {"sk_islave_health1","0"};
cvar_t	sk_islave_health2 = {"sk_islave_health2","0"};
cvar_t	sk_islave_health3 = {"sk_islave_health3","0"};

cvar_t	sk_islave_dmg_claw1 = {"sk_islave_dmg_claw1","0"};
cvar_t	sk_islave_dmg_claw2 = {"sk_islave_dmg_claw2","0"};
cvar_t	sk_islave_dmg_claw3 = {"sk_islave_dmg_claw3","0"};

cvar_t	sk_islave_dmg_clawrake1	= {"sk_islave_dmg_clawrake1","0"};
cvar_t	sk_islave_dmg_clawrake2	= {"sk_islave_dmg_clawrake2","0"};
cvar_t	sk_islave_dmg_clawrake3	= {"sk_islave_dmg_clawrake3","0"};
	
cvar_t	sk_islave_dmg_zap1 = {"sk_islave_dmg_zap1","0"};
cvar_t	sk_islave_dmg_zap2 = {"sk_islave_dmg_zap2","0"};
cvar_t	sk_islave_dmg_zap3 = {"sk_islave_dmg_zap3","0"};


// Icthyosaur
cvar_t	sk_ichthyosaur_health1	= {"sk_ichthyosaur_health1","0"};
cvar_t	sk_ichthyosaur_health2	= {"sk_ichthyosaur_health2","0"};
cvar_t	sk_ichthyosaur_health3	= {"sk_ichthyosaur_health3","0"};

cvar_t	sk_ichthyosaur_shake1	= {"sk_ichthyosaur_shake1","0"};
cvar_t	sk_ichthyosaur_shake2	= {"sk_ichthyosaur_shake2","0"};
cvar_t	sk_ichthyosaur_shake3	= {"sk_ichthyosaur_shake3","0"};


// Leech
cvar_t	sk_leech_health1 = {"sk_leech_health1","0"};
cvar_t	sk_leech_health2 = {"sk_leech_health2","0"};
cvar_t	sk_leech_health3 = {"sk_leech_health3","0"};

cvar_t	sk_leech_dmg_bite1 = {"sk_leech_dmg_bite1","0"};
cvar_t	sk_leech_dmg_bite2 = {"sk_leech_dmg_bite2","0"};
cvar_t	sk_leech_dmg_bite3 = {"sk_leech_dmg_bite3","0"};

// Controller
cvar_t	sk_controller_health1 = {"sk_controller_health1","0"};
cvar_t	sk_controller_health2 = {"sk_controller_health2","0"};
cvar_t	sk_controller_health3 = {"sk_controller_health3","0"};

cvar_t	sk_controller_dmgzap1 = {"sk_controller_dmgzap1","0"};
cvar_t	sk_controller_dmgzap2 = {"sk_controller_dmgzap2","0"};
cvar_t	sk_controller_dmgzap3 = {"sk_controller_dmgzap3","0"};

cvar_t	sk_controller_speedball1 = {"sk_controller_speedball1","0"};
cvar_t	sk_controller_speedball2 = {"sk_controller_speedball2","0"};
cvar_t	sk_controller_speedball3 = {"sk_controller_speedball3","0"};

cvar_t	sk_controller_dmgball1 = {"sk_controller_dmgball1","0"};
cvar_t	sk_controller_dmgball2 = {"sk_controller_dmgball2","0"};
cvar_t	sk_controller_dmgball3 = {"sk_controller_dmgball3","0"};

// Nihilanth
cvar_t	sk_nihilanth_health1 = {"sk_nihilanth_health1","0"};
cvar_t	sk_nihilanth_health2 = {"sk_nihilanth_health2","0"};
cvar_t	sk_nihilanth_health3 = {"sk_nihilanth_health3","0"};

cvar_t	sk_nihilanth_zap1 = {"sk_nihilanth_zap1","0"};
cvar_t	sk_nihilanth_zap2 = {"sk_nihilanth_zap2","0"};
cvar_t	sk_nihilanth_zap3 = {"sk_nihilanth_zap3","0"};

// Scientist
cvar_t	sk_scientist_health1 = {"sk_scientist_health1","0"};
cvar_t	sk_scientist_health2 = {"sk_scientist_health2","0"};
cvar_t	sk_scientist_health3 = {"sk_scientist_health3","0"};

// Snark
cvar_t	sk_snark_health1 = {"sk_snark_health1","0"};
cvar_t	sk_snark_health2 = {"sk_snark_health2","0"};
cvar_t	sk_snark_health3 = {"sk_snark_health3","0"};

cvar_t	sk_snark_dmg_bite1 = {"sk_snark_dmg_bite1","0"};
cvar_t	sk_snark_dmg_bite2 = {"sk_snark_dmg_bite2","0"};
cvar_t	sk_snark_dmg_bite3 = {"sk_snark_dmg_bite3","0"};

cvar_t	sk_snark_dmg_pop1 = {"sk_snark_dmg_pop1","0"};
cvar_t	sk_snark_dmg_pop2 = {"sk_snark_dmg_pop2","0"};
cvar_t	sk_snark_dmg_pop3 = {"sk_snark_dmg_pop3","0"};

// Zombie
cvar_t	sk_zombie_health1 = {"sk_zombie_health1","0"};
cvar_t	sk_zombie_health2 = {"sk_zombie_health2","0"};
cvar_t	sk_zombie_health3 = {"sk_zombie_health3","0"};

cvar_t	sk_zombie_dmg_one_slash1 = {"sk_zombie_dmg_one_slash1","0"};
cvar_t	sk_zombie_dmg_one_slash2 = {"sk_zombie_dmg_one_slash2","0"};
cvar_t	sk_zombie_dmg_one_slash3 = {"sk_zombie_dmg_one_slash3","0"};

cvar_t	sk_zombie_dmg_both_slash1 = {"sk_zombie_dmg_both_slash1","0"};
cvar_t	sk_zombie_dmg_both_slash2 = {"sk_zombie_dmg_both_slash2","0"};
cvar_t	sk_zombie_dmg_both_slash3 = {"sk_zombie_dmg_both_slash3","0"};

//Turret
cvar_t	sk_turret_health1 = {"sk_turret_health1","0"};
cvar_t	sk_turret_health2 = {"sk_turret_health2","0"};
cvar_t	sk_turret_health3 = {"sk_turret_health3","0"};

// MiniTurret
cvar_t	sk_miniturret_health1 = {"sk_miniturret_health1","0"};
cvar_t	sk_miniturret_health2 = {"sk_miniturret_health2","0"};
cvar_t	sk_miniturret_health3 = {"sk_miniturret_health3","0"};

// Sentry Turret
cvar_t	sk_sentry_health1 = {"sk_sentry_health1","0"};
cvar_t	sk_sentry_health2 = {"sk_sentry_health2","0"};
cvar_t	sk_sentry_health3 = {"sk_sentry_health3","0"};

// PLAYER WEAPONS

// Crowbar whack
cvar_t	sk_plr_crowbar1 = {"sk_plr_crowbar1","0"};
cvar_t	sk_plr_crowbar2 = {"sk_plr_crowbar2","0"};
cvar_t	sk_plr_crowbar3 = {"sk_plr_crowbar3","0"};

// Glock Round
cvar_t	sk_plr_9mm_bullet1 = {"sk_plr_9mm_bullet1","0"};
cvar_t	sk_plr_9mm_bullet2 = {"sk_plr_9mm_bullet2","0"};
cvar_t	sk_plr_9mm_bullet3 = {"sk_plr_9mm_bullet3","0"};

// 357 Round
cvar_t	sk_plr_357_bullet1 = {"sk_plr_357_bullet1","0"};
cvar_t	sk_plr_357_bullet2 = {"sk_plr_357_bullet2","0"};
cvar_t	sk_plr_357_bullet3 = {"sk_plr_357_bullet3","0"};

// MP5 Round
cvar_t	sk_plr_9mmAR_bullet1 = {"sk_plr_9mmAR_bullet1","0"};
cvar_t	sk_plr_9mmAR_bullet2 = {"sk_plr_9mmAR_bullet2","0"};
cvar_t	sk_plr_9mmAR_bullet3 = {"sk_plr_9mmAR_bullet3","0"};


// M203 grenade
cvar_t	sk_plr_9mmAR_grenade1 = {"sk_plr_9mmAR_grenade1","0"};
cvar_t	sk_plr_9mmAR_grenade2 = {"sk_plr_9mmAR_grenade2","0"};
cvar_t	sk_plr_9mmAR_grenade3 = {"sk_plr_9mmAR_grenade3","0"};


// Shotgun buckshot
cvar_t	sk_plr_buckshot1 = {"sk_plr_buckshot1","0"};
cvar_t	sk_plr_buckshot2 = {"sk_plr_buckshot2","0"};
cvar_t	sk_plr_buckshot3 = {"sk_plr_buckshot3","0"};


// Crossbow
cvar_t	sk_plr_xbow_bolt_client1 = {"sk_plr_xbow_bolt_client1","0"};
cvar_t	sk_plr_xbow_bolt_client2 = {"sk_plr_xbow_bolt_client2","0"};
cvar_t	sk_plr_xbow_bolt_client3 = {"sk_plr_xbow_bolt_client3","0"};

cvar_t	sk_plr_xbow_bolt_monster1 = {"sk_plr_xbow_bolt_monster1","0"};
cvar_t	sk_plr_xbow_bolt_monster2 = {"sk_plr_xbow_bolt_monster2","0"};
cvar_t	sk_plr_xbow_bolt_monster3 = {"sk_plr_xbow_bolt_monster3","0"};


// RPG
cvar_t	sk_plr_rpg1 = {"sk_plr_rpg1","0"};
cvar_t	sk_plr_rpg2 = {"sk_plr_rpg2","0"};
cvar_t	sk_plr_rpg3 = {"sk_plr_rpg3","0"};


// Zero Point Generator
cvar_t	sk_plr_gauss1 = {"sk_plr_gauss1","0"};
cvar_t	sk_plr_gauss2 = {"sk_plr_gauss2","0"};
cvar_t	sk_plr_gauss3 = {"sk_plr_gauss3","0"};


// Tau Cannon
cvar_t	sk_plr_egon_narrow1 = {"sk_plr_egon_narrow1","0"};
cvar_t	sk_plr_egon_narrow2 = {"sk_plr_egon_narrow2","0"};
cvar_t	sk_plr_egon_narrow3 = {"sk_plr_egon_narrow3","0"};

cvar_t	sk_plr_egon_wide1 = {"sk_plr_egon_wide1","0"};
cvar_t	sk_plr_egon_wide2 = {"sk_plr_egon_wide2","0"};
cvar_t	sk_plr_egon_wide3 = {"sk_plr_egon_wide3","0"};


// Hand Grendade
cvar_t	sk_plr_hand_grenade1 = {"sk_plr_hand_grenade1","0"};
cvar_t	sk_plr_hand_grenade2 = {"sk_plr_hand_grenade2","0"};
cvar_t	sk_plr_hand_grenade3 = {"sk_plr_hand_grenade3","0"};


// Satchel Charge
cvar_t	sk_plr_satchel1	= {"sk_plr_satchel1","0"};
cvar_t	sk_plr_satchel2	= {"sk_plr_satchel2","0"};
cvar_t	sk_plr_satchel3	= {"sk_plr_satchel3","0"};


// Tripmine
cvar_t	sk_plr_tripmine1 = {"sk_plr_tripmine1","0"};
cvar_t	sk_plr_tripmine2 = {"sk_plr_tripmine2","0"};
cvar_t	sk_plr_tripmine3 = {"sk_plr_tripmine3","0"};


// WORLD WEAPONS
cvar_t	sk_12mm_bullet1 = {"sk_12mm_bullet1","0"};
cvar_t	sk_12mm_bullet2 = {"sk_12mm_bullet2","0"};
cvar_t	sk_12mm_bullet3 = {"sk_12mm_bullet3","0"};

cvar_t	sk_9mmAR_bullet1 = {"sk_9mmAR_bullet1","0"};
cvar_t	sk_9mmAR_bullet2 = {"sk_9mmAR_bullet2","0"};
cvar_t	sk_9mmAR_bullet3 = {"sk_9mmAR_bullet3","0"};

cvar_t	sk_9mm_bullet1 = {"sk_9mm_bullet1","0"};
cvar_t	sk_9mm_bullet2 = {"sk_9mm_bullet2","0"};
cvar_t	sk_9mm_bullet3 = {"sk_9mm_bullet3","0"};


// HORNET
cvar_t	sk_hornet_dmg1 = {"sk_hornet_dmg1","0"};
cvar_t	sk_hornet_dmg2 = {"sk_hornet_dmg2","0"};
cvar_t	sk_hornet_dmg3 = {"sk_hornet_dmg3","0"};

// HEALTH/CHARGE
cvar_t	sk_suitcharger1	= { "sk_suitcharger1","0" };
cvar_t	sk_suitcharger2	= { "sk_suitcharger2","0" };		
cvar_t	sk_suitcharger3	= { "sk_suitcharger3","0" };		

cvar_t	sk_battery1	= { "sk_battery1","0" };			
cvar_t	sk_battery2	= { "sk_battery2","0" };			
cvar_t	sk_battery3	= { "sk_battery3","0" };			

cvar_t	sk_healthcharger1	= { "sk_healthcharger1","0" };		
cvar_t	sk_healthcharger2	= { "sk_healthcharger2","0" };		
cvar_t	sk_healthcharger3	= { "sk_healthcharger3","0" };		

cvar_t	sk_healthkit1	= { "sk_healthkit1","0" };		
cvar_t	sk_healthkit2	= { "sk_healthkit2","0" };		
cvar_t	sk_healthkit3	= { "sk_healthkit3","0" };		

cvar_t	sk_scientist_heal1	= { "sk_scientist_heal1","0" };	
cvar_t	sk_scientist_heal2	= { "sk_scientist_heal2","0" };	
cvar_t	sk_scientist_heal3	= { "sk_scientist_heal3","0" };	


// monster damage adjusters
cvar_t	sk_monster_head1	= { "sk_monster_head1","2" };
cvar_t	sk_monster_head2	= { "sk_monster_head2","2" };
cvar_t	sk_monster_head3	= { "sk_monster_head3","2" };

cvar_t	sk_monster_chest1	= { "sk_monster_chest1","1" };
cvar_t	sk_monster_chest2	= { "sk_monster_chest2","1" };
cvar_t	sk_monster_chest3	= { "sk_monster_chest3","1" };

cvar_t	sk_monster_stomach1	= { "sk_monster_stomach1","1" };
cvar_t	sk_monster_stomach2	= { "sk_monster_stomach2","1" };
cvar_t	sk_monster_stomach3	= { "sk_monster_stomach3","1" };

cvar_t	sk_monster_arm1	= { "sk_monster_arm1","1" };
cvar_t	sk_monster_arm2	= { "sk_monster_arm2","1" };
cvar_t	sk_monster_arm3	= { "sk_monster_arm3","1" };

cvar_t	sk_monster_leg1	= { "sk_monster_leg1","1" };
cvar_t	sk_monster_leg2	= { "sk_monster_leg2","1" };
cvar_t	sk_monster_leg3	= { "sk_monster_leg3","1" };

// player damage adjusters
cvar_t	sk_player_head1	= { "sk_player_head1","2" };
cvar_t	sk_player_head2	= { "sk_player_head2","2" };
cvar_t	sk_player_head3	= { "sk_player_head3","2" };

cvar_t	sk_player_chest1 = { "sk_player_chest1","1" };
cvar_t	sk_player_chest2 = { "sk_player_chest2","1" };
cvar_t	sk_player_chest3 = { "sk_player_chest3","1" };

cvar_t	sk_player_stomach1 = { "sk_player_stomach1","1" };
cvar_t	sk_player_stomach2 = { "sk_player_stomach2","1" };
cvar_t	sk_player_stomach3 = { "sk_player_stomach3","1" };

cvar_t	sk_player_arm1	= { "sk_player_arm1","1" };
cvar_t	sk_player_arm2	= { "sk_player_arm2","1" };
cvar_t	sk_player_arm3	= { "sk_player_arm3","1" };

cvar_t	sk_player_leg1	= { "sk_player_leg1","1" };
cvar_t	sk_player_leg2	= { "sk_player_leg2","1" };
cvar_t	sk_player_leg3	= { "sk_player_leg3","1" };

// END Cvars for Skill Level settings

// BMOD Begin - CVARs
cvar_t  bm_ver = { "bm_ver", "", FCVAR_SERVER | FCVAR_UNLOGGED };
cvar_t  bm_url = { "bm_url", "", FCVAR_SERVER | FCVAR_UNLOGGED };

cvar_t  bm_bver = { "bm_bver", "", FCVAR_SERVER | FCVAR_UNLOGGED };
cvar_t  bm_bname = { "bm_bname", "", FCVAR_SERVER | FCVAR_UNLOGGED };
cvar_t  bm_burl = { "bm_burl", "", FCVAR_SERVER | FCVAR_UNLOGGED };

cvar_t  bm_guns = { "bm_guns", "crowbar;357;9mmhandgun" };
cvar_t  bm_ammo = { "bm_ammo", "9mmclip;9mmclip;357;357" };		
cvar_t  bm_g = { "bm_g", "7", FCVAR_SERVER | FCVAR_UNLOGGED };
cvar_t  bm_dmg_messages = { "bm_dmg_messages", "1" };
cvar_t  bm_matchkills = { "bm_matchkills", "1" };

cvar_t  bm_freezetime = { "bm_freezetime", "4.5", FCVAR_SERVER };
cvar_t  bm_thrust = { "bm_thrust", "0" };
cvar_t  bm_spawneffects = { "bm_spawneffects", "1" };
//cvar_t  bm_snarktrails = { "bm_snarktrails", "1" };
cvar_t  bm_snarktrails = { "bm_snarktrails", "0AE00AC8" };
cvar_t  bm_xbowtracers = { "bm_xbowtracers", "1" };

cvar_t  bm_spawnkilltime = { "bm_spawnkilltime", "10", FCVAR_SERVER };
cvar_t  bm_maxspawnkills = { "bm_maxspawnkills", "3" };
cvar_t  bm_typekills = { "bm_typekills", "0", FCVAR_SERVER };
cvar_t  bm_maxtypekills = { "bm_maxtypekills", "3" };
cvar_t  bm_typecam = { "bm_typecam", "1" };

cvar_t  bm_bantime = { "bm_bantime", "30" };

cvar_t  bm_spamlimit = { "bm_spamlimit", "4" };
cvar_t  bm_antispam = { "bm_antispam", "1" };

cvar_t  bm_spawnmines = { "bm_spawnmines", "0", FCVAR_SERVER };
cvar_t  bm_spawnsatchels = { "bm_spawnsatchels", "0", FCVAR_SERVER };
cvar_t  bm_voting = { "bm_voting", "1", FCVAR_SERVER };
cvar_t  bm_votetime = { "bm_votetime", "180", FCVAR_SERVER };
cvar_t  bm_maxtime = { "bm_maxtime", "120" };
cvar_t  bm_maxfrags = { "bm_maxfrags", "100" };

cvar_t  bm_mods = { "bm_mods", "", FCVAR_SERVER };
cvar_t  bm_rpg_mod = { "bm_rpg_mod", "1" };
cvar_t  bm_shotty_mod = { "bm_shotty_mod", "0" };
cvar_t  bm_xbow_mod = { "bm_xbow_mod", "1" };
cvar_t  bm_mp5_mod = { "bm_mp5_mod", "1" };
cvar_t  bm_cbar_mod = { "bm_cbar_mod", "1" };
cvar_t  bm_tau_mod = { "bm_tau_mod", "1" };
cvar_t  bm_snarks_mod = { "bm_snarks_mod", "1" };
cvar_t  bm_gluon_mod = { "bm_gluon_mod", "1" };
cvar_t  bm_hornet_mod = { "bm_hornet_mod", "1" };
cvar_t  bm_trip_mod = { "bm_trip_mod", "1" };
/*
cvar_t	bm_score_crowbar = { "bm_score_crowbar", "1" };
cvar_t	bm_score_throwncbar = { "bm_score_throwncbar", "1" };
cvar_t	bm_score_9mm = { "bm_score_9mm", "1" };
cvar_t	bm_score_357 = { "bm_score_357", "1" };
cvar_t	bm_score_mp5 = { "bm_score_mp5", "1" };
cvar_t	bm_score_shotgun = { "bm_score_shotgun", "1" };
cvar_t	bm_score_squidspit = { "bm_score_squidspit", "1" };
cvar_t	bm_score_zapgun = { "bm_score_zapgun", "1" };
cvar_t	bm_score_mp5grenade = { "bm_score_mp5grenade", "1" };
cvar_t	bm_score_gluon = { "bm_score_gluon", "1" };
cvar_t	bm_score_tau = { "bm_score_tau", "1" };
cvar_t	bm_score_bolt = { "bm_score_bolt", "1" };
cvar_t	bm_score_crossbow = { "bm_score_crossbow", "1" };
cvar_t	bm_score_satchel = { "bm_score_satchel", "1" };
cvar_t	bm_score_handgrenade = { "bm_score_handgrenade", "1" };
cvar_t	bm_score_rpg = { "bm_score_rpg", "1" };
cvar_t	bm_score_snarks = { "bm_score_snarks", "1" };
cvar_t	bm_score_tripmine = { "bm_score_tripmine", "1" };
*/
cvar_t  bm_map = { "bm_map", "" };
cvar_t  bm_nextmap = { "bm_nextmap", "", FCVAR_SERVER | FCVAR_UNLOGGED };

cvar_t  bm_rune_rand = { "bm_rune_rand", "0", FCVAR_SERVER };
cvar_t  bm_runemask = { "bm_runemask", "63", FCVAR_SERVER };

cvar_t  bm_rune_cbar = { "bm_rune_cbar", "1", FCVAR_SERVER };
cvar_t  bm_rune_cbar_t = { "bm_rune_cbar_t", "30" };
cvar_t  bm_rune_cbar_r = { "bm_rune_cbar_r", "60" };
cvar_t  bm_rune_gren = { "bm_rune_gren", "1", FCVAR_SERVER };
cvar_t  bm_rune_gren_t = { "bm_rune_gren_t", "30" };
cvar_t  bm_rune_gren_r = { "bm_rune_gren_r", "120" };
cvar_t  bm_rune_357 = { "bm_rune_357", "1", FCVAR_SERVER };
cvar_t  bm_rune_357_t = { "bm_rune_357_t", "30" };
cvar_t  bm_rune_357_r = { "bm_rune_357_r", "60" };
cvar_t  bm_rune_health = { "bm_rune_health", "1", FCVAR_SERVER };
cvar_t  bm_rune_health_t = { "bm_rune_health_t", "30" };
cvar_t  bm_rune_health_r = { "bm_rune_health_r", "60" };
cvar_t  bm_rune_armor = { "bm_rune_armor", "1", FCVAR_SERVER };
cvar_t  bm_rune_armor_t = { "bm_rune_armor_t", "30" };
cvar_t  bm_rune_armor_r = { "bm_rune_armor_r", "60" };
cvar_t  bm_rune_shotty = { "bm_rune_shotty", "1", FCVAR_SERVER };
cvar_t  bm_rune_shotty_t = { "bm_rune_shotty_t", "30" };
cvar_t  bm_rune_shotty_r = { "bm_rune_shotty_r", "180" };
// BMOD End - CVARs

// Register your console variables here
// This gets called one time when the game is initialied
void GameDLLInit( void )
{
	// Register cvars here:
	if( CVAR_GET_POINTER( "build" ) )
		g_fIsXash3D = TRUE;

	g_psv_gravity = CVAR_GET_POINTER( "sv_gravity" );
	g_psv_aim = CVAR_GET_POINTER( "sv_aim" );
	g_footsteps = CVAR_GET_POINTER( "mp_footsteps" );

	g_psv_developer = CVAR_GET_POINTER( "developer" );

	g_enable_cheats = CVAR_GET_POINTER( "sv_cheats" );

	CVAR_REGISTER( &displaysoundlist );
	CVAR_REGISTER( &allow_spectators );

	CVAR_REGISTER( &teamplay );
	CVAR_REGISTER( &fraglimit );
	CVAR_REGISTER( &timelimit );

	CVAR_REGISTER( &fragsleft );
	CVAR_REGISTER( &timeleft );

	CVAR_REGISTER( &friendlyfire );
	CVAR_REGISTER( &falldamage );
	CVAR_REGISTER( &weaponstay );
	CVAR_REGISTER( &selfgauss );
	CVAR_REGISTER( &chargerfix );
	CVAR_REGISTER( &satchelfix );
	CVAR_REGISTER( &explosionfix );
	CVAR_REGISTER( &monsteryawspeedfix );
	CVAR_REGISTER( &corpsephysics );
	CVAR_REGISTER( &pushablemode );
	CVAR_REGISTER( &forcerespawn );
	CVAR_REGISTER( &flashlight );
	CVAR_REGISTER( &aimcrosshair );
	CVAR_REGISTER( &decalfrequency );
	CVAR_REGISTER( &teamlist );
	CVAR_REGISTER( &teamoverride );
	CVAR_REGISTER( &defaultteam );
	CVAR_REGISTER( &allowmonsters );
	CVAR_REGISTER( &bhopcap );
	CVAR_REGISTER( &multibyte_only );

	CVAR_REGISTER( &mp_chattime );



// REGISTER CVARS FOR SKILL LEVEL STUFF
	// Agrunt
	CVAR_REGISTER( &sk_agrunt_health1 );// {"sk_agrunt_health1","0"};
	CVAR_REGISTER( &sk_agrunt_health2 );// {"sk_agrunt_health2","0"};
	CVAR_REGISTER( &sk_agrunt_health3 );// {"sk_agrunt_health3","0"};

	CVAR_REGISTER( &sk_agrunt_dmg_punch1 );// {"sk_agrunt_dmg_punch1","0"};
	CVAR_REGISTER( &sk_agrunt_dmg_punch2 );// {"sk_agrunt_dmg_punch2","0"};
	CVAR_REGISTER( &sk_agrunt_dmg_punch3 );// {"sk_agrunt_dmg_punch3","0"};

	// Apache
	CVAR_REGISTER( &sk_apache_health1 );// {"sk_apache_health1","0"};
	CVAR_REGISTER( &sk_apache_health2 );// {"sk_apache_health2","0"};
	CVAR_REGISTER( &sk_apache_health3 );// {"sk_apache_health3","0"};

	// Barney
	CVAR_REGISTER( &sk_barney_health1 );// {"sk_barney_health1","0"};
	CVAR_REGISTER( &sk_barney_health2 );// {"sk_barney_health2","0"};
	CVAR_REGISTER( &sk_barney_health3 );// {"sk_barney_health3","0"};

	// Bullsquid
	CVAR_REGISTER( &sk_bullsquid_health1 );// {"sk_bullsquid_health1","0"};
	CVAR_REGISTER( &sk_bullsquid_health2 );// {"sk_bullsquid_health2","0"};
	CVAR_REGISTER( &sk_bullsquid_health3 );// {"sk_bullsquid_health3","0"};

	CVAR_REGISTER( &sk_bullsquid_dmg_bite1 );// {"sk_bullsquid_dmg_bite1","0"};
	CVAR_REGISTER( &sk_bullsquid_dmg_bite2 );// {"sk_bullsquid_dmg_bite2","0"};
	CVAR_REGISTER( &sk_bullsquid_dmg_bite3 );// {"sk_bullsquid_dmg_bite3","0"};

	CVAR_REGISTER( &sk_bullsquid_dmg_whip1 );// {"sk_bullsquid_dmg_whip1","0"};
	CVAR_REGISTER( &sk_bullsquid_dmg_whip2 );// {"sk_bullsquid_dmg_whip2","0"};
	CVAR_REGISTER( &sk_bullsquid_dmg_whip3 );// {"sk_bullsquid_dmg_whip3","0"};

	CVAR_REGISTER( &sk_bullsquid_dmg_spit1 );// {"sk_bullsquid_dmg_spit1","0"};
	CVAR_REGISTER( &sk_bullsquid_dmg_spit2 );// {"sk_bullsquid_dmg_spit2","0"};
	CVAR_REGISTER( &sk_bullsquid_dmg_spit3 );// {"sk_bullsquid_dmg_spit3","0"};

	CVAR_REGISTER( &sk_bigmomma_health_factor1 );// {"sk_bigmomma_health_factor1","1.0"};
	CVAR_REGISTER( &sk_bigmomma_health_factor2 );// {"sk_bigmomma_health_factor2","1.0"};
	CVAR_REGISTER( &sk_bigmomma_health_factor3 );// {"sk_bigmomma_health_factor3","1.0"};

	CVAR_REGISTER( &sk_bigmomma_dmg_slash1 );// {"sk_bigmomma_dmg_slash1","50"};
	CVAR_REGISTER( &sk_bigmomma_dmg_slash2 );// {"sk_bigmomma_dmg_slash2","50"};
	CVAR_REGISTER( &sk_bigmomma_dmg_slash3 );// {"sk_bigmomma_dmg_slash3","50"};

	CVAR_REGISTER( &sk_bigmomma_dmg_blast1 );// {"sk_bigmomma_dmg_blast1","100"};
	CVAR_REGISTER( &sk_bigmomma_dmg_blast2 );// {"sk_bigmomma_dmg_blast2","100"};
	CVAR_REGISTER( &sk_bigmomma_dmg_blast3 );// {"sk_bigmomma_dmg_blast3","100"};

	CVAR_REGISTER( &sk_bigmomma_radius_blast1 );// {"sk_bigmomma_radius_blast1","250"};
	CVAR_REGISTER( &sk_bigmomma_radius_blast2 );// {"sk_bigmomma_radius_blast2","250"};
	CVAR_REGISTER( &sk_bigmomma_radius_blast3 );// {"sk_bigmomma_radius_blast3","250"};

	// Gargantua
	CVAR_REGISTER( &sk_gargantua_health1 );// {"sk_gargantua_health1","0"};
	CVAR_REGISTER( &sk_gargantua_health2 );// {"sk_gargantua_health2","0"};
	CVAR_REGISTER( &sk_gargantua_health3 );// {"sk_gargantua_health3","0"};

	CVAR_REGISTER( &sk_gargantua_dmg_slash1 );// {"sk_gargantua_dmg_slash1","0"};
	CVAR_REGISTER( &sk_gargantua_dmg_slash2 );// {"sk_gargantua_dmg_slash2","0"};
	CVAR_REGISTER( &sk_gargantua_dmg_slash3 );// {"sk_gargantua_dmg_slash3","0"};

	CVAR_REGISTER( &sk_gargantua_dmg_fire1 );// {"sk_gargantua_dmg_fire1","0"};
	CVAR_REGISTER( &sk_gargantua_dmg_fire2 );// {"sk_gargantua_dmg_fire2","0"};
	CVAR_REGISTER( &sk_gargantua_dmg_fire3 );// {"sk_gargantua_dmg_fire3","0"};

	CVAR_REGISTER( &sk_gargantua_dmg_stomp1 );// {"sk_gargantua_dmg_stomp1","0"};
	CVAR_REGISTER( &sk_gargantua_dmg_stomp2 );// {"sk_gargantua_dmg_stomp2","0"};
	CVAR_REGISTER( &sk_gargantua_dmg_stomp3	);// {"sk_gargantua_dmg_stomp3","0"};

	// Hassassin
	CVAR_REGISTER( &sk_hassassin_health1 );// {"sk_hassassin_health1","0"};
	CVAR_REGISTER( &sk_hassassin_health2 );// {"sk_hassassin_health2","0"};
	CVAR_REGISTER( &sk_hassassin_health3 );// {"sk_hassassin_health3","0"};

	// Headcrab
	CVAR_REGISTER( &sk_headcrab_health1 );// {"sk_headcrab_health1","0"};
	CVAR_REGISTER( &sk_headcrab_health2 );// {"sk_headcrab_health2","0"};
	CVAR_REGISTER( &sk_headcrab_health3 );// {"sk_headcrab_health3","0"};

	CVAR_REGISTER( &sk_headcrab_dmg_bite1 );// {"sk_headcrab_dmg_bite1","0"};
	CVAR_REGISTER( &sk_headcrab_dmg_bite2 );// {"sk_headcrab_dmg_bite2","0"};
	CVAR_REGISTER( &sk_headcrab_dmg_bite3 );// {"sk_headcrab_dmg_bite3","0"};

	// Hgrunt
	CVAR_REGISTER( &sk_hgrunt_health1 );// {"sk_hgrunt_health1","0"};
	CVAR_REGISTER( &sk_hgrunt_health2 );// {"sk_hgrunt_health2","0"};
	CVAR_REGISTER( &sk_hgrunt_health3 );// {"sk_hgrunt_health3","0"};

	CVAR_REGISTER( &sk_hgrunt_kick1 );// {"sk_hgrunt_kick1","0"};
	CVAR_REGISTER( &sk_hgrunt_kick2 );// {"sk_hgrunt_kick2","0"};
	CVAR_REGISTER( &sk_hgrunt_kick3 );// {"sk_hgrunt_kick3","0"};

	CVAR_REGISTER( &sk_hgrunt_pellets1 );
	CVAR_REGISTER( &sk_hgrunt_pellets2 );
	CVAR_REGISTER( &sk_hgrunt_pellets3 );

	CVAR_REGISTER( &sk_hgrunt_gspeed1 );
	CVAR_REGISTER( &sk_hgrunt_gspeed2 );
	CVAR_REGISTER( &sk_hgrunt_gspeed3 );

	// Houndeye
	CVAR_REGISTER( &sk_houndeye_health1 );// {"sk_houndeye_health1","0"};
	CVAR_REGISTER( &sk_houndeye_health2 );// {"sk_houndeye_health2","0"};
	CVAR_REGISTER( &sk_houndeye_health3 );// {"sk_houndeye_health3","0"};

	CVAR_REGISTER( &sk_houndeye_dmg_blast1 );// {"sk_houndeye_dmg_blast1","0"};
	CVAR_REGISTER( &sk_houndeye_dmg_blast2 );// {"sk_houndeye_dmg_blast2","0"};
	CVAR_REGISTER( &sk_houndeye_dmg_blast3 );// {"sk_houndeye_dmg_blast3","0"};

	// ISlave
	CVAR_REGISTER( &sk_islave_health1 );// {"sk_islave_health1","0"};
	CVAR_REGISTER( &sk_islave_health2 );// {"sk_islave_health2","0"};
	CVAR_REGISTER( &sk_islave_health3 );// {"sk_islave_health3","0"};

	CVAR_REGISTER( &sk_islave_dmg_claw1 );// {"sk_islave_dmg_claw1","0"};
	CVAR_REGISTER( &sk_islave_dmg_claw2 );// {"sk_islave_dmg_claw2","0"};
	CVAR_REGISTER( &sk_islave_dmg_claw3 );// {"sk_islave_dmg_claw3","0"};

	CVAR_REGISTER( &sk_islave_dmg_clawrake1 );// {"sk_islave_dmg_clawrake1","0"};
	CVAR_REGISTER( &sk_islave_dmg_clawrake2 );// {"sk_islave_dmg_clawrake2","0"};
	CVAR_REGISTER( &sk_islave_dmg_clawrake3 );// {"sk_islave_dmg_clawrake3","0"};

	CVAR_REGISTER( &sk_islave_dmg_zap1 );// {"sk_islave_dmg_zap1","0"};
	CVAR_REGISTER( &sk_islave_dmg_zap2 );// {"sk_islave_dmg_zap2","0"};
	CVAR_REGISTER( &sk_islave_dmg_zap3 );// {"sk_islave_dmg_zap3","0"};

	// Icthyosaur
	CVAR_REGISTER( &sk_ichthyosaur_health1 );// {"sk_ichthyosaur_health1","0"};
	CVAR_REGISTER( &sk_ichthyosaur_health2 );// {"sk_ichthyosaur_health2","0"};
	CVAR_REGISTER( &sk_ichthyosaur_health3 );// {"sk_ichthyosaur_health3","0"};

	CVAR_REGISTER( &sk_ichthyosaur_shake1 );// {"sk_ichthyosaur_health3","0"};
	CVAR_REGISTER( &sk_ichthyosaur_shake2 );// {"sk_ichthyosaur_health3","0"};
	CVAR_REGISTER( &sk_ichthyosaur_shake3 );// {"sk_ichthyosaur_health3","0"};

	// Leech
	CVAR_REGISTER( &sk_leech_health1 );// {"sk_leech_health1","0"};
	CVAR_REGISTER( &sk_leech_health2 );// {"sk_leech_health2","0"};
	CVAR_REGISTER( &sk_leech_health3 );// {"sk_leech_health3","0"};

	CVAR_REGISTER( &sk_leech_dmg_bite1 );// {"sk_leech_dmg_bite1","0"};
	CVAR_REGISTER( &sk_leech_dmg_bite2 );// {"sk_leech_dmg_bite2","0"};
	CVAR_REGISTER( &sk_leech_dmg_bite3 );// {"sk_leech_dmg_bite3","0"};

	// Controller
	CVAR_REGISTER( &sk_controller_health1 );
	CVAR_REGISTER( &sk_controller_health2 );
	CVAR_REGISTER( &sk_controller_health3 );

	CVAR_REGISTER( &sk_controller_dmgzap1 );
	CVAR_REGISTER( &sk_controller_dmgzap2 );
	CVAR_REGISTER( &sk_controller_dmgzap3 );

	CVAR_REGISTER( &sk_controller_speedball1 );
	CVAR_REGISTER( &sk_controller_speedball2 );
	CVAR_REGISTER( &sk_controller_speedball3 );

	CVAR_REGISTER( &sk_controller_dmgball1 );
	CVAR_REGISTER( &sk_controller_dmgball2 );
	CVAR_REGISTER( &sk_controller_dmgball3 );

	// Nihilanth
	CVAR_REGISTER( &sk_nihilanth_health1 );// {"sk_nihilanth_health1","0"};
	CVAR_REGISTER( &sk_nihilanth_health2 );// {"sk_nihilanth_health2","0"};
	CVAR_REGISTER( &sk_nihilanth_health3 );// {"sk_nihilanth_health3","0"};

	CVAR_REGISTER( &sk_nihilanth_zap1 );
	CVAR_REGISTER( &sk_nihilanth_zap2 );
	CVAR_REGISTER( &sk_nihilanth_zap3 );

	// Scientist
	CVAR_REGISTER( &sk_scientist_health1 );// {"sk_scientist_health1","0"};
	CVAR_REGISTER( &sk_scientist_health2 );// {"sk_scientist_health2","0"};
	CVAR_REGISTER( &sk_scientist_health3 );// {"sk_scientist_health3","0"};

	// Snark
	CVAR_REGISTER( &sk_snark_health1 );// {"sk_snark_health1","0"};
	CVAR_REGISTER( &sk_snark_health2 );// {"sk_snark_health2","0"};
	CVAR_REGISTER( &sk_snark_health3 );// {"sk_snark_health3","0"};

	CVAR_REGISTER( &sk_snark_dmg_bite1 );// {"sk_snark_dmg_bite1","0"};
	CVAR_REGISTER( &sk_snark_dmg_bite2 );// {"sk_snark_dmg_bite2","0"};
	CVAR_REGISTER( &sk_snark_dmg_bite3 );// {"sk_snark_dmg_bite3","0"};

	CVAR_REGISTER( &sk_snark_dmg_pop1 );// {"sk_snark_dmg_pop1","0"};
	CVAR_REGISTER( &sk_snark_dmg_pop2 );// {"sk_snark_dmg_pop2","0"};
	CVAR_REGISTER( &sk_snark_dmg_pop3 );// {"sk_snark_dmg_pop3","0"};

	// Zombie
	CVAR_REGISTER( &sk_zombie_health1 );// {"sk_zombie_health1","0"};
	CVAR_REGISTER( &sk_zombie_health2 );// {"sk_zombie_health3","0"};
	CVAR_REGISTER( &sk_zombie_health3 );// {"sk_zombie_health3","0"};

	CVAR_REGISTER( &sk_zombie_dmg_one_slash1 );// {"sk_zombie_dmg_one_slash1","0"};
	CVAR_REGISTER( &sk_zombie_dmg_one_slash2 );// {"sk_zombie_dmg_one_slash2","0"};
	CVAR_REGISTER( &sk_zombie_dmg_one_slash3 );// {"sk_zombie_dmg_one_slash3","0"};

	CVAR_REGISTER( &sk_zombie_dmg_both_slash1 );// {"sk_zombie_dmg_both_slash1","0"};
	CVAR_REGISTER( &sk_zombie_dmg_both_slash2 );// {"sk_zombie_dmg_both_slash2","0"};
	CVAR_REGISTER( &sk_zombie_dmg_both_slash3 );// {"sk_zombie_dmg_both_slash3","0"};

	//Turret
	CVAR_REGISTER( &sk_turret_health1 );// {"sk_turret_health1","0"};
	CVAR_REGISTER( &sk_turret_health2 );// {"sk_turret_health2","0"};
	CVAR_REGISTER( &sk_turret_health3 );// {"sk_turret_health3","0"};

	// MiniTurret
	CVAR_REGISTER( &sk_miniturret_health1 );// {"sk_miniturret_health1","0"};
	CVAR_REGISTER( &sk_miniturret_health2 );// {"sk_miniturret_health2","0"};
	CVAR_REGISTER( &sk_miniturret_health3 );// {"sk_miniturret_health3","0"};

	// Sentry Turret
	CVAR_REGISTER( &sk_sentry_health1 );// {"sk_sentry_health1","0"};
	CVAR_REGISTER( &sk_sentry_health2 );// {"sk_sentry_health2","0"};
	CVAR_REGISTER( &sk_sentry_health3 );// {"sk_sentry_health3","0"};


	// PLAYER WEAPONS

	// Crowbar whack
	CVAR_REGISTER( &sk_plr_crowbar1 );// {"sk_plr_crowbar1","0"};
	CVAR_REGISTER( &sk_plr_crowbar2 );// {"sk_plr_crowbar2","0"};
	CVAR_REGISTER( &sk_plr_crowbar3 );// {"sk_plr_crowbar3","0"};

	// Glock Round
	CVAR_REGISTER( &sk_plr_9mm_bullet1 );// {"sk_plr_9mm_bullet1","0"};
	CVAR_REGISTER( &sk_plr_9mm_bullet2 );// {"sk_plr_9mm_bullet2","0"};
	CVAR_REGISTER( &sk_plr_9mm_bullet3 );// {"sk_plr_9mm_bullet3","0"};

	// 357 Round
	CVAR_REGISTER( &sk_plr_357_bullet1 );// {"sk_plr_357_bullet1","0"};
	CVAR_REGISTER( &sk_plr_357_bullet2 );// {"sk_plr_357_bullet2","0"};
	CVAR_REGISTER( &sk_plr_357_bullet3 );// {"sk_plr_357_bullet3","0"};

	// MP5 Round
	CVAR_REGISTER( &sk_plr_9mmAR_bullet1 );// {"sk_plr_9mmAR_bullet1","0"};
	CVAR_REGISTER( &sk_plr_9mmAR_bullet2 );// {"sk_plr_9mmAR_bullet2","0"};
	CVAR_REGISTER( &sk_plr_9mmAR_bullet3 );// {"sk_plr_9mmAR_bullet3","0"};

	// M203 grenade
	CVAR_REGISTER( &sk_plr_9mmAR_grenade1 );// {"sk_plr_9mmAR_grenade1","0"};
	CVAR_REGISTER( &sk_plr_9mmAR_grenade2 );// {"sk_plr_9mmAR_grenade2","0"};
	CVAR_REGISTER( &sk_plr_9mmAR_grenade3 );// {"sk_plr_9mmAR_grenade3","0"};

	// Shotgun buckshot
	CVAR_REGISTER( &sk_plr_buckshot1 );// {"sk_plr_buckshot1","0"};
	CVAR_REGISTER( &sk_plr_buckshot2 );// {"sk_plr_buckshot2","0"};
	CVAR_REGISTER( &sk_plr_buckshot3 );// {"sk_plr_buckshot3","0"};

	// Crossbow
	CVAR_REGISTER( &sk_plr_xbow_bolt_monster1 );// {"sk_plr_xbow_bolt1","0"};
	CVAR_REGISTER( &sk_plr_xbow_bolt_monster2 );// {"sk_plr_xbow_bolt2","0"};
	CVAR_REGISTER( &sk_plr_xbow_bolt_monster3 );// {"sk_plr_xbow_bolt3","0"};

	CVAR_REGISTER( &sk_plr_xbow_bolt_client1 );// {"sk_plr_xbow_bolt1","0"};
	CVAR_REGISTER( &sk_plr_xbow_bolt_client2 );// {"sk_plr_xbow_bolt2","0"};
	CVAR_REGISTER( &sk_plr_xbow_bolt_client3 );// {"sk_plr_xbow_bolt3","0"};

	// RPG
	CVAR_REGISTER( &sk_plr_rpg1 );// {"sk_plr_rpg1","0"};
	CVAR_REGISTER( &sk_plr_rpg2 );// {"sk_plr_rpg2","0"};
	CVAR_REGISTER( &sk_plr_rpg3 );// {"sk_plr_rpg3","0"};

	// Gauss Gun
	CVAR_REGISTER( &sk_plr_gauss1 );// {"sk_plr_gauss1","0"};
	CVAR_REGISTER( &sk_plr_gauss2 );// {"sk_plr_gauss2","0"};
	CVAR_REGISTER( &sk_plr_gauss3 );// {"sk_plr_gauss3","0"};

	// Egon Gun
	CVAR_REGISTER( &sk_plr_egon_narrow1 );// {"sk_plr_egon_narrow1","0"};
	CVAR_REGISTER( &sk_plr_egon_narrow2 );// {"sk_plr_egon_narrow2","0"};
	CVAR_REGISTER( &sk_plr_egon_narrow3 );// {"sk_plr_egon_narrow3","0"};

	CVAR_REGISTER( &sk_plr_egon_wide1 );// {"sk_plr_egon_wide1","0"};
	CVAR_REGISTER( &sk_plr_egon_wide2 );// {"sk_plr_egon_wide2","0"};
	CVAR_REGISTER( &sk_plr_egon_wide3 );// {"sk_plr_egon_wide3","0"};

	// Hand Grendade
	CVAR_REGISTER( &sk_plr_hand_grenade1 );// {"sk_plr_hand_grenade1","0"};
	CVAR_REGISTER( &sk_plr_hand_grenade2 );// {"sk_plr_hand_grenade2","0"};
	CVAR_REGISTER( &sk_plr_hand_grenade3 );// {"sk_plr_hand_grenade3","0"};

	// Satchel Charge
	CVAR_REGISTER( &sk_plr_satchel1 );// {"sk_plr_satchel1","0"};
	CVAR_REGISTER( &sk_plr_satchel2 );// {"sk_plr_satchel2","0"};
	CVAR_REGISTER( &sk_plr_satchel3 );// {"sk_plr_satchel3","0"};

	// Tripmine
	CVAR_REGISTER( &sk_plr_tripmine1 );// {"sk_plr_tripmine1","0"};
	CVAR_REGISTER( &sk_plr_tripmine2 );// {"sk_plr_tripmine2","0"};
	CVAR_REGISTER( &sk_plr_tripmine3 );// {"sk_plr_tripmine3","0"};

	// WORLD WEAPONS
	CVAR_REGISTER( &sk_12mm_bullet1 );// {"sk_12mm_bullet1","0"};
	CVAR_REGISTER( &sk_12mm_bullet2 );// {"sk_12mm_bullet2","0"};
	CVAR_REGISTER( &sk_12mm_bullet3 );// {"sk_12mm_bullet3","0"};

	CVAR_REGISTER( &sk_9mmAR_bullet1 );// {"sk_9mm_bullet1","0"};
	CVAR_REGISTER( &sk_9mmAR_bullet2 );// {"sk_9mm_bullet1","0"};
	CVAR_REGISTER( &sk_9mmAR_bullet3 );// {"sk_9mm_bullet1","0"};

	CVAR_REGISTER( &sk_9mm_bullet1 );// {"sk_9mm_bullet1","0"};
	CVAR_REGISTER( &sk_9mm_bullet2 );// {"sk_9mm_bullet2","0"};
	CVAR_REGISTER( &sk_9mm_bullet3 );// {"sk_9mm_bullet3","0"};

	// HORNET
	CVAR_REGISTER( &sk_hornet_dmg1 );// {"sk_hornet_dmg1","0"};
	CVAR_REGISTER( &sk_hornet_dmg2 );// {"sk_hornet_dmg2","0"};
	CVAR_REGISTER( &sk_hornet_dmg3 );// {"sk_hornet_dmg3","0"};

	// HEALTH/SUIT CHARGE DISTRIBUTION
	CVAR_REGISTER( &sk_suitcharger1 );
	CVAR_REGISTER( &sk_suitcharger2 );
	CVAR_REGISTER( &sk_suitcharger3 );

	CVAR_REGISTER( &sk_battery1 );
	CVAR_REGISTER( &sk_battery2 );
	CVAR_REGISTER( &sk_battery3 );

	CVAR_REGISTER( &sk_healthcharger1 );
	CVAR_REGISTER( &sk_healthcharger2 );
	CVAR_REGISTER( &sk_healthcharger3 );

	CVAR_REGISTER( &sk_healthkit1 );
	CVAR_REGISTER( &sk_healthkit2 );
	CVAR_REGISTER( &sk_healthkit3 );

	CVAR_REGISTER( &sk_scientist_heal1 );
	CVAR_REGISTER( &sk_scientist_heal2 );
	CVAR_REGISTER( &sk_scientist_heal3 );

	// monster damage adjusters
	CVAR_REGISTER( &sk_monster_head1 );
	CVAR_REGISTER( &sk_monster_head2 );
	CVAR_REGISTER( &sk_monster_head3 );

	CVAR_REGISTER( &sk_monster_chest1 );
	CVAR_REGISTER( &sk_monster_chest2 );
	CVAR_REGISTER( &sk_monster_chest3 );

	CVAR_REGISTER( &sk_monster_stomach1 );
	CVAR_REGISTER( &sk_monster_stomach2 );
	CVAR_REGISTER( &sk_monster_stomach3 );

	CVAR_REGISTER( &sk_monster_arm1 );
	CVAR_REGISTER( &sk_monster_arm2 );
	CVAR_REGISTER( &sk_monster_arm3 );

	CVAR_REGISTER( &sk_monster_leg1 );
	CVAR_REGISTER( &sk_monster_leg2 );
	CVAR_REGISTER( &sk_monster_leg3 );

	// player damage adjusters
	CVAR_REGISTER( &sk_player_head1 );
	CVAR_REGISTER( &sk_player_head2 );
	CVAR_REGISTER( &sk_player_head3 );

	CVAR_REGISTER( &sk_player_chest1 );
	CVAR_REGISTER( &sk_player_chest2 );
	CVAR_REGISTER( &sk_player_chest3 );

	CVAR_REGISTER( &sk_player_stomach1 );
	CVAR_REGISTER( &sk_player_stomach2 );
	CVAR_REGISTER( &sk_player_stomach3 );

	CVAR_REGISTER( &sk_player_arm1 );
	CVAR_REGISTER( &sk_player_arm2 );
	CVAR_REGISTER( &sk_player_arm3 );

	CVAR_REGISTER( &sk_player_leg1 );
	CVAR_REGISTER( &sk_player_leg2 );
	CVAR_REGISTER( &sk_player_leg3 );
// END REGISTER CVARS FOR SKILL LEVEL STUFF

	// BMOD Begin - CVARs
	CVAR_REGISTER( &bm_bver );
	CVAR_REGISTER( &bm_bname );
	CVAR_REGISTER( &bm_burl );
	CVAR_REGISTER( &bm_ver );
	CVAR_REGISTER( &bm_url );
	//CVAR_REGISTER( &bm_plat );
	CVAR_REGISTER( &bm_guns );
	CVAR_REGISTER( &bm_ammo );
	CVAR_REGISTER( &bm_g );
	CVAR_REGISTER( &bm_dmg_messages );
	CVAR_REGISTER( &bm_matchkills );
	//CVAR_REGISTER( &bm_trips );
	CVAR_REGISTER( &bm_freezetime );
	CVAR_REGISTER( &bm_thrust );
	CVAR_REGISTER( &bm_spawneffects );
	CVAR_REGISTER( &bm_snarktrails );
	CVAR_REGISTER( &bm_xbowtracers );

	CVAR_REGISTER( &bm_spawnkilltime );
	CVAR_REGISTER( &bm_maxspawnkills );
	CVAR_REGISTER( &bm_typekills );
	CVAR_REGISTER( &bm_maxtypekills );
	CVAR_REGISTER( &bm_typecam );
	
	CVAR_REGISTER( &bm_bantime );

	CVAR_REGISTER( &bm_spamlimit );
	CVAR_REGISTER( &bm_antispam );

	CVAR_REGISTER( &bm_spawnmines );
	CVAR_REGISTER( &bm_spawnsatchels );
	CVAR_REGISTER( &bm_voting );
	CVAR_REGISTER( &bm_votetime );
	CVAR_REGISTER( &bm_maxtime );
	CVAR_REGISTER( &bm_maxfrags );
 	//CVAR_REGISTER( &bm_cbmad );
 	//CVAR_REGISTER( &bm_cheatdetect );

/*	CVAR_REGISTER( &bm_score_crowbar );
	CVAR_REGISTER( &bm_score_throwncbar );
	CVAR_REGISTER( &bm_score_9mm );
	CVAR_REGISTER( &bm_score_357 );
	CVAR_REGISTER( &bm_score_mp5 );
	CVAR_REGISTER( &bm_score_shotgun );
	CVAR_REGISTER( &bm_score_squidspit );
	CVAR_REGISTER( &bm_score_zapgun );
	CVAR_REGISTER( &bm_score_mp5grenade );
	CVAR_REGISTER( &bm_score_gluon );
	CVAR_REGISTER( &bm_score_tau );
	CVAR_REGISTER( &bm_score_bolt );
	CVAR_REGISTER( &bm_score_crossbow );
	CVAR_REGISTER( &bm_score_satchel );
	CVAR_REGISTER( &bm_score_handgrenade );
	CVAR_REGISTER( &bm_score_rpg );
	CVAR_REGISTER( &bm_score_snarks );
	CVAR_REGISTER( &bm_score_tripmine );
*/
	CVAR_REGISTER( &bm_mods );
	CVAR_REGISTER( &bm_rpg_mod );
	CVAR_REGISTER( &bm_shotty_mod );
	CVAR_REGISTER( &bm_xbow_mod );
	CVAR_REGISTER( &bm_mp5_mod );
	CVAR_REGISTER( &bm_cbar_mod );
	CVAR_REGISTER( &bm_tau_mod );
	CVAR_REGISTER( &bm_snarks_mod );
	CVAR_REGISTER( &bm_gluon_mod );
	CVAR_REGISTER( &bm_hornet_mod );
	CVAR_REGISTER( &bm_trip_mod );
	CVAR_REGISTER( &bm_map );
	CVAR_REGISTER( &bm_nextmap );
	CVAR_REGISTER( &bm_rune_rand );
	CVAR_REGISTER( &bm_runemask );
	CVAR_REGISTER( &bm_rune_cbar );
	CVAR_REGISTER( &bm_rune_cbar_t );
	CVAR_REGISTER( &bm_rune_cbar_r );
	CVAR_REGISTER( &bm_rune_gren );
	CVAR_REGISTER( &bm_rune_gren_t );
	CVAR_REGISTER( &bm_rune_gren_r );
	CVAR_REGISTER( &bm_rune_357 );
	CVAR_REGISTER( &bm_rune_357_t );
	CVAR_REGISTER( &bm_rune_357_r );
	CVAR_REGISTER( &bm_rune_health );
	CVAR_REGISTER( &bm_rune_health_t );
	CVAR_REGISTER( &bm_rune_health_r );
	CVAR_REGISTER( &bm_rune_armor );
	CVAR_REGISTER( &bm_rune_armor_t );
	CVAR_REGISTER( &bm_rune_armor_r );
	CVAR_REGISTER( &bm_rune_shotty );
	CVAR_REGISTER( &bm_rune_shotty_t );
	CVAR_REGISTER( &bm_rune_shotty_r );
	// BMOD End - CVARs

	// BMOD Begin - Server commands
	ADD_SERVER_COMMAND( "s", BModCmd_AdminSay );
	ADD_SERVER_COMMAND( "w", BModCmd_AdminWhisper );
	ADD_SERVER_COMMAND( "markspawnpoints", BModCmd_ShowSpawns );
	ADD_SERVER_COMMAND( "sspeak", BModCmd_SpeakAll );
	ADD_SERVER_COMMAND( "create", BModCmd_Create );
	ADD_SERVER_COMMAND( "remove", BModCmd_Remove );
	ADD_SERVER_COMMAND( "delete", BModCmd_Delete );
	ADD_SERVER_COMMAND( "replace", BModCmd_Replace );
	ADD_SERVER_COMMAND( "info", BModCmd_Info );
	ADD_SERVER_COMMAND( "llama", BModCmd_Llama );
	ADD_SERVER_COMMAND( "unllama", BModCmd_Unllama );
	// BMOD End - Server commands

	SERVER_COMMAND( "exec skill.cfg\n" );
}

// ++BMod
// New bubble mod commands
//
// CMD_ARGC () - number of arguments
// CMD_ARGV(n) - nth argument. 0=actual command. 
// CMD_ARGS () -

// Helper function for finding a player pointer by UID. 
CBasePlayer* GetPlayerByUID( int userId )
{
	CBasePlayer *client = NULL;

	while( ( ( client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" ) ) != NULL )
		&& ( client->IsPlayer() ) ) 
	{
		if( userId == GETPLAYERUSERID( client->edict() ) )
			return client;
	}

	return 0;
}

// Admin Say 
void BModCmd_AdminSay( void )
{
	int	j;
	char	*p;
	char	text[128];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pc, *pcmd = CMD_ARGV(0);

	// We can get a raw string now, without the "say " prepended
	if( CMD_ARGC() < 2 )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: s \"<string>\"\n" );
		return;
	}

	p = (char *)CMD_ARGS();

	// make sure the text has content
	for( pc = p; pc != NULL && *pc != 0; pc++ )
	{
		if( isprint( *pc ) && !isspace( *pc ) )
		{
			pc = NULL;	// we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if( pc != NULL )
		return;  // no character found, so say nothing

	sprintf( text, "%c%s ", 2, "<ADMIN>" );

	j = sizeof(text) - 2 - strlen( text );  // -2 for /n and null terminator
	if( (int)strlen( p ) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );

	UTIL_ClientPrintAll( HUD_PRINTTALK, text ); 

	// echo to server console
	g_engfuncs.pfnServerPrint( text );

	UTIL_LogPrintf( "\"ADMIN<-1><-1><>\" say \"%s\"\n", p );
}

// Admin Whisper 
void BModCmd_AdminWhisper( void )
{	
	int	j;
	char	*p;
	char	text[128];
	const char *cpSay = "say";
	const char *cpSayTeam = "say_team";
	const char *pc, *pcmd = CMD_ARGV( 0 );

	// We can get a raw string now, without the "say " prepended
	if( CMD_ARGC() < 3 )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: w <PlayerUID> \"<string>\"\n" );
		return;
	}

	int UID = atoi( CMD_ARGV( 1 ) );
	
	CBasePlayer *Player = GetPlayerByUID( UID );
	if( Player == NULL )
	{
		g_engfuncs.pfnServerPrint( "Invalid Player UID.\n" );
		return;
	}

	p = (char *)CMD_ARGS();

	// Skip over the UID
	while( *p != ' ')
		p++;
	while( *p == ' ')
		p++;

	// make sure the text has content
	for( pc = p; pc != NULL && *pc != 0; pc++ )
	{
		if( isprint( *pc ) && !isspace( *pc ) )
		{
			pc = NULL;	// we've found an alphanumeric character,  so text is valid
			break;
		}
	}

	if( pc != NULL )
		return;  // no character found, so say nothing

	sprintf( text, "%c%s ", 2, "<ADMIN>(whispers)" );

	j = sizeof(text) - 2 - strlen( text );  // -2 for /n and null terminator
	if( (int)strlen( p ) > j )
		p[j] = 0;

	strcat( text, p );
	strcat( text, "\n" );

	ClientPrint( Player->pev, HUD_PRINTTALK, text ); 

	// echo to server console
	g_engfuncs.pfnServerPrint( text );

	UTIL_LogPrintf( "\"ADMIN<-1><-1><>\" say \"(to %s) %s\"\n", STRING( Player->pev->netname ), p );
}

// Show Spawn Points 
void BModCmd_ShowSpawns( void )
{
	BOOL marked = UTIL_FindEntityByClassname( NULL, "boxmarker" ) ? TRUE : FALSE;

	if( marked )
	{
		g_engfuncs.pfnServerPrint( "Spawn points already marked!\n" );
		return;
	}

	marked = TRUE;

	CBaseEntity *pSpot = NULL;
	CBaseEntity *pEnt = NULL;
	CBoxMarker *pBox = NULL;

	TraceResult tr;

	while( ( ( pSpot = UTIL_FindEntityByClassname( pSpot, "info_player_deathmatch" ) ) != NULL ) )
	{
		UTIL_TraceLine( pSpot->pev->origin, pSpot->pev->origin - Vector( 0, 0, 1024 ), ignore_monsters, pSpot->edict(), &tr );
		Vector vecTop = pSpot->pev->origin + Vector( 0, 0, 36 );
		float height = fabs( vecTop.z - tr.vecEndPos.z ) / 2;

		pEnt = CBaseEntity::Create( "boxmarker", Vector( vecTop.x, vecTop.y, ( vecTop.z + tr.vecEndPos.z ) / 2), g_vecZero, NULL );
		// CBaseEntity *pEnt2 = CBaseEntity::Create( "zaprift", Vector( vecTop.x, vecTop.y, ( vecTop.z + tr.vecEndPos.z ) / 2 ), g_vecZero, NULL );
		pBox = (CBoxMarker *)pEnt;
		pBox->m_vecExtents = Vector(16,16,height);
	}
}

// Speak to all players using vox. 
void BModCmd_SpeakAll( void )
{
	int	j;
	char	*p, *pc;
	char	text[128];

	if( CMD_ARGC() < 2 )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: sspeak \"<vox string>\"\n" );
		return;
	}

	p = (char *)CMD_ARGS();

	// remove quotes if present
	if( *p == '"' )
	{
		p++;
		p[strlen( p ) - 1] = 0;
	}

	// make sure the text has content
	for( pc = p; pc != NULL && *pc != 0; pc++ )
	{
		if( isprint( *pc ) && !isspace( *pc ) )
		{
			pc = NULL;	// we've found an alphanumeric character,  so text is valid
			break;
		}
	}
	if( pc != NULL )
		return;  // no character found, so say nothing

	strcpy( text, "");

	j = sizeof( text ) - 2 - strlen( text );  // -2 for /n and null terminator
	if( (int)strlen( p ) > j )
		p[j] = 0;

	strcat( text, p );
	//strcat( text, "\n" );

	UTIL_SpeakAll( text ); 
}

// create entity x y z ay
void BModCmd_Create( void )
{
	if( ( CMD_ARGC() < 6 ) )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: create <entity> <xpos> <ypos> <zpos> <y angle>\n" );
		return;
	}

	char *entity = (char *)CMD_ARGV( 1 );
	Vector position = Vector( atoi( CMD_ARGV( 2 ) ), atoi( CMD_ARGV( 3 ) ), atoi( CMD_ARGV( 4 ) ) );
	Vector angle = Vector( 0, atoi( CMD_ARGV( 5 ) ), 0 );

	if( angle.y < 0 )
	{
		angle.y = RANDOM_LONG( 0, 360 );
	}

	// Fix weapon names
	if( !strcmp( entity, "weapon_9mmhandgun" ) )
	{
		strcpy( entity, "weapon_glock" );
	}
	else if( !strcmp( entity, "weapon_9mmAR" ) )
	{
		strcpy( entity, "weapon_mp5" );
	}
	else if( !strcmp( entity, "weapon_python" ) )
	{
		strcpy( entity, "weapon_357" );
	}

	// Fix ammo names
	if( !strcmp( entity, "ammo_9mmclip" ) )
	{
		strcpy( entity, "ammo_glockclip" );
	}
	else if( !strcmp( entity, "ammo_9mmAR" ) )
	{
		strcpy( entity, "ammo_mp5clip" );
	}
	else if( !strcmp( entity, "ammo_ARgrenades" ) )
	{
		strcpy( entity, "ammo_mp5grenades" );
	}
	
	if( !strncmp( entity, "weapon_", 7 ) ||
		!strcmp( entity, "item_healthkit" ) ||
		!strcmp( entity, "item_battery" ) ||
		!strcmp( entity, "item_longjump" ) ||
		!strncmp( entity, "ammo_", 5 ) ||
		!strcmp( entity, "info_player_deathmatch" ) )
	{
		CBaseEntity *ent = CBaseEntity::Create(entity, position, angle, NULL );

		if( !strcmp( entity, "info_player_deathmatch" ) )
			ent->pev->classname = MAKE_STRING( "info_player_deathmatch" );
	}
	else
	{
		g_engfuncs.pfnServerPrint( "You only add items, ammo, weapons, or spawn points with this command.\n" );
	}
}

// remove entity 
void BModCmd_Remove( void )
{
	if( ( CMD_ARGC() < 2 ) )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: remove <entity name>\n" );
		return;
	}

	char *entity = (char *)CMD_ARGV( 1 );
	CBaseEntity *target = NULL;

	// Fix weapon names
	if( !strcmp(entity, "weapon_glock" ) )
	{
		strcpy(entity, "weapon_9mmhandgun" );
	}
	else if( !strcmp( entity, "weapon_mp5" ) )
	{
		strcpy( entity, "weapon_9mmAR" );
	}
	else if( !strcmp( entity, "weapon_python" ) )
	{
		strcpy( entity, "weapon_357" );
	}

	// Fix ammo names
	if( !strcmp( entity, "ammo_9mmclip" ) )
	{
		strcpy( entity, "ammo_glockclip" );
	}
	else if( !strcmp( entity, "ammo_9mmAR") )
	{
		strcpy( entity, "ammo_mp5clip");
	}
	else if( !strcmp( entity, "ammo_ARgrenades") )
	{
		strcpy( entity, "ammo_mp5grenades");
	}

	if( !strncmp( entity, "weapon_", 7 ) ||
		!strcmp( entity, "item_healthkit" ) ||
		!strcmp( entity, "item_battery" ) ||
		!strcmp( entity, "item_longjump" ) ||
		!strncmp( entity, "ammo_", 5 ) )
	{
		while( ( target = UTIL_FindEntityInSphere( target, Vector( 0, 0, 0 ), 4096 ) ) != NULL )
		{
			if( !strcmp( STRING( target->pev->classname ), entity ) && ( target->pev->owner == NULL ) )
			{
				target->Killed( NULL, 0 );
			}
		}
	}
	else
	{
		g_engfuncs.pfnServerPrint( "You only remove items, ammo, or weapons with this command.\n" );
	}
}

// delete a single entity 
void BModCmd_Delete( void )
{
	if( ( CMD_ARGC() < 5 ) )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: delete <entity name> <xpos> <ypos> <zpos>\n" );
		return;
	}

	char *entity = (char *)CMD_ARGV( 1 );
	Vector position = Vector( atoi( CMD_ARGV( 2 ) ), atoi( CMD_ARGV( 3 ) ), atoi( CMD_ARGV( 4 ) ) );
	CBaseEntity *target = NULL;

	// Fix weapon names
	if( !strcmp( entity, "weapon_glock" ) )
	{
		strcpy( entity, "weapon_9mmhandgun" );
	}
	else if( !strcmp( entity, "weapon_mp5" ) )
	{
		strcpy( entity, "weapon_9mmAR" );
	}
	else if( !strcmp( entity, "weapon_python" ) )
	{
		strcpy( entity, "weapon_357" );
	}

	// Fix ammo names
	if( !strcmp( entity, "ammo_9mmclip" ) )
	{
		strcpy( entity, "ammo_glockclip" );
	}
	else if( !strcmp( entity, "ammo_9mmAR" ) )
	{
		strcpy( entity, "ammo_mp5clip" );
	}
	else if( !strcmp( entity, "ammo_ARgrenades" ) )
	{
		strcpy( entity, "ammo_mp5grenades" );
	}

	if( !strncmp( entity, "weapon_", 7 ) ||
		!strcmp( entity, "item_healthkit" ) ||
		!strcmp( entity, "item_battery" ) ||
		!strcmp( entity, "item_longjump" ) ||
		!strncmp( entity, "ammo_", 5 ) ||
		!strcmp( entity, "info_player_deathmatch" ) )
	{
		bool deleted = FALSE;
		while( !deleted && ( target = UTIL_FindEntityInSphere( target, position, 64 ) ) != NULL )
		{
			if( !strcmp( STRING( target->pev->classname ), entity ) && ( target->pev->owner == NULL ) )
			{
				target->Killed( NULL, 0 );
				deleted = TRUE;
				g_engfuncs.pfnServerPrint( "Entity deleted.\n" );
			}  
		}
		
		if( !deleted )
		{
			g_engfuncs.pfnServerPrint( "Entity not found.\n" );
		}
	}
	else
	{
		g_engfuncs.pfnServerPrint( "You only delete items, ammo, weapons, or spawn points with this command.\n" );
	}
}

// replace entity 
void BModCmd_Replace( void )
{
	if( ( CMD_ARGC() < 3 ) )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: replace <entity> <with entity>\n" );
		return;
	}

	char *entity = (char *)CMD_ARGV( 1 );
	char *entity2 = (char *)CMD_ARGV( 2 );
	CBaseEntity *target = NULL;

	// Fix weapon names
	if( !strcmp( entity, "weapon_glock" ) )
	{
		strcpy( entity, "weapon_9mmhandgun" );
	}
	else if( !strcmp( entity, "weapon_mp5" ) )
	{
		strcpy( entity, "weapon_9mmAR" );
	}
	else if( !strcmp( entity, "weapon_python" ) )
	{
		strcpy( entity, "weapon_357" );
	}

	if( !strcmp( entity2, "weapon_9mmhandgun" ) )
	{
		strcpy( entity2, "weapon_glock" );
	}
	else if( !strcmp( entity2, "weapon_9mmAR" ) )
	{
		strcpy( entity2, "weapon_mp5" );
	}
	else if( !strcmp( entity2, "weapon_python" ) )
	{
		strcpy( entity2, "weapon_357" );
	}

	// Fix ammo names
	if( !strcmp( entity, "ammo_9mmclip" ) )
	{
		strcpy( entity, "ammo_glockclip" );
	}
	else if( !strcmp( entity, "ammo_9mmAR") )
	{
		strcpy( entity, "ammo_mp5clip");
	}
	else if( !strcmp( entity, "ammo_ARgrenades") )
	{
		strcpy( entity, "ammo_mp5grenades" );
	}

	if( !strcmp( entity2, "ammo_9mmclip") )
	{
		strcpy( entity2, "ammo_glockclip" );
	}
	else if( !strcmp( entity2, "ammo_9mmAR" ) )
	{
		strcpy( entity2, "ammo_mp5clip" );
	}
	else if( !strcmp( entity2, "ammo_ARgrenades" ) )
	{
		strcpy( entity2, "ammo_mp5grenades" );
	}

	if( ( !strncmp( entity, "weapon_", 7 ) ||
		!strcmp( entity, "item_healthkit" ) ||
		!strcmp( entity, "item_battery" ) ||
		!strcmp( entity, "item_longjump" ) ||
		!strncmp( entity, "ammo_", 5 ) ) &&
	   ( !strncmp( entity2, "weapon_", 7 ) ||
		!strcmp( entity2, "item_healthkit" ) ||
		!strcmp( entity2, "item_battery" ) ||
		!strcmp( entity2, "item_longjump" ) ||
		!strncmp( entity2, "ammo_", 5 ) ) )
	{
		
		while( ( target = UTIL_FindEntityInSphere( target, Vector( 0, 0, 0 ), 4096 ) ) != NULL )
		{
			if( !strcmp( STRING( target->pev->classname ), entity ) && ( target->pev->owner == NULL ) )
			{
				CBaseEntity::Create(entity2, target->pev->origin, target->pev->angles, NULL );
				target->Killed(NULL, 0);
			}
		}
	}
	else
	{
		g_engfuncs.pfnServerPrint( "You only replace items, ammo, or weapons with this command.\n" );
	}
}

// info on a player
void BModCmd_Info( void )
{
	if( CMD_ARGC() < 2 )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: info <PlayerUID>\n" );
		return;
	}

	int UID = atoi( CMD_ARGV( 1 ) );

	CBasePlayer *Player = GetPlayerByUID( UID );
	if( Player == NULL )
	{
		g_engfuncs.pfnServerPrint( "Invalid Player UID.\n" );
		return;
	}

	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "Player: %s\n",STRING( Player->pev->netname ) ) );
	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "Health/Armor: %d/%d\n",(int)Player->pev->health, (int)Player->pev->armorvalue ) );

	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "Spawn Kills: %d\n",Player->m_iSpawnKills ) );
	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "Type Kills: %d\n",Player->m_iTypeKills ) );
	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "Leet: %d\n",Player->m_LeetSpeak ) );
	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "Locate: %d\n",Player->m_LocateMode ) );
	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "Llama: %d\n",Player->m_IsLlama ) );
	g_engfuncs.pfnServerPrint( "\n" );
}

// Llamafy a player
void BModCmd_Llama( void )
{
	if( CMD_ARGC() < 2 )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: llama <PlayerUID>\n" );
		return;
	}

	int UID = atoi( CMD_ARGV( 1 ) );
	
	CBasePlayer *Player = GetPlayerByUID( UID );
	if( Player == NULL )
	{
		g_engfuncs.pfnServerPrint( "Invalid Player UID.\n" );
		return;
	}

	Player->m_IsLlama = TRUE;

	UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "<SERVER> %s is now a llama! Bleeet!\n", STRING( Player->pev->netname ) ) ); 
	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "%s is now a llama! Bleeet!\n", STRING( Player->pev->netname ) ) );
}

// Unllamafy a player
void BModCmd_Unllama( void )
{
	if( CMD_ARGC() < 2 )
	{
		g_engfuncs.pfnServerPrint( "Not enough arguments.\nUSAGE: unllama <PlayerUID>\n" );
		return;
	}

	int UID = atoi( CMD_ARGV( 1 ) );
	
	CBasePlayer *Player = GetPlayerByUID( UID );
	if( Player == NULL )
	{
		g_engfuncs.pfnServerPrint( "Invalid Player UID.\n" );
		return;
	}

	Player->m_IsLlama = FALSE;

	UTIL_ClientPrintAll( HUD_PRINTTALK, UTIL_VarArgs( "<SERVER> %s is unllamafied.\n", STRING( Player->pev->netname ) ) ); 
	g_engfuncs.pfnServerPrint( UTIL_VarArgs( "%s is unllamafied.\n", STRING( Player->pev->netname ) ) );
}
