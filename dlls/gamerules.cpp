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
// GameRules.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"teamplay_gamerules.h"
#include	"skill.h"
#include	"game.h"

#include	"ctf_gamerules.h"

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

DLL_GLOBAL CGameRules *g_pGameRules = NULL;
extern DLL_GLOBAL BOOL g_fGameOver;
extern int gmsgDeathMsg;	// client dll messages
extern int gmsgMOTD;

int g_teamplay = 0;

//=========================================================
//=========================================================
BOOL CGameRules::CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry )
{
	int iAmmoIndex;

	if( pszAmmoName )
	{
		iAmmoIndex = pPlayer->GetAmmoIndex( pszAmmoName );

		if( iAmmoIndex > -1 )
		{
			if( pPlayer->AmmoInventory( iAmmoIndex ) < iMaxCarry )
			{
				// player has room for more of this type of ammo
				return TRUE;
			}
		}
	}

	return FALSE;
}

//=========================================================
//=========================================================
edict_t *CGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );

	pPlayer->pev->origin = VARS( pentSpawnSpot )->origin + Vector( 0, 0, 1 );
	pPlayer->pev->v_angle  = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->angles = VARS( pentSpawnSpot )->angles;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;

	return pentSpawnSpot;
}

//=========================================================
//=========================================================
BOOL CGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon )
{
	// only living players can have items
	if( pPlayer->pev->deadflag != DEAD_NO )
		return FALSE;

	if( pWeapon->pszAmmo1() )
	{
		if( !CanHaveAmmo( pPlayer, pWeapon->pszAmmo1(), pWeapon->iMaxAmmo1() ) )
		{
			// we can't carry anymore ammo for this gun. We can only 
			// have the gun if we aren't already carrying one of this type
			if( pPlayer->HasPlayerItem( pWeapon ) )
			{
				return FALSE;
			}
		}
	}
	else
	{
		// weapon doesn't use ammo, don't take another if you already have it.
		if( pPlayer->HasPlayerItem( pWeapon ) )
		{
			return FALSE;
		}
	}

	// note: will fall through to here if GetItemInfo doesn't fill the struct!
	return TRUE;
}

