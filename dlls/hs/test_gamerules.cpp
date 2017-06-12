//
// test_gamerules
// Game rules for the game mode, for testing weapons
// and other beta testing.
//

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"test_gamerules.h"
#include	"game.h"

extern DLL_GLOBAL BOOL		g_fGameOver;
extern int gmsgScoreInfo;
extern int gmsgShowGameTitle;

CTestplay :: CTestplay()
{
 //Genuflect
}

BOOL CTestplay::IsHeavyRain()
{
 return FALSE;
}

void CTestplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	ALERT(at_console, "Test Game Mode Initalized!\no Giving Weapons\no Kills don't count\no This is for testing and sandboxing\no Have fun!");

	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;

	edict_t *pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	while( ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ) ) )
	{
		pWeaponEntity->Touch( pPlayer );
		addDefault = FALSE;
	}

	if ( addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_fotn" );
		pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
		pPlayer->GiveNamedItem( "ammo_9mmclip" );
		pPlayer->GiveNamedItem( "weapon_shotgun" );
		pPlayer->GiveNamedItem( "ammo_buckshot" );
		pPlayer->GiveNamedItem( "weapon_9mmAR" );
		pPlayer->GiveNamedItem( "ammo_9mmAR" );
		pPlayer->GiveNamedItem( "ammo_ARgrenades" );
		pPlayer->GiveNamedItem( "weapon_handgrenade" );
		pPlayer->GiveNamedItem( "weapon_tripmine" );
		pPlayer->GiveNamedItem( "weapon_rpg" );
		pPlayer->GiveNamedItem( "ammo_rpgclip" );
		pPlayer->GiveNamedItem( "weapon_satchel" );
		pPlayer->GiveNamedItem( "weapon_snark" );
		pPlayer->GiveNamedItem( "weapon_soda" );
		pPlayer->GiveNamedItem( "weapon_dosh" );
		pPlayer->GiveNamedItem( "weapon_beamkatana" );
		pPlayer->GiveNamedItem( "weapon_ak47" );
		pPlayer->GiveNamedItem( "weapon_bow" );
		pPlayer->GiveNamedItem( "weapon_jason" );
		pPlayer->GiveNamedItem( "weapon_jihad" );
		pPlayer->GiveNamedItem( "weapon_jackal" );
		pPlayer->GiveNamedItem( "weapon_nstar" );
		pPlayer->GiveNamedItem( "weapon_mw2" );
		pPlayer->GiveNamedItem( "weapon_zapper" );
		pPlayer->GiveNamedItem( "weapon_goldengun" );
		pPlayer->GiveNamedItem( "weapon_boombox" );
		pPlayer->GiveNamedItem( "weapon_scientist" );
		pPlayer->GiveNamedItem( "weapon_modman" );
	}
		//char welcometext[1024] = "Welcome to Test Gamemode!\n";
		//UTIL_SayText( welcometext, pPlayer );
}

void CTestplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CBasePlayer *peKiller = NULL;
	CBaseEntity *ktmp = CBaseEntity::Instance( pKiller );
	if ( pVictim->pev == pKiller )  
	{  // killed self
		char victext[1024] = "You killed yourself.\n";
		UTIL_SayText( victext, pVictim );
		return;
	}
	else if ( ktmp && ktmp->IsPlayer() )
	{

		char victext[1024] = "You got shot and died.\n";
		UTIL_SayText( victext, pVictim );

	CBaseEntity *ep = CBaseEntity::Instance( pKiller );
	if ( ep && ep->Classify() == CLASS_PLAYER )
	{
		CBasePlayer *PK = (CBasePlayer*)ep;

		char kiltext[1024] = "You killed a person.\n";
		UTIL_SayText( kiltext, PK );

		PK->m_flNextDecalTime = gpGlobals->time;
	}
	else
	{
		// World did them in, Genuflect.
	}
	}
}

BOOL CTestplay::IsTest( void )
{
	return TRUE;
}
