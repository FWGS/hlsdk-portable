/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
//
// multi_gamerules.h
//

//=========================================================
// CHalfLifeMultiplay - rules for the basic half life multiplayer
// competition
//=========================================================
//++ BulliT
#if !defined(_MULTIPLAY_H_)
#define _MULTIPLAY_H_
#include "aggamerules.h"
//-- Martin Webrant


//=========================================================
// CHalfLifeMultiplay - rules for the basic half life multiplayer
// competition
//=========================================================
//++ BulliT
class CHalfLifeMultiplay : public AgGameRules
//class CHalfLifeMultiplay : public CGameRules
//-- Martin Webrant
{
public:
	CHalfLifeMultiplay();

// GR_Think
	virtual void Think( void );
	virtual void RefreshSkillData( void );
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual BOOL FAllowFlashlight( void );

	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );

// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer( void );
	virtual BOOL IsDeathmatch( void );
	virtual BOOL IsCoOp( void );

// Client connection/disconnection
	// If ClientConnected returns FALSE, the connection is rejected and the user is provided the reason specified in
	//  svRejectReason
	// Only the client's name and remote address are provided to the dll for verification.
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );
	virtual void UpdateGameMode( CBasePlayer *pPlayer );  // the client needs to be informed of the current game mode

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual BOOL  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );

// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual edict_t *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	virtual BOOL AllowAutoTargetCrosshair( void );
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );

// Weapon retrieval
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );// The player is touching an CBasePlayerItem, do I give it to him?

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );

// Item retrieval
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );

// Ammo retrieval
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );

// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );
	virtual float FlHEVChargerRechargeTime( void );

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

// Teamplay stuff	
	virtual const char *GetTeamID( CBaseEntity *pEntity ) {return "";}
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	virtual BOOL PlayTextureSounds( void ) { return FALSE; }
	virtual BOOL PlayFootstepSounds( CBasePlayer *pl, float fvol );

// Monsters
	virtual BOOL FAllowMonsters( void );

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }

protected:
	virtual void ChangeLevel( void );
	virtual void GoToIntermission( void );
	float m_flIntermissionEndTime;
	BOOL m_iEndIntermissionButtonHit;
	void SendMOTDToClient( edict_t *client );
};

#endif //_MULTIPLAY_H_
