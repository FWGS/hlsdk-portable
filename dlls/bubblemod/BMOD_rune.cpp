// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//        Tyler Lund <halflife@bubblemod.org>
//
// LICENSE                                                            
//                                                                    
//        Permission is granted to anyone to use this  software  for  
//        any purpose on any computer system, and to redistribute it  
//        in any way, subject to the following restrictions:          
//                                                                    
//        1. The author is not responsible for the  consequences  of  
//           use of this software, no matter how awful, even if they  
//           arise from defects in it.                                
//        2. The origin of this software must not be misrepresented,  
//           either by explicit claim or by omission.                 
//        3. Altered  versions  must  be plainly marked as such, and  
//           must  not  be  misrepresented  (by  explicit  claim  or  
//           omission) as being the original software.                
//        3a. It would be nice if I got  a  copy  of  your  improved  
//            version  sent to halflife@bubblemod.org. 
//        4. This notice must not be removed or altered.              
//                                                                    
// ---------------------------------------------------------------

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "BMOD_rune.h"
#include "BMOD_messaging.h"
#include "BMOD_player.h"

#define RUNE_MODEL "models/w_oxygen.mdl"

extern DLL_GLOBAL BOOL		g_runes_learn;
extern cvar_t	bm_runemask;

void CRune::Spawn( void )
{
	Precache();
	// SET_MODEL(ENT(pev), RUNE_MODEL);
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_TRIGGER;
	pev->classname = MAKE_STRING("rune");

	pev->renderfx |= kRenderFxGlowShell;
	pev->rendercolor = m_vRuneColor;
	pev->renderamt = 50; 

	m_randomize = FALSE;
	
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch( &CRune::RuneTouch );

	pev->effects |= EF_NODRAW;
	SetThink( &CRune::Materialize );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(.5, 2); 	

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ), (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

void CRune::RuneTouch( CBaseEntity *pOther )
{

	// if it's not a player, just bounce
	if ( !pOther->IsPlayer() )
	{

		if (!pev->velocity)
			return;

		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin - Vector(0,0,10), ignore_monsters, edict(), &tr );

		if ( tr.flFraction < 1.0f )
		{
			// add a bit of static friction
			pev->velocity = pev->velocity * 0.5f;
		}
		return;
	}


	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	if (!pPlayer->BMOD_IsASpawn() && MyTouch( pPlayer ))
	{
		pPlayer->pev->rendermode = kRenderNormal;
		pPlayer->pev->renderfx = kRenderFxGlowShell;
		pPlayer->pev->rendercolor = m_vRuneColor;  
        pPlayer->pev->renderamt = 10;  // glow shell distance from entity

		SetTouch( NULL );
		Respawn(); 
	}
}

CBaseEntity* CRune::Respawn( void )
{
	pev->effects |= EF_NODRAW;

	if (m_randomize)
		SetThink( &CRune::MaterializeRandom );
	else
		SetThink( &CRune::Materialize );

	return this;
}

void CRune::MaterializeRandom ( void )
{
	// Make sure at least one rune type is allowed
	if (!((int)bm_runemask.value & 63)) 
	{
		UTIL_Remove( this );
		return;
	}

	CBaseEntity *NewRune = NULL;
	int rand = 1;

	for (int i = 0;  i < RANDOM_LONG(3,10); i++) 
	{
		do {
			rand *= 2;
			if (rand > 63)
				rand = 1;
		} while (!(rand & (int)bm_runemask.value)); 
	}

	if      (rand == RUNE_CROWBAR) 
		NewRune = CBaseEntity::Create("item_CrowbarRune", pev->origin, Vector (0,0,0), NULL );
	else if (rand == RUNE_GRENADE) 
		NewRune = CBaseEntity::Create("item_GrenadeRune", pev->origin, Vector (0,0,0), NULL );
	else if (rand == RUNE_357) 
		NewRune = CBaseEntity::Create("item_357Rune", pev->origin, Vector (0,0,0), NULL );
	else if (rand == RUNE_HEALTH) 
		NewRune = CBaseEntity::Create("item_HealthRune", pev->origin, Vector (0,0,0), NULL );
	else if (rand == RUNE_BATTERY) 
		NewRune = CBaseEntity::Create("item_BatteryRune", pev->origin, Vector (0,0,0), NULL );
	else if (rand == RUNE_SHOTGUN) 
		NewRune = CBaseEntity::Create("item_ShotgunRune", pev->origin, Vector (0,0,0), NULL );

	((CRune *)NewRune)->m_randomize = TRUE;
	UTIL_Remove( this );
}