//=========================================================
// load the SkillData struct with the proper values based on the skill level.
//=========================================================
void CGameRules::RefreshSkillData ( void )
{
	int iSkill;

	iSkill = (int)CVAR_GET_FLOAT( "skill" );
	g_iSkillLevel = iSkill;

	if( iSkill < 1 )
	{
		iSkill = 1;
	}
	else if( iSkill > 3 )
	{
		iSkill = 3; 
	}

	gSkillData.iSkillLevel = iSkill;

	ALERT( at_console, "\nGAME SKILL LEVEL:%d\n",iSkill );

	//Agrunt		
	gSkillData.agruntHealth = GetSkillCvar( "sk_agrunt_health" );
	gSkillData.agruntDmgPunch = GetSkillCvar( "sk_agrunt_dmg_punch" );

	// Apache 
	gSkillData.apacheHealth = GetSkillCvar( "sk_apache_health" );

	// Barney
	gSkillData.barneyHealth = GetSkillCvar( "sk_barney_health" );

	// Big Momma
	gSkillData.bigmommaHealthFactor = GetSkillCvar( "sk_bigmomma_health_factor" );
	gSkillData.bigmommaDmgSlash = GetSkillCvar( "sk_bigmomma_dmg_slash" );
	gSkillData.bigmommaDmgBlast = GetSkillCvar( "sk_bigmomma_dmg_blast" );
	gSkillData.bigmommaRadiusBlast = GetSkillCvar( "sk_bigmomma_radius_blast" );

	// Bullsquid
	gSkillData.bullsquidHealth = GetSkillCvar( "sk_bullsquid_health" );
	gSkillData.bullsquidDmgBite = GetSkillCvar( "sk_bullsquid_dmg_bite" );
	gSkillData.bullsquidDmgWhip = GetSkillCvar( "sk_bullsquid_dmg_whip" );
	gSkillData.bullsquidDmgSpit = GetSkillCvar( "sk_bullsquid_dmg_spit" );

	// Gargantua
	gSkillData.gargantuaHealth = GetSkillCvar( "sk_gargantua_health" );
	gSkillData.gargantuaDmgSlash = GetSkillCvar( "sk_gargantua_dmg_slash" );
	gSkillData.gargantuaDmgFire = GetSkillCvar( "sk_gargantua_dmg_fire" );
	gSkillData.gargantuaDmgStomp = GetSkillCvar( "sk_gargantua_dmg_stomp" );

	// Hassassin
	gSkillData.hassassinHealth = GetSkillCvar( "sk_hassassin_health" );

	// Headcrab
	gSkillData.headcrabHealth = GetSkillCvar( "sk_headcrab_health" );
	gSkillData.headcrabDmgBite = GetSkillCvar( "sk_headcrab_dmg_bite" );

	// Hgrunt 
	gSkillData.hgruntHealth = GetSkillCvar( "sk_hgrunt_health" );
	gSkillData.hgruntDmgKick = GetSkillCvar( "sk_hgrunt_kick" );
	gSkillData.hgruntShotgunPellets = GetSkillCvar( "sk_hgrunt_pellets" );
	gSkillData.hgruntGrenadeSpeed = GetSkillCvar( "sk_hgrunt_gspeed" );

	// Houndeye
	gSkillData.houndeyeHealth = GetSkillCvar( "sk_houndeye_health" );
	gSkillData.houndeyeDmgBlast = GetSkillCvar( "sk_houndeye_dmg_blast" );

	// ISlave
	gSkillData.slaveHealth = GetSkillCvar( "sk_islave_health" );
	gSkillData.slaveDmgClaw = GetSkillCvar( "sk_islave_dmg_claw" );
	gSkillData.slaveDmgClawrake = GetSkillCvar( "sk_islave_dmg_clawrake" );
	gSkillData.slaveDmgZap = GetSkillCvar( "sk_islave_dmg_zap" );

	// Icthyosaur
	gSkillData.ichthyosaurHealth = GetSkillCvar( "sk_ichthyosaur_health" );
	gSkillData.ichthyosaurDmgShake = GetSkillCvar( "sk_ichthyosaur_shake" );

	// Leech
	gSkillData.leechHealth = GetSkillCvar( "sk_leech_health" );

	gSkillData.leechDmgBite = GetSkillCvar( "sk_leech_dmg_bite" );

	// Controller
	gSkillData.controllerHealth = GetSkillCvar( "sk_controller_health" );
	gSkillData.controllerDmgZap = GetSkillCvar( "sk_controller_dmgzap" );
	gSkillData.controllerSpeedBall = GetSkillCvar( "sk_controller_speedball" );
	gSkillData.controllerDmgBall = GetSkillCvar( "sk_controller_dmgball" );

	// Nihilanth
	gSkillData.nihilanthHealth = GetSkillCvar( "sk_nihilanth_health" );
	gSkillData.nihilanthZap = GetSkillCvar( "sk_nihilanth_zap" );

	// Scientist
	gSkillData.scientistHealth = GetSkillCvar( "sk_scientist_health" );

	// Snark
	gSkillData.snarkHealth = GetSkillCvar( "sk_snark_health" );
	gSkillData.snarkDmgBite = GetSkillCvar( "sk_snark_dmg_bite" );
	gSkillData.snarkDmgPop = GetSkillCvar( "sk_snark_dmg_pop" );

	// Zombie
	gSkillData.zombieHealth = GetSkillCvar( "sk_zombie_health" );
	gSkillData.zombieDmgOneSlash = GetSkillCvar( "sk_zombie_dmg_one_slash" );
	gSkillData.zombieDmgBothSlash = GetSkillCvar( "sk_zombie_dmg_both_slash" );

	//Turret
	gSkillData.turretHealth = GetSkillCvar( "sk_turret_health" );

	// MiniTurret
	gSkillData.miniturretHealth = GetSkillCvar( "sk_miniturret_health" );

	// Sentry Turret
	gSkillData.sentryHealth = GetSkillCvar( "sk_sentry_health" );

	//
	// Op4 Monsters
	//
	// Otis
	gSkillData.otisHealth = GetSkillCvar( "sk_otis_health" );

	// Pitdrone
	gSkillData.pitdroneHealth = GetSkillCvar( "sk_pitdrone_health" );
	gSkillData.pitdroneDmgBite = GetSkillCvar( "sk_pitdrone_dmg_bite" );
	gSkillData.pitdroneDmgWhip = GetSkillCvar( "sk_pitdrone_dmg_whip" );
	gSkillData.pitdroneDmgSpit = GetSkillCvar( "sk_pitdrone_dmg_spit" );

	// Hgrunt Ally
	gSkillData.hgruntAllyHealth = GetSkillCvar( "sk_hgrunt_ally_health" );
	gSkillData.hgruntAllyDmgKick = GetSkillCvar( "sk_hgrunt_ally_kick" );
	gSkillData.hgruntAllyShotgunPellets = GetSkillCvar( "sk_hgrunt_ally_pellets" );
	gSkillData.hgruntAllyGrenadeSpeed = GetSkillCvar( "sk_hgrunt_ally_gspeed" );

	// Medic Ally 
	gSkillData.medicAllyHealth = GetSkillCvar( "sk_medic_ally_health" );
	gSkillData.medicAllyDmgKick = GetSkillCvar( "sk_medic_ally_kick" );
	gSkillData.medicAllyGrenadeSpeed = GetSkillCvar( "sk_medic_ally_gspeed" );
	gSkillData.medicAllyHeal = GetSkillCvar( "sk_medic_ally_heal" );

	// Torch Ally 
	gSkillData.torchAllyHealth = GetSkillCvar( "sk_torch_ally_health" );
	gSkillData.torchAllyDmgKick = GetSkillCvar( "sk_torch_ally_kick" );
	gSkillData.torchAllyGrenadeSpeed = GetSkillCvar( "sk_torch_ally_gspeed" );

	// Male Assassin
	gSkillData.massnHealth = GetSkillCvar( "sk_massassin_health" );
	gSkillData.massnDmgKick = GetSkillCvar( "sk_massassin_kick" );
	gSkillData.massnGrenadeSpeed = GetSkillCvar( "sk_massassin_gspeed" );

	// ShockTrooper 
	gSkillData.strooperHealth = GetSkillCvar( "sk_shocktrooper_health" );
	gSkillData.strooperDmgKick = GetSkillCvar( "sk_shocktrooper_kick" );
	gSkillData.strooperGrenadeSpeed = GetSkillCvar( "sk_shocktrooper_gspeed" );
	gSkillData.strooperMaxCharge = GetSkillCvar( "sk_shocktrooper_maxcharge" );
	gSkillData.strooperRchgSpeed = GetSkillCvar( "sk_shocktrooper_rchgspeed" );

	// Scientist
	gSkillData.cleansuitScientistHealth = GetSkillCvar( "sk_cleansuit_scientist_health" );
	gSkillData.cleansuitScientistHeal = GetSkillCvar( "sk_cleansuit_scientist_heal" );

	// Voltigore
	gSkillData.voltigoreHealth = GetSkillCvar( "sk_voltigore_health" );
	gSkillData.voltigoreDmgPunch = GetSkillCvar( "sk_voltigore_dmg_punch" );
	gSkillData.voltigoreDmgBeam = GetSkillCvar( "sk_voltigore_dmg_beam" );

	// Baby Voltigore
	gSkillData.babyVoltigoreHealth = GetSkillCvar( "sk_babyvoltigore_health" );
	gSkillData.babyVoltigoreDmgPunch = GetSkillCvar( "sk_babyvoltigore_dmg_punch" );

	// Pitworm
	gSkillData.pwormHealth = GetSkillCvar( "sk_pitworm_health" );
	gSkillData.pwormDmgSwipe = GetSkillCvar( "sk_pitworm_dmg_swipe" );
	gSkillData.pwormDmgBeam = GetSkillCvar( "sk_pitworm_dmg_beam" );

	// Geneworm
	gSkillData.gwormHealth = GetSkillCvar( "sk_geneworm_health" );
	gSkillData.gwormDmgSpit = GetSkillCvar( "sk_geneworm_dmg_spit" );
	gSkillData.gwormDmgHit = GetSkillCvar( "sk_geneworm_dmg_hit" );

	// Zombie Barney
	gSkillData.zbarneyHealth = GetSkillCvar( "sk_zombie_barney_health" );
	gSkillData.zbarneyDmgOneSlash = GetSkillCvar( "sk_zombie_barney_dmg_one_slash" );
	gSkillData.zbarneyDmgBothSlash = GetSkillCvar( "sk_zombie_barney_dmg_both_slash" );

	// Zombie Soldier
	gSkillData.zgruntHealth = GetSkillCvar( "sk_zombie_soldier_health" );
	gSkillData.zgruntDmgOneSlash = GetSkillCvar( "sk_zombie_soldier_dmg_one_slash" );
	gSkillData.zgruntDmgBothSlash = GetSkillCvar( "sk_zombie_soldier_dmg_both_slash" );

	// Gonome
	gSkillData.gonomeHealth = GetSkillCvar( "sk_gonome_health" );
	gSkillData.gonomeDmgOneSlash = GetSkillCvar( "sk_gonome_dmg_one_slash" );
	gSkillData.gonomeDmgGuts = GetSkillCvar( "sk_gonome_dmg_guts" );
	gSkillData.gonomeDmgOneBite = GetSkillCvar( "sk_gonome_dmg_one_bite" );

	// Shock Roach
	gSkillData.sroachHealth = GetSkillCvar( "sk_shockroach_health" );
	gSkillData.sroachDmgBite = GetSkillCvar( "sk_shockroach_dmg_bite" );
	gSkillData.sroachLifespan = GetSkillCvar( "sk_shockroach_lifespan" );

	// PLAYER WEAPONS

	// Crowbar whack
	gSkillData.plrDmgCrowbar = GetSkillCvar( "sk_plr_crowbar" );

	// Glock Round
	gSkillData.plrDmg9MM = GetSkillCvar( "sk_plr_9mm_bullet" );

	// 357 Round
	gSkillData.plrDmg357 = GetSkillCvar( "sk_plr_357_bullet" );

	// MP5 Round
	gSkillData.plrDmgMP5 = GetSkillCvar( "sk_plr_9mmAR_bullet" );

	// M203 grenade
	gSkillData.plrDmgM203Grenade = GetSkillCvar( "sk_plr_9mmAR_grenade" );

	// Shotgun buckshot
	gSkillData.plrDmgBuckshot = GetSkillCvar( "sk_plr_buckshot" );

	// Crossbow
	gSkillData.plrDmgCrossbowClient = GetSkillCvar( "sk_plr_xbow_bolt_client" );
	gSkillData.plrDmgCrossbowMonster = GetSkillCvar( "sk_plr_xbow_bolt_monster" );

	// RPG
	gSkillData.plrDmgRPG = GetSkillCvar( "sk_plr_rpg" );

	// Gauss gun
	gSkillData.plrDmgGauss = GetSkillCvar( "sk_plr_gauss" );

	// Egon Gun
	gSkillData.plrDmgEgonNarrow = GetSkillCvar( "sk_plr_egon_narrow" );
	gSkillData.plrDmgEgonWide = GetSkillCvar( "sk_plr_egon_wide" );

	// Hand Grendade
	gSkillData.plrDmgHandGrenade = GetSkillCvar( "sk_plr_hand_grenade" );

	// Satchel Charge
	gSkillData.plrDmgSatchel = GetSkillCvar( "sk_plr_satchel" );

	// Tripmine
	gSkillData.plrDmgTripmine = GetSkillCvar( "sk_plr_tripmine" );

	// MONSTER WEAPONS
	gSkillData.monDmg12MM = GetSkillCvar( "sk_12mm_bullet" );
	gSkillData.monDmgMP5 = GetSkillCvar ("sk_9mmAR_bullet" );
	gSkillData.monDmg9MM = GetSkillCvar( "sk_9mm_bullet" );

	// MONSTER HORNET
	gSkillData.monDmgHornet = GetSkillCvar( "sk_hornet_dmg" );

	// PLAYER HORNET
// Up to this point, player hornet damage and monster hornet damage were both using
// monDmgHornet to determine how much damage to do. In tuning the hivehand, we now need
// to separate player damage and monster hivehand damage. Since it's so late in the project, we've
// added plrDmgHornet to the SKILLDATA struct, but not to the engine CVar list, so it's inaccesible
// via SKILLS.CFG. Any player hivehand tuning must take place in the code. (sjb)
	gSkillData.plrDmgHornet = 7;

	//
	// Op4 Player weapons
	//
	gSkillData.plrDmgPWrench = GetSkillCvar( "sk_plr_pipewrench" );
	gSkillData.plrDmgKnife = GetSkillCvar( "sk_plr_knife" );
	gSkillData.plrDmgGrapple = GetSkillCvar( "sk_plr_grapple" );
	gSkillData.plrDmgEagle = GetSkillCvar( "sk_plr_eagle" );
	gSkillData.plrDmgDisplacer = GetSkillCvar( "sk_plr_displacer_self" );
	gSkillData.plrDmgShockroachSingleplayer = GetSkillCvar( "sk_plr_shockroachs" );
	gSkillData.plrDmgSpore = GetSkillCvar( "sk_plr_spore" );
	gSkillData.plrDmg762 = GetSkillCvar( "sk_plr_762_bullet" );
	gSkillData.plrDmg556 = GetSkillCvar( "sk_plr_556_bullet" );

	gSkillData.monDmg762 = GetSkillCvar( "sk_plr_762_bullet" );
	gSkillData.monDmg556 = GetSkillCvar( "sk_plr_556_bullet" );
	gSkillData.monDmgDisplacer = GetSkillCvar( "sk_plr_displacer_other" );
	gSkillData.plrDmgShockroachMultiplayer = GetSkillCvar( "sk_plr_shockroachm" );

	// HEALTH/CHARGE
	gSkillData.suitchargerCapacity = GetSkillCvar( "sk_suitcharger" );
	gSkillData.batteryCapacity = GetSkillCvar( "sk_battery" );
	gSkillData.healthchargerCapacity = GetSkillCvar ( "sk_healthcharger" );
	gSkillData.healthkitCapacity = GetSkillCvar ( "sk_healthkit" );
	gSkillData.scientistHeal = GetSkillCvar ( "sk_scientist_heal" );

	// monster damage adj
	gSkillData.monHead = GetSkillCvar( "sk_monster_head" );
	gSkillData.monChest = GetSkillCvar( "sk_monster_chest" );
	gSkillData.monStomach = GetSkillCvar( "sk_monster_stomach" );
	gSkillData.monLeg = GetSkillCvar( "sk_monster_leg" );
	gSkillData.monArm = GetSkillCvar( "sk_monster_arm" );

	// player damage adj
	gSkillData.plrHead = GetSkillCvar( "sk_player_head" );
	gSkillData.plrChest = GetSkillCvar( "sk_player_chest" );
	gSkillData.plrStomach = GetSkillCvar( "sk_player_stomach" );
	gSkillData.plrLeg = GetSkillCvar( "sk_player_leg" );
	gSkillData.plrArm = GetSkillCvar( "sk_player_arm" );

	//
	// Op4 additional cvars
	//
	gSkillData.displacerDmgRadius = GetSkillCvar( "sk_plr_displacer_radius" );
}

//=========================================================
// instantiate the proper game rules object
//=========================================================

CGameRules *InstallGameRules( void )
{
	SERVER_COMMAND( "exec game.cfg\n" );
	SERVER_EXECUTE();

	if( !gpGlobals->deathmatch )
	{
		// generic half-life
		g_teamplay = 0;
		return new CHalfLifeRules;
	}
	else
	{
		// HACK HACK!! Put here to tell whether the current map supports
		// CTF gamerules since there is no explicit way to specify
		// the CTF gamerule in server creation page.
		if( !strnicmp( STRING( gpGlobals->mapname ), "op4ctf", 6 ) )
		{
			// capture the flag
			g_teamplay = 1;
			return new CCTFMultiplay;
		}

		if( teamplay.value > 0 )
		{
			// teamplay
			g_teamplay = 1;
			return new CHalfLifeTeamplay;
		}
		else
		{
			// vanilla deathmatch??
			g_teamplay = 0;
			return new CHalfLifeMultiplay;
		}
	}
}
