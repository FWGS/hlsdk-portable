// Shitty Co-Op Implimention

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"gamerules.h"
#include	"coop_gamerules.h"
#include	"game.h"

extern DLL_GLOBAL BOOL		g_fGameOver;
extern int gmsgScoreInfo;
extern int gmsgShowGameTitle;

CCoopplay :: CCoopplay()
{
 //Genuflect
}

BOOL CCoopplay::IsDeathmatch()
{
 return FALSE;
}

void CCoopplay::PlayerSpawn( CBasePlayer *pPlayer )
{
	BOOL		addDefault;
	CBaseEntity	*pWeaponEntity = NULL;

	pPlayer->pev->weapons |= (1<<WEAPON_SUIT);
	
	addDefault = TRUE;

	edict_t *pClient = g_engfuncs.pfnPEntityOfEntIndex( 1 );

	while ( pWeaponEntity = UTIL_FindEntityByClassname( pWeaponEntity, "game_player_equip" ))
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
}

void CCoopplay::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	//Nothing
}

BOOL CCoopplay::IsCoOp( void )
{
	return TRUE;
}