void CRune::Materialize( void )
{
	// Pool of random spawn points.
	static Vector spawnPoints[200];

	// Populate the random spawn point pool.
	if (!g_runes_learn) {
		g_runes_learn = TRUE;
		CBaseEntity *pSpot = NULL;
		for ( int i = 0; i < 200; i++ ) {
			do 
				pSpot = UTIL_FindEntityInSphere( pSpot, Vector(0,0,0), 4096 );
			while (pSpot==NULL || 
				!(
					!strcmp(STRING(pSpot->pev->classname), "info_player_deathmatch") |
					!strncmp(STRING(pSpot->pev->classname), "weapon_", 7) |
					!strncmp(STRING(pSpot->pev->classname), "ammo_", 5)
				));
			spawnPoints[i] = pSpot->pev->origin;
		}

	}

	if ( pev->effects & EF_NODRAW )
	{
		pev->angles.x = 0;
		pev->angles.z = 0;
		pev->origin = spawnPoints[RANDOM_LONG(0, 199)];

		pev->velocity.x = RANDOM_FLOAT( .5, 2 );
		if (RANDOM_LONG(0,1))
			pev->velocity.x = pev->velocity.x * -1;

		pev->velocity.y = RANDOM_FLOAT( .5, 2 );
		if (RANDOM_LONG(0,1))
			pev->velocity.y = pev->velocity.y * -1;

		pev->velocity.z = RANDOM_FLOAT( .5, 2 );
		pev->avelocity.y = RANDOM_FLOAT( 0, 100 );
		pev->velocity = pev->velocity.Normalize() * 300;

		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;

		SetTouch( &CRune::RuneTouch );
	}
	// EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "buttons/blip2.wav", 1, ATTN_NORM, 0, 150 );
	pev->nextthink = gpGlobals->time + 2.0f;
}

void CRune::Precache( void )
{
	PRECACHE_MODEL (RUNE_MODEL);
	PRECACHE_SOUND ("buttons/blip2.wav");
}

LINK_ENTITY_TO_CLASS( item_rune, CRune );

////////////////////////////////////////////////
//  Crowbar rune
////////////////////////////////////////////////
#define CROWBARRUNE_MODEL "models/w_crowbar.mdl"
class CCrowbarRune : public CRune
{
	void Spawn ( void )
	{
		m_vRuneColor = Vector(255,20,20);
		SET_MODEL(ENT(pev), CROWBARRUNE_MODEL);
		CRune::Spawn();

	}
	void Precache( void )
	{
		PRECACHE_MODEL (CROWBARRUNE_MODEL);
	}
	BOOL MyTouch (CBasePlayer *pPlayer ) 
	{ 
		// One rune at a time folks.
		if (!pPlayer->m_RuneFlags)
		{
			float time = Q_max(1, CVAR_GET_FLOAT("bm_rune_cbar_t"));
			float respawn = Q_max(1, CVAR_GET_FLOAT("bm_rune_cbar_r"));

			//RuneMsg( pPlayer, MSG_RUNE_CROWBAR, m_vRuneColor, time - .5);
			PrintMessage( pPlayer, BMOD_CHAN_RUNE, m_vRuneColor, Vector (0.1f, time - 0.5f, 0.1f), "SUPER CROWBAR\nIncreased crowbar damage / infinite throws.");

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM ); 
			pPlayer->m_RuneFlags = RUNE_CROWBAR;
			pPlayer->m_RuneTime = gpGlobals->time + time;
			pev->nextthink = gpGlobals->time + respawn;
			return TRUE; 
		}
		else
			return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( item_CrowbarRune, CCrowbarRune );

////////////////////////////////////////////////
//  Grenade rune
////////////////////////////////////////////////
#define GRENADERUNE_MODEL "models/w_grenade.mdl"
class CGrenadeRune : public CRune
{
	void Spawn ( void )
	{
		m_vRuneColor = Vector(255,128,0);
		SET_MODEL(ENT(pev), GRENADERUNE_MODEL);
		CRune::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL (GRENADERUNE_MODEL);
	}
	BOOL MyTouch (CBasePlayer *pPlayer ) 
	{ 
		// One rune at a time folks.
		if (!pPlayer->m_RuneFlags)
		{
			float time = Q_max(1, CVAR_GET_FLOAT("bm_rune_gren_t"));
			float respawn = Q_max(1, CVAR_GET_FLOAT("bm_rune_gren_r"));

			PrintMessage( pPlayer, BMOD_CHAN_RUNE, m_vRuneColor, Vector (0.1f, time - 0.5f, 0.1f), "MEGA GRENADE\nIncreased hand grenade damage.");
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM ); 
			pPlayer->m_RuneFlags = RUNE_GRENADE;
			pPlayer->m_RuneTime = gpGlobals->time + time;
			pev->nextthink = gpGlobals->time + respawn;
			return TRUE; 
		}
		else
			return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( item_GrenadeRune, CGrenadeRune );


////////////////////////////////////////////////
//  Health rune
////////////////////////////////////////////////
#define HEALTHRUNE_MODEL "models/w_medkit.mdl"
class CHealthRune : public CRune
{
	void Spawn ( void )
	{
		m_vRuneColor = Vector(20,255,20);
		SET_MODEL(ENT(pev), HEALTHRUNE_MODEL);
		CRune::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL (HEALTHRUNE_MODEL);
	}
	BOOL MyTouch (CBasePlayer *pPlayer ) 
	{ 
		// One rune at a time folks.
		if (!pPlayer->m_RuneFlags)
		{
			float time = Q_max(1, CVAR_GET_FLOAT("bm_rune_health_t"));
			float respawn = Q_max(1, CVAR_GET_FLOAT("bm_rune_health_r"));
			PrintMessage( pPlayer, BMOD_CHAN_RUNE, m_vRuneColor, Vector (0.1f, time - 0.5f, 0.1f), "DOUBLE HEALTH PICKUPS\nMed packs / machines give 2X health. Fast Heal on Bubble Gun.");

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM ); 
			pPlayer->m_RuneFlags = RUNE_HEALTH;
			pPlayer->m_RuneTime = gpGlobals->time + time;
			pev->nextthink = gpGlobals->time + respawn;
			return TRUE; 
		}
		else
			return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( item_HealthRune, CHealthRune );

////////////////////////////////////////////////
//  Battery rune
////////////////////////////////////////////////
#define BATTERYRUNE_MODEL "models/w_battery.mdl"
class CBatteryRune : public CRune
{
	void Spawn ( void )
	{
		m_vRuneColor = Vector(0,255,255);
		SET_MODEL(ENT(pev), BATTERYRUNE_MODEL);
		CRune::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL (BATTERYRUNE_MODEL);
	}
	BOOL MyTouch (CBasePlayer *pPlayer ) 
	{ 
		// One rune at a time folks.
		if (!pPlayer->m_RuneFlags)
		{
			float time = Q_max(1, CVAR_GET_FLOAT("bm_rune_armor_t"));
			float respawn = Q_max(1, CVAR_GET_FLOAT("bm_rune_armor_r"));

			PrintMessage( pPlayer, BMOD_CHAN_RUNE, m_vRuneColor, Vector (0.1f, time - 0.5f, 0.1f), "DOUBLE ARMOR PICKUPS\nArmor batteries / machines give 2X armor. Bubble Gun gives armor.");
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM ); 
			pPlayer->m_RuneFlags = RUNE_BATTERY;
			pPlayer->m_RuneTime = gpGlobals->time + time;
			pev->nextthink = gpGlobals->time + respawn;
			return TRUE; 
		}
		else
			return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( item_BatteryRune, CBatteryRune );

////////////////////////////////////////////////
//  357 rune
////////////////////////////////////////////////
#define _357RUNE_MODEL "models/w_357.mdl"
class C357Rune : public CRune
{
	void Spawn ( void )
	{
		m_vRuneColor = Vector(250,20,250);
		SET_MODEL(ENT(pev), _357RUNE_MODEL);
		CRune::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL (_357RUNE_MODEL);
	}
	BOOL MyTouch (CBasePlayer *pPlayer ) 
	{ 
		// One rune at a time folks.
		if (!pPlayer->m_RuneFlags)
		{
			float time = Q_max(1, CVAR_GET_FLOAT("bm_rune_357_t"));
			float respawn = Q_max(1, CVAR_GET_FLOAT("bm_rune_357_r"));

			PrintMessage( pPlayer, BMOD_CHAN_RUNE, m_vRuneColor, Vector (0.1f, time - 0.5f, 0.1f), "SUPER MAGNUM\nIncreased magnum damage.");
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM ); 
			pPlayer->m_RuneFlags = RUNE_357;
			pPlayer->m_RuneTime = gpGlobals->time + time;
			pev->nextthink = gpGlobals->time + respawn;
			return TRUE; 
		}
		else
			return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( item_357Rune, C357Rune );

////////////////////////////////////////////////
//  Shotgun rune
////////////////////////////////////////////////
#define SHOTGUNRUNE_MODEL "models/w_shotgun.mdl"
class CShotgunRune : public CRune
{
	void Spawn ( void )
	{
		m_vRuneColor = Vector(20,20,255);
		SET_MODEL(ENT(pev), SHOTGUNRUNE_MODEL);
		CRune::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL (SHOTGUNRUNE_MODEL);
	}
	BOOL MyTouch (CBasePlayer *pPlayer ) 
	{ 
		// One rune at a time folks.
		if (!pPlayer->m_RuneFlags)
		{
			float time = Q_max(1, CVAR_GET_FLOAT("bm_rune_shotty_t"));
			float respawn = Q_max(1, CVAR_GET_FLOAT("bm_rune_shotty_r"));

			PrintMessage( pPlayer, BMOD_CHAN_RUNE, m_vRuneColor, Vector (0.1f, time - 0.5f, 0.1f), "SUPER SHOTGUN\nIncreased shotgun speed / fast reload.");
			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM ); 
			pPlayer->m_RuneFlags = RUNE_SHOTGUN;
			pPlayer->m_RuneTime = gpGlobals->time + time;
			pev->nextthink = gpGlobals->time + respawn;
			return TRUE; 
		}
		else
			return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( item_ShotgunRune, CShotgunRune );
