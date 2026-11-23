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
/*

===== player.cpp ========================================================

  functions dealing with the player

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "client.h"
#include "weapons.h"
#include "gamerules.h"
#include "game.h"
#include "soundent.h"
#include "monsters.h"
#include "shake.h"
#include "hltv.h"
#include "pm_shared.h"
#include "decals.h"
#include "triggers.h"
#include "nodes.h"
#include "czeror/cz_music.h"

// counterterrorist.cpp
extern void CZ_SpawnSandstormCTs( void );
extern void CZ_EquipDirectStart( CBasePlayer *pPlayer );

#define HITGROUP_SHIELD		8
#define PLAYER_HUD_DAMAGE_MASK	0x774118
#define PLAYER_LARGE_FLINCH_WEAPONS	( ( 1 << WEAPON_SCOUT ) | ( 1 << WEAPON_AUG ) | ( 1 << WEAPON_SG550 ) \
	| ( 1 << WEAPON_AWP ) | ( 1 << WEAPON_M3 ) | ( 1 << WEAPON_M4A1 ) | ( 1 << WEAPON_G3SG1 ) \
	| ( 1 << WEAPON_DEAGLE ) | ( 1 << WEAPON_SG552 ) | ( 1 << WEAPON_AK47 ) | ( 1 << WEAPON_FAMAS ) | ( 1 << WEAPON_GALIL ) )

int gEvilImpulse101;
BOOL gInitHUD = TRUE;
int LEVEL_BonusLevels = LEVEL_Truth | LEVEL_Run | LEVEL_Miami | LEVEL_Fastline | LEVEL_Pipedream | LEVEL_Sandstorm;
int giPrecacheGrunt;
int gmsgShake;
int gmsgFade;
int gmsgSelAmmo;
int gmsgFlashlight;
int gmsgFlashBattery;
int gmsgResetHUD;
int gmsgInitHUD;
int gmsgShowGameTitle;
int gmsgCurWeapon;
int gmsgHealth;
int gmsgDamage;
int gmsgBattery;
int gmsgTrain;
int gmsgLogo;
int gmsgWeaponList;
int gmsgAmmoX;
int gmsgHudText;
int gmsgDeathMsg;
int gmsgScoreInfo;
int gmsgTeamInfo;
int gmsgTeamScore;
int gmsgGameMode;
int gmsgMOTD;
int gmsgServerName;
int gmsgAmmoPickup;
int gmsgWeapPickup;
int gmsgItemPickup;
int gmsgHideWeapon;
int gmsgSetCurWeap;
int gmsgSayText;
int gmsgTextMsg;
int gmsgSetFOV;
int gmsgShowMenu;
int gmsgGeigerRange;
int gmsgTeamNames;
int gmsgReceiveW;
int gmsgStatusText;
int gmsgStatusValue;
int gmsgBarTime;
int gmsgParticle;
int gmsgPartState;
int gmsgAccesBits;
int gmsgNightVision;
int gmsgSniperLaser;
int gmsgFiberCamera;
int gmsgSecurityCamera;
int gmsgHudIcons;
int gmsgLevelLocks;
int gmsgWorldMap;
int gmsgWindInfo;
int gmsgGrassInfo;

/*
 * GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
 *
 * ENTITY_METHOD(PlayerDie)
 */
entvars_t* g_pevLastInflictor;  // Set in combat.cpp.  Used to pass the damage inflictor for death messages.
CBaseEntity* g_pLastSpawn;

extern int gDisplayTitle;
extern int g_iSkillLevel;
extern BOOL g_fGameOver;

// the world node graph
extern CGraph WorldGraph;
extern cvar_t forcerespawn;
extern cvar_t decalfrequency;
extern void CopyToBodyQue( entvars_t* pev );
extern ULONG g_ulModelIndexPlayer;

int CBasePlayer::gLevelLocks = LEVEL_BonusLevels | LEVEL_Training | LEVEL_Recoil;

// Global Savedata for player
TYPEDESCRIPTION CBasePlayer::m_playerSaveData[] =
{
	DEFINE_FIELD( CBasePlayer, m_flFlashLightTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iFlashBattery, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonLast, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonPressed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_afButtonReleased, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgItems, FIELD_INTEGER, 5 ),
	DEFINE_FIELD( CBasePlayer, m_afPhysicsFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flTimeStepSound, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flTimeWeaponIdle, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSwimTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flDuckTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flWallJumpTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flSuitUpdate, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_rgSuitPlayList, FIELD_INTEGER, CSUITPLAYLIST ),
	DEFINE_FIELD( CBasePlayer, m_iSuitPlayNext, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgiSuitNoRepeat, FIELD_INTEGER, CSUITNOREPEAT ),
	DEFINE_ARRAY( CBasePlayer, m_rgflSuitNoRepeatTime, FIELD_TIME, CSUITNOREPEAT ),
	DEFINE_FIELD( CBasePlayer, m_lastDamageAmount, FIELD_INTEGER ),
	DEFINE_ARRAY( CBasePlayer, m_rgpPlayerItems, FIELD_CLASSPTR, 7 ),
	DEFINE_FIELD( CBasePlayer, m_pActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pClientActiveItem, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_pLastItem, FIELD_CLASSPTR ),
	DEFINE_ARRAY( CBasePlayer, m_rgAmmo, FIELD_INTEGER, 32 ),
	DEFINE_FIELD( CBasePlayer, m_idrowndmg, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_idrownrestored, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_tSneaking, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iTrain, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bitsHUDDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iTargetVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponVolume, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iExtraSoundTypes, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iWeaponFlash, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_fLongJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_tbdPrev, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_pTank, FIELD_EHANDLE ),
	DEFINE_FIELD( CBasePlayer, m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iFOV, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iIncognito, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iAccessoryBits, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iTeam, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, random_seed, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flFlinchTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flAnimTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_bHighDamage, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_flVelocityModifier, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iLastZoom, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bResumeZoom, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_flEjectBrass, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iKevlar, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bNotKilled, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_bHasPrimary, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_flDeathThrowTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_iThrowDirection, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flLastTalk, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_bJustConnected, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_fLastMovement, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_bHasNightVision, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_bNightVisionOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_flIdleCheckTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iHostagesKilled, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bIsDefusing, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_bFirstConnected, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_bFirstRestore, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_iHudIcons, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iHudIconsCorrectWeapon, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_iHudIconsInPosition, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_stealthTrigger, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_toolTrigger, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_cameraTrigger, FIELD_CLASSPTR ),
	DEFINE_FIELD( CBasePlayer, m_lastx, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_lasty, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_flStartCharge, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flAmmoStartCharge, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flPlayAftershock, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_flNextAmmoBurn, FIELD_FLOAT ),
	DEFINE_ARRAY( CBasePlayer, m_izSBarState, FIELD_INTEGER, SBAR_END ),
	DEFINE_FIELD( CBasePlayer, m_flNextSBarUpdateTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_flStatusBarDisappearDelay, FIELD_TIME ),
	DEFINE_ARRAY( CBasePlayer, m_SbarString0, FIELD_CHARACTER, SBAR_STRING_SIZE ),
	DEFINE_ARRAY( CBasePlayer, m_SbarString1, FIELD_CHARACTER, SBAR_STRING_SIZE ),
	DEFINE_FIELD( CBasePlayer, m_flNextChatTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_iAutoWepSwitch, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_bVGUIMenus, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_bShowHints, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_bShieldDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_bLeftHanded, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_defuseTime, FIELD_FLOAT ),
	DEFINE_FIELD( CBasePlayer, m_fiberCamOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_fiberCamAngle, FIELD_VECTOR ),
	DEFINE_FIELD( CBasePlayer, m_fiberCamID, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_inFiberCam, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_endingMission, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBasePlayer, m_LevelLocks, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, savedHudBits, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, savedIconBits, FIELD_INTEGER ),
	DEFINE_FIELD( CBasePlayer, m_chatterTime, FIELD_TIME ),
	DEFINE_FIELD( CBasePlayer, m_forceEquip, FIELD_BOOLEAN )
};

LINK_ENTITY_TO_CLASS( player, CBasePlayer );

int CBasePlayer::Save( CSave& save )
{
	if ( !CBaseMonster::Save( save ) )
		return 0;
	return save.WriteFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );
}

int CBasePlayer::Restore( CRestore& restore )
{
	if ( !CBaseMonster::Restore( restore ) )
		return 0;
	int status = restore.ReadFields( "PLAYER", this, m_playerSaveData, ARRAYSIZE( m_playerSaveData ) );
	SAVERESTOREDATA* saveData = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	if ( !saveData->fUseLandmark )
	{
		ALERT( at_console, "No Landmark:%s\n", saveData->szLandmarkName );
		edict_t* spawn = EntSelectSpawnPoint( this );
		pev->origin = VARS( spawn )->origin + Vector( 0, 0, 1 );
		pev->angles = VARS( spawn )->angles;
	}
	pev->v_angle.z = 0;	// Clear out roll
	pev->angles = pev->v_angle;
	pev->fixangle = 1;           // turn this way immediately
	m_bloodColor = BLOOD_COLOR_RED;
	g_ulModelIndexPlayer = pev->modelindex;
	if ( pev->flags & FL_DUCKING )
		UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", m_fLongJump ? "1" : "0" );
	TabulateAmmo();
	m_bFirstRestore = TRUE;
	gLevelLocks = m_LevelLocks;
	return status;
}

void CBasePlayer::DeathSound()
{
	// water death sounds
	/*
	if (pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "player/h2odeath.wav", 1, ATTN_NONE);
		return;
	}
	*/

	// temporarily using pain sounds for death sounds
	switch ( RANDOM_LONG( 1, 5 ) )
	{
	case 1:
		EMIT_SOUND( edict(), CHAN_VOICE, "player/pl_pain5.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( edict(), CHAN_VOICE, "player/pl_pain6.wav", 1, ATTN_NORM );
		break;
	case 3:
		EMIT_SOUND( edict(), CHAN_VOICE, "player/pl_pain7.wav", 1, ATTN_NORM );
		break;
	}
}

BOOL CBasePlayer::HasWeapons()
{
	for ( int i = 0; i < ARRAYSIZE( m_rgpPlayerItems ); i++ )
	{
		if ( m_rgpPlayerItems[i] )
			return TRUE;
	}
	return FALSE;
}

BOOL CBasePlayer::IsOnLadder()
{
	return pev->movetype == MOVETYPE_FLY;
}

void CBasePlayer::StartSneaking()
{
	m_tSneaking = gpGlobals->time - 1;
}

void CBasePlayer::StopSneaking()
{
	m_tSneaking = gpGlobals->time + 30;
}

BOOL CBasePlayer::IsSneaking()
{
	return gpGlobals->time >= m_tSneaking;
}

int CBasePlayer::ObjectCaps()
{
	return 0;
}

void CBasePlayer::RenewItems()
{
}

int CBasePlayer::TakeHealth( float health, int damageType )
{
	return CBaseMonster::TakeHealth( health, damageType );
}

bool CBasePlayer::HintMessage( const char* message, BOOL displayIfDead, BOOL force )
{
	if ( !displayIfDead && !IsAlive() )
		return false;
	return ( !force && !m_bShowHints ) || m_hintMessageQueue.AddMessage( message );
}

void CBasePlayer::SetMaxSpeed( const float speed )
{
	g_engfuncs.pfnSetClientMaxspeed( edict(), speed );
}

BOOL CBasePlayer::FBecomeProne()
{
	m_afPhysicsFlags |= PFLAG_ONBARNACLE;
	return TRUE;
}

//=========================================================
// BarnacleVictimBitten - bad name for a function that is called
// by Barnacle victims when the barnacle pulls their head
// into its mouth. For the player, just die.
//=========================================================
void CBasePlayer::BarnacleVictimBitten( entvars_t* barnacle )
{
	TakeDamage( barnacle, barnacle, pev->health + pev->armorvalue, DMG_SLASH | DMG_ALWAYSGIB );
}

//=========================================================
// BarnacleVictimReleased - overridden for player who has
// physics flags concerns. 
//=========================================================
void CBasePlayer::BarnacleVictimReleased()
{
	m_afPhysicsFlags &= ~PFLAG_ONBARNACLE;
}

int CBasePlayer::GetAmmoIndex( const char* name )
{
	if ( !name )
		return -1;
	for ( int i = 1; i < ARRAYSIZE( CBasePlayerItem::AmmoInfoArray ); i++ )
	{
		if ( CBasePlayerItem::AmmoInfoArray[i].pszName && !stricmp( name, CBasePlayerItem::AmmoInfoArray[i].pszName ) )
			return i;
	}
	return -1;
}

int CBasePlayer::AmmoInventory( int index )
{
	return index == -1 ? -1 : m_rgAmmo[index];
}

int CBasePlayer::Illumination()
{
	int illumination = m_iWeaponFlash + GETENTITYILLUM( edict() );
	return illumination > 255 ? 255 : illumination;
}

void CBasePlayer::ResetAutoaim()
{
	if ( m_vecAutoAim.x != 0 || m_vecAutoAim.y != 0 )
	{
		m_vecAutoAim = g_vecZero;
		SET_CROSSHAIRANGLE( edict(), 0, 0 );
	}
	m_fOnTarget = FALSE;
}

Vector CBasePlayer::GetAutoaimVector( float delta )
{
	UTIL_MakeVectors( pev->v_angle + pev->punchangle );
	return gpGlobals->v_forward;
}

Vector CBasePlayer::AutoaimDeflection( Vector& source, float distance, float delta )
{
	edict_t* target = INDEXENT( 1 );
	if ( g_psv_aim->value == 0 )
	{
		m_fOnTarget = FALSE;
		return g_vecZero;
	}
	UTIL_MakeVectors( pev->v_angle + pev->punchangle + m_vecAutoAim );
	Vector bestDirection = gpGlobals->v_forward;
	m_fOnTarget = FALSE;
	TraceResult trace;
	UTIL_TraceLine( source, source + bestDirection * distance, dont_ignore_monsters, edict(), &trace );
	if ( trace.pHit && trace.pHit->v.takedamage != DAMAGE_NO )
	{
		if ( !( ( pev->waterlevel != 3 && trace.pHit->v.waterlevel == 3 )
			|| ( pev->waterlevel == 3 && trace.pHit->v.waterlevel == 0 ) ) )
		{
			if ( trace.pHit->v.takedamage == DAMAGE_AIM )
				m_fOnTarget = TRUE;
			return m_vecAutoAim;
		}
	}
	edict_t* bestTarget = NULL;
	for ( int i = 1; i < gpGlobals->maxEntities; i++, target++ )
	{
		if ( target->free || target->v.takedamage != DAMAGE_AIM || target == edict() )
			continue;
//		if (pev->team > 0 && pEdict->v.team == pev->team)
//			continue;	// don't aim at teammate
		if ( !g_pGameRules->ShouldAutoAim( this, target ) )
			continue;
		CBaseEntity* entity = Instance( target );
		if ( !entity || !entity->IsAlive() )
			continue;
		if ( ( pev->waterlevel == 3 && entity->pev->waterlevel == 0 )
			|| ( pev->waterlevel != 3 && entity->pev->waterlevel == 3 ) )
			continue;
		Vector center = entity->BodyTarget( source );
		Vector direction = ( center - source ).Normalize();
		if ( DotProduct( direction, gpGlobals->v_forward ) < 0 )
			continue;
		float dot = fabs( DotProduct( direction, gpGlobals->v_up ) ) * 0.5
			+ fabs( DotProduct( direction, gpGlobals->v_right ) );
		dot *= ( center - source ).Length() / distance * 0.2 + 1;
		if ( dot > delta )
			continue;
		UTIL_TraceLine( source, center, dont_ignore_monsters, edict(), &trace );
		if ( trace.flFraction != 1 && trace.pHit != target )
			continue;
		if ( IRelationship( entity ) < 0 && !entity->IsPlayer() && !g_pGameRules->IsDeathmatch() )
			continue;
		bestDirection = direction;
		bestTarget = target;
		delta = dot;
	}
	if ( bestTarget )
	{
		bestDirection = UTIL_VecToAngles( bestDirection );
		bestDirection.x = -bestDirection.x;
		bestDirection = bestDirection - pev->v_angle - pev->punchangle;
		if ( bestTarget->v.takedamage == DAMAGE_AIM )
			m_fOnTarget = TRUE;
		return bestDirection;
	}
	return Vector( 0, 0, 0 );
}

void CBasePlayer::ItemPreFrame()
{
	if ( m_flNextAttack <= gpGlobals->time && m_pActiveItem )
		m_pActiveItem->ItemPreFrame();
}

BOOL CBasePlayer::SwitchWeapon( CBasePlayerItem* weapon )
{
	if ( !weapon->CanDeploy() )
		return FALSE;
	ResetAutoaim();
	if ( m_pActiveItem )
		m_pActiveItem->Holster();
	m_pLastItem = m_pActiveItem;
	m_pActiveItem = weapon;
	weapon->Deploy();
	return TRUE;
}

void CBasePlayer::EnableControl( BOOL control )
{
	if ( control )
		pev->flags &= ~FL_FROZEN;
	else
		pev->flags |= FL_FROZEN;
}

BOOL CBasePlayer::FlashlightIsOn()
{
	return pev->effects & EF_DIMLIGHT;
}

void CBasePlayer::FlashlightTurnOn()
{
	if ( !g_pGameRules->FAllowFlashlight() || !( pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return;
	EMIT_SOUND( edict(), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1, ATTN_NORM );
	pev->effects |= EF_DIMLIGHT;
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE( 1 );
		WRITE_BYTE( m_iFlashBattery );
	MESSAGE_END();
	m_flFlashLightTime = gpGlobals->time + 1.2;
}

void CBasePlayer::FlashlightTurnOff()
{
	EMIT_SOUND( edict(), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1, ATTN_NORM );
	pev->effects &= ~EF_DIMLIGHT;
	MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( m_iFlashBattery );
	MESSAGE_END();
	m_flFlashLightTime = gpGlobals->time + 0.2;
}

BOOL CBasePlayer::NightVisionIsOn()
{
	return pev->effects & EF_NIGHTVISION;
}

void CBasePlayer::NightVisionTurnOn()
{
	if ( !m_bHasNightVision )
		return;
	EMIT_SOUND( edict(), CHAN_WEAPON, SOUND_FLASHLIGHT_ON, 1, ATTN_NORM );
	pev->effects |= EF_NIGHTVISION;
	MESSAGE_BEGIN( MSG_ONE, gmsgNightVision, NULL, pev );
		WRITE_BYTE( 1 );
	MESSAGE_END();
}

void CBasePlayer::NightVisionTurnOff()
{
	EMIT_SOUND( edict(), CHAN_WEAPON, SOUND_FLASHLIGHT_OFF, 1, ATTN_NORM );
	pev->effects &= ~EF_NIGHTVISION;
	MESSAGE_BEGIN( MSG_ONE, gmsgNightVision, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();
}

BOOL CBasePlayer::SniperLaserIsOn()
{
	return pev->effects & EF_SNIPERLASER;
}

void CBasePlayer::SniperLaserTurnOn()
{
	pev->effects |= EF_SNIPERLASER;
}

void CBasePlayer::SniperLaserTurnOff()
{
	pev->effects &= ~EF_SNIPERLASER;
}

void CBasePlayer::StartFiberCamera( Vector origin, Vector angles, const int entityID )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgFiberCamera, NULL, pev );
		WRITE_BYTE( 1 );
		WRITE_SHORT( entityID );
		WRITE_COORD( origin.x );
		WRITE_COORD( origin.y );
		WRITE_COORD( origin.z );
		WRITE_COORD( angles.x );
		WRITE_COORD( angles.y );
		WRITE_COORD( angles.z );
	MESSAGE_END();
	pev->iuser1 = OBS_CHASE_FREE;
	pev->iuser2 = entityID;
	SET_VIEW( edict(), INDEXENT( entityID ) );
	m_fiberCamOrigin = origin;
	m_fiberCamAngle = angles;
	m_fiberCamID = entityID;
	m_inFiberCam = TRUE;
}

void CBasePlayer::StopFiberCamera()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgFiberCamera, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_SHORT( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
	MESSAGE_END();
	pev->iuser1 = 0;
	pev->iuser2 = 0;
	SET_VIEW( edict(), edict() );
	m_fiberCamID = 0;
	m_fiberCamOrigin = g_vecZero;
	m_fiberCamAngle = g_vecZero;
	m_inFiberCam = FALSE;
}

void CBasePlayer::SendAmmoUpdate()
{
	for ( int i = 0; i < ARRAYSIZE( m_rgAmmo ); i++ )
	{
		if ( m_rgAmmo[i] == m_rgAmmoLast[i] )
			continue;
		m_rgAmmoLast[i] = m_rgAmmo[i];

		// send "Ammo" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgAmmoX, NULL, pev );
			WRITE_BYTE( i );
			WRITE_BYTE( max( 0, min( m_rgAmmo[i], 254 ) ) );  // clamp the value to one byte
		MESSAGE_END();
	}
}

void CBasePlayer::SetProgressBarTime( int time )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, pev );
		WRITE_SHORT( time );
	MESSAGE_END();
	CBaseEntity* entity = NULL;
	int index = entindex();
	while ( ( entity = UTIL_FindEntityByClassname( entity, "player" ) ) != NULL )
	{
		if ( FNullEnt( entity->edict() ) )
			break;
		CBasePlayer* player = GetClassPtr( (CBasePlayer*)entity->pev );
		if ( player->pev->iuser1 == OBS_IN_EYE && player->pev->iuser2 == index )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgBarTime, NULL, player->pev );
				WRITE_SHORT( time );
			MESSAGE_END();
		}
	}
}

void CBasePlayer::SetCustomDecalFrames( int frames )
{
	m_nCustomSprayFrames = (unsigned int)( frames - 1 ) > 6 ? -1 : frames;
}

/*
=============
GetCustomDecalFrames

  Returns the # of custom frames this player's custom clan logo contains.
=============
*/
int CBasePlayer::GetCustomDecalFrames()
{
	return m_nCustomSprayFrames;
}

int CBasePlayer::GetActiveWeaponClassname()
{
	return m_pActiveItem ? m_pActiveItem->pev->classname : 0;
}

const char* CBasePlayer::TeamID()
{
	return pev ? m_szTeamName : "";
}

BOOL CBasePlayer::IsAlive()
{
	return pev->deadflag == DEAD_NO && pev->health > 0;
}

int CBasePlayer::Classify()
{
	return CLASS_PLAYER;
}

Vector CBasePlayer::BodyTarget( const Vector& source )
{
	float scale = RANDOM_FLOAT( 0.5, 1.1 );
	return Center() + pev->view_ofs * scale;
}

BOOL CBasePlayer::IsArmored( int hitGroup )
{
	if ( !m_iKevlar )
		return FALSE;
	if ( hitGroup == HITGROUP_HEAD )
		return m_iKevlar == 2;
	return hitGroup >= HITGROUP_GENERIC && hitGroup <= HITGROUP_RIGHTARM;
}

BOOL CBasePlayer::ShouldDoLargeFlinch( int hitGroup, int gunType )
{
	if ( ( pev->flags & FL_DUCKING ) || hitGroup == HITGROUP_LEFTLEG || hitGroup == HITGROUP_RIGHTLEG )
		return FALSE;
	return (unsigned int)gunType <= WEAPON_GALIL && ( ( 1 << gunType ) & PLAYER_LARGE_FLINCH_WEAPONS ) != 0;
}

void CBasePlayer::UpdateShieldCrosshair( bool draw )
{
	if ( draw )
		m_iHideHUD &= ~HIDEHUD_CROSSHAIR;
	else
		m_iHideHUD |= HIDEHUD_CROSSHAIR;
}

void CBasePlayer::GiveNamedItem( const char* name )
{
	edict_t* entity = CREATE_NAMED_ENTITY( MAKE_STRING( name ) );
	if ( FNullEnt( entity ) )
	{
		ALERT( at_console, "NULL Ent in GiveNamedItem!\n" );
		return;
	}
	entity->v.origin = pev->origin;
	entity->v.spawnflags |= SF_NORESPAWN;
	DispatchSpawn( entity );
	DispatchTouch( entity, edict() );
}

BOOL CBasePlayer::HasPlayerItem( CBasePlayerItem* item )
{
	for ( CBasePlayerItem* current = m_rgpPlayerItems[item->iItemSlot()]; current; current = current->m_pNext )
	{
		if ( !strcmp( STRING( current->pev->classname ), STRING( item->pev->classname ) ) )
			return TRUE;
	}
	return FALSE;
}

BOOL CBasePlayer::HasNamedPlayerItem( const char* name )
{
	for ( int i = 0; i < ARRAYSIZE( m_rgpPlayerItems ); i++ )
	{
		for ( CBasePlayerItem* item = m_rgpPlayerItems[i]; item; item = item->m_pNext )
		{
			if ( !strcmp( name, STRING( item->pev->classname ) ) )
				return TRUE;
		}
	}
	return FALSE;
}

void CBasePlayer::SelectItem( const char* name )
{
	if ( !name )
		return;
	for ( int i = 0; i < ARRAYSIZE( m_rgpPlayerItems ); i++ )
	{
		for ( CBasePlayerItem* item = m_rgpPlayerItems[i]; item; item = item->m_pNext )
		{
			if ( strcmp( STRING( item->pev->classname ), name ) )
				continue;
			if ( m_pActiveItem == item )
				return;
			ResetAutoaim();

			// FIX, this needs to queue them up and delay
			if ( m_pActiveItem )
				m_pActiveItem->Holster();
			m_pLastItem = m_pActiveItem;
			m_pActiveItem = item;
			item->Deploy();
			m_pActiveItem->UpdateItemInfo();
			return;
		}
	}
}

void CBasePlayer::SelectPrevItem( int slot )
{
}

void CBasePlayer::SelectNextItem( int slot )
{
	CBasePlayerItem* item = m_rgpPlayerItems[slot];
	if ( !item )
		return;
	if ( item == m_pActiveItem )
	{
		if ( !item->m_pNext )
			return;
		CBasePlayerItem* next = item->m_pNext;
		CBasePlayerItem* tail = next;
		while ( tail->m_pNext )
			tail = tail->m_pNext;
		tail->m_pNext = item;
		m_pActiveItem->m_pNext = NULL;
		m_rgpPlayerItems[slot] = next;
		item = next;
	}
	ResetAutoaim();
	if ( m_pActiveItem )
		m_pActiveItem->Holster();
	m_pActiveItem = item;
	item->Deploy();
	m_pActiveItem->UpdateItemInfo();
}

BOOL CBasePlayer::SelectLastItem()
{
	if ( !m_pLastItem || ( m_pActiveItem && !m_pActiveItem->CanHolster() ) )
		return FALSE;
	ResetAutoaim();

	// FIX, this needs to queue them up and delay
	if ( m_pActiveItem )
		m_pActiveItem->Holster();
	CBasePlayerItem* item = m_pLastItem;
	m_pLastItem = m_pActiveItem;
	m_pActiveItem = item;
	item->Deploy();
	m_pActiveItem->UpdateItemInfo();
	return TRUE;
}

int CBasePlayer::AddPlayerItem( CBasePlayerItem* item )
{
	item->iItemSlot();
	if ( !item->AddToPlayer( this ) )
	{
		if ( gEvilImpulse101 )
			item->Kill();
		return 0;
	}
	g_pGameRules->PlayerGotWeapon( this, item );
	if ( item->iItemSlot() == 1 )
		m_bHasPrimary = TRUE;
	item->CheckRespawn();
	item->m_pNext = m_rgpPlayerItems[item->iItemSlot()];
	m_rgpPlayerItems[item->iItemSlot()] = item;
	if ( g_pGameRules->FShouldSwitchWeapon( this, item ) || m_forceEquip )
	{
		SwitchWeapon( item );
		m_forceEquip = FALSE;
	}
	return 1;
}

int CBasePlayer::RemovePlayerItem( CBasePlayerItem* item )
{
	if ( m_pActiveItem == item )
	{
		ResetAutoaim();
		m_pActiveItem->pev->nextthink = 0;
		m_pActiveItem->SetThink( NULL );
		m_pActiveItem = NULL;
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	if ( m_pLastItem == item )
		m_pLastItem = NULL;
	CBasePlayerItem* current = m_rgpPlayerItems[item->iItemSlot()];
	if ( item->m_bIsAccessory )
		m_iAccessoryBits &= ~( 1 << item->m_iId );
	else
		pev->weapons &= ~( 1 << item->m_iId );
	if ( item == current )
	{
		m_rgpPlayerItems[item->iItemSlot()] = item->m_pNext;
		return 1;
	}
	while ( current )
	{
		if ( current->m_pNext == item )
		{
			current->m_pNext = item->m_pNext;
			return 1;
		}
		current = current->m_pNext;
	}
	return 0;
}

int CBasePlayer::GiveAmmo( int count, const char* name, int maximum )
{
	if ( !name || !g_pGameRules->CanHaveAmmo( this, name, maximum ) )
		return -1;
	int index = GetAmmoIndex( name );
	if ( index < 0 )
		return -1;
	int amount = min( maximum - m_rgAmmo[index], count );
	if ( amount <= 0 )
		return index;
	m_rgAmmo[index] += amount;
	if ( gmsgAmmoPickup )  // make sure the ammo messages have been linked first
	{
		// Send the message that ammo has been picked up
		MESSAGE_BEGIN( MSG_ONE, gmsgAmmoPickup, NULL, pev );
			WRITE_BYTE( GetAmmoIndex( name ) );
			WRITE_BYTE( amount );
		MESSAGE_END();
	}
	TabulateAmmo();
	return index;
}

void CBasePlayer::FireWeapon()
{
	if ( m_pActiveItem && m_pActiveItem->IsWeapon() )
		((CBasePlayerWeapon*)m_pActiveItem)->PrimaryAttack();
}

void CBasePlayer::AltFireWeapon()
{
	if ( m_pActiveItem && m_pActiveItem->IsWeapon() )
		((CBasePlayerWeapon*)m_pActiveItem)->SecondaryAttack();
}

Vector CBasePlayer::GetGunPosition()
{
	return pev->origin + pev->view_ofs;
}

void CBasePlayer::ForceClientDllUpdate()
{
	m_iClientBattery = -1;
	m_iClientHealth = -1;
	m_fGameHUDInitialized = FALSE;
	m_iTrain |= 0xc0;
	m_fWeapon = FALSE;
	m_fKnownItem = FALSE;    // Force weaponinit messages.
	m_fInitHUD = TRUE;
	m_iOldAccessoryBits = 0;
	m_fNewAmmo = FALSE;
	m_iClientHideHUD = -1;
	memset( m_rgAmmoLast, 0, sizeof( m_rgAmmoLast ) );
	UpdateClientData();
}

void CBasePlayer::AddPoints( int score, BOOL allowNegative )
{
	if ( !allowNegative && score < 0 )
	{
		if ( pev->frags < 0 )		// Can't go more negative
			return;
		if ( -score > pev->frags )	// Will this go negative?
			score = (int)-pev->frags;
	}
	pev->frags += score;
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( entindex() );
		WRITE_SHORT( (int)pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( g_pGameRules->GetTeamIndex( m_szTeamName ) + 1 );
	MESSAGE_END();
}

void CBasePlayer::AddPointsToTeam( int score, BOOL allowNegative )
{
	int index = entindex();
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity* player = UTIL_PlayerByIndex( i );
		if ( player && i != index && g_pGameRules->PlayerRelationship( this, player ) == GR_TEAMMATE )
			player->AddPoints( score, allowNegative );
	}
}

void CBasePlayer::SetSecurityCamera( bool status, CTriggerCamera* camera )
{
	m_cameraTrigger = camera;
	MESSAGE_BEGIN( MSG_ONE, gmsgSecurityCamera, NULL, pev );
		WRITE_BYTE( status );
	MESSAGE_END();
}

void CBasePlayer::GetCTHelp( CBaseEntity* enemy )
{
	CBaseEntity* entity = NULL;
	while ( ( entity = UTIL_FindEntityInSphere( entity, pev->origin, 512 ) ) != NULL )
	{
		if ( !strstr( STRING( entity->pev->classname ), "monster_ct_" ) )
			continue;
		CBaseMonster* monster = entity->MyMonsterPointer();
		if ( !monster->m_hEnemy )
		{
			monster->m_hEnemy = enemy;
			monster->m_vecEnemyLKP = enemy->pev->origin;
			monster->SetConditions( bits_COND_NEW_ENEMY );
		}
	}
}

//Player ID
void CBasePlayer::InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString0[0] = 0;
	m_SbarString1[0] = 0;
}

void CBasePlayer::Activate()
{
	m_bInitClientMsgs = TRUE;
}

BOOL CBasePlayer::ShouldFadeOnDeath()
{
	return FALSE;
}

void CBasePlayer::Precache()
{
	m_igeigerRangePrev = 1000;
	m_bitsDamageType = 0;
	m_bitsHUDDamage = -1;
	m_iClientBattery = -1;
	m_iTrain = 0xc0;
	m_flgeigerRange = 1000;
	m_bFirstConnected = FALSE;
	if ( !gmsgSelAmmo )
		LinkUserMessages();
	m_iUpdateTime = 5;  // won't update for 1/2 a second
	if ( gInitHUD )
		m_fInitHUD = TRUE;
}

void CBasePlayer::Jump()
{
	if ( ( pev->flags & FL_WATERJUMP ) || pev->waterlevel > 1 || !( m_afButtonPressed & IN_JUMP )
		|| !( pev->flags & FL_ONGROUND ) || !pev->groundentity )
		return;
	UTIL_MakeVectors( pev->angles );

	// ClearBits(pev->flags, FL_ONGROUND);		// don't stairwalk

	SetAnimation( PLAYER_JUMP );
	if ( m_fLongJump && ( pev->button & IN_DUCK ) && pev->flDuckTime > 0 && pev->velocity.Length() > 50 )
		SetAnimation( PLAYER_SUPERJUMP );
	if ( pev->groundentity && ( pev->groundentity->v.flags & FL_CONVEYOR ) )
		pev->velocity = pev->velocity + pev->basevelocity;
}

void CBasePlayer::Duck()
{
	if ( ( pev->button & IN_DUCK ) && m_IdealActivity != ACT_LEAP )
		SetAnimation( PLAYER_WALK );
}

void CBasePlayer::ResetMaxSpeed()
{
	float speed;
	if ( g_pGameRules->IsMultiplayer() )
	{
		speed = 1;
		pev->velocity = g_vecZero;
	}
	else if ( m_bIsVIP )
	{
		speed = 227;
	}
	else if ( m_pActiveItem )
	{
		speed = m_pActiveItem->GetMaxSpeed();
		if ( speed == 1 )
			pev->velocity = g_vecZero;
	}
	else
	{
		speed = 240;
	}
	SetMaxSpeed( speed );
}

void CBasePlayer::DropActiveItem()
{
	if ( !m_pActiveItem )
		return;
	int slot = m_pActiveItem->iItemSlot();
	if ( slot == 1 || slot == 2 || slot == 5 )
		DropPlayerItem( "" );
}

void CBasePlayer::SendInitClientMessages()
{
	SendFogMessage();
	SendWeatherInfo();
	SendLevelLockInfo();
	SendWindInfo();
	SendGrassInfo();
	m_bInitClientMsgs = FALSE;
}

void CBasePlayer::Pain( int hitGroup, bool hasArmour )
{
	int sound = RANDOM_LONG( 0, 2 );
	const char* sample;
	if ( hitGroup == HITGROUP_HEAD )
	{
		if ( m_iKevlar == 2 )
			sample = "player/bhit_helmet-1.wav";
		else if ( sound == 0 )
			sample = "player/headshot1.wav";
		else if ( sound == 1 )
			sample = "player/headshot2.wav";
		else
			sample = "player/headshot3.wav";
	}
	else if ( hasArmour && hitGroup != HITGROUP_LEFTLEG && hitGroup != HITGROUP_RIGHTLEG )
	{
		sample = "player/bhit_kevlar-1.wav";
	}
	else if ( sound == 0 )
	{
		sample = "player/bhit_flesh-1.wav";
	}
	else if ( sound == 1 )
	{
		sample = "player/bhit_flesh-2.wav";
	}
	else
	{
		sample = "player/bhit_flesh-3.wav";
	}
	EMIT_SOUND( edict(), CHAN_VOICE, sample, 1, ATTN_NORM );
}

/*
===========
TabulateAmmo
This function is used to find and store 
all the ammo we have into the ammo vars.
============
*/
void CBasePlayer::TabulateAmmo()
{
	ammo_buckshot = AmmoInventory( GetAmmoIndex( "buckshot" ) );
	ammo_9mm = AmmoInventory( GetAmmoIndex( "9mm" ) );
	ammo_556nato = AmmoInventory( GetAmmoIndex( "556Nato" ) );
	ammo_556natobox = AmmoInventory( GetAmmoIndex( "556NatoBox" ) );
	ammo_762nato = AmmoInventory( GetAmmoIndex( "762Nato" ) );
	ammo_762natobox = AmmoInventory( GetAmmoIndex( "762NatoBox" ) );
	ammo_45acp = AmmoInventory( GetAmmoIndex( "45acp" ) );
	ammo_50ae = AmmoInventory( GetAmmoIndex( "50AE" ) );
	ammo_338mag = AmmoInventory( GetAmmoIndex( "338Magnum" ) );
	ammo_57mm = AmmoInventory( GetAmmoIndex( "57mm" ) );
	ammo_357sig = AmmoInventory( GetAmmoIndex( "357SIG" ) );
}

void CBasePlayer::SetPrefsFromUserinfo( char* infoBuffer )
{
	const char* value = g_engfuncs.pfnInfoKeyValue( infoBuffer, "_cl_autowepswitch" );
	m_iAutoWepSwitch = *value ? atoi( value ) : 1;
	value = g_engfuncs.pfnInfoKeyValue( infoBuffer, "_vgui_menus" );
	m_bVGUIMenus = !*value || atoi( value ) != 0;
	value = g_engfuncs.pfnInfoKeyValue( infoBuffer, "_ah" );
	m_bShowHints = !*value || atoi( value ) != 0;
}

void CBasePlayer::UpdateGeigerCounter()
{
	if ( !( gpGlobals->time >= m_flgeigerDelay ) )
		return;
	m_flgeigerDelay = gpGlobals->time + 0.25;
	BYTE range = (BYTE)( m_flgeigerRange / 4 );
	if ( range != m_igeigerRangePrev )
	{
		m_igeigerRangePrev = range;
		MESSAGE_BEGIN( MSG_ONE, gmsgGeigerRange, NULL, pev );
			WRITE_BYTE( range );
		MESSAGE_END();
	}

	// reset counter and semaphore
	if ( !RANDOM_LONG( 0, 3 ) )
		m_flgeigerRange = 1000;
}

void CBasePlayer::CheckSuitUpdate()
{
	if ( !( pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return;
	int search = m_iSuitPlayNext;

	// if in range of radiation source, ping geiger counter
	UpdateGeigerCounter();
	if ( g_pGameRules->IsMultiplayer() || !( gpGlobals->time >= m_flSuitUpdate && m_flSuitUpdate > 0 ) )
		return;
	int sentence = 0;
	for ( int i = 0; i < CSUITPLAYLIST; i++ )
	{
		sentence = m_rgSuitPlayList[search];
		if ( sentence )
			break;
		search = ( search + 1 ) % CSUITPLAYLIST;
	}
	if ( !sentence )
	{
		// queue is empty, don't check 
		m_flSuitUpdate = 0;
		return;
	}
	m_rgSuitPlayList[search] = 0;
	if ( sentence > 0 )
	{
		char name[CBSENTENCENAME_MAX + 2];
		strcpy( name, "!" );
		strcat( name, gszallsentencenames[sentence] );
		EMIT_SOUND_SUIT( edict(), name );
	}
	else
	{
		EMIT_GROUPID_SUIT( edict(), -sentence );
	}
	m_flSuitUpdate = gpGlobals->time + 3.5;
}

void CBasePlayer::SetSuitUpdate( const char* name, int group, int noRepeatTime )
{
	if ( !( pev->weapons & ( 1 << WEAPON_SUIT ) ) || g_pGameRules->IsMultiplayer() )
		return;
	if ( !name )
	{
		memset( m_rgSuitPlayList, 0, sizeof( m_rgSuitPlayList ) );
		return;
	}
	int sentence;
	if ( group )
	{
		sentence = -SENTENCEG_GetIndex( name );
	}
	else
	{
		sentence = SENTENCEG_Lookup( name, NULL );
		if ( sentence < 0 )
			return;
	}
	int empty = -1;
	int slot;
	for ( slot = 0; slot < CSUITNOREPEAT; slot++ )
	{
		if ( m_rgiSuitNoRepeat[slot] == sentence )
			break;
		if ( !m_rgiSuitNoRepeat[slot] )
			empty = slot;
	}
	if ( slot < CSUITNOREPEAT )
	{
		if ( gpGlobals->time <= m_rgflSuitNoRepeatTime[slot] )
			return;
		m_rgiSuitNoRepeat[slot] = 0;
		m_rgflSuitNoRepeatTime[slot] = 0;
	}
	else if ( noRepeatTime )
	{
		slot = empty < 0 ? RANDOM_LONG( 0, CSUITNOREPEAT - 1 ) : empty;
	}
	if ( noRepeatTime )
	{
		m_rgiSuitNoRepeat[slot] = sentence;
		m_rgflSuitNoRepeatTime[slot] = gpGlobals->time + noRepeatTime;
	}
	m_rgSuitPlayList[m_iSuitPlayNext] = sentence;
	m_iSuitPlayNext = ( m_iSuitPlayNext + 1 ) % CSUITPLAYLIST;
	if ( gpGlobals->time >= m_flSuitUpdate )
		m_flSuitUpdate = gpGlobals->time + ( m_flSuitUpdate == 0 ? 0.1 : 3.5 );
}

void CBasePlayer::CheckTimeBasedDamage()
{
	if ( !( m_bitsDamageType & DMG_TIMEBASED ) || fabsf( gpGlobals->time - m_tbdPrev ) < 2 )
		return;
	m_tbdPrev = gpGlobals->time;
	for ( int i = 0; i < CDMG_TIMEBASED; i++ )
	{
		int damageType = DMG_PARALYZE << i;
		if ( !( m_bitsDamageType & damageType ) )
			continue;
		BYTE duration = 2;
		if ( i == itbd_Poison )
		{
			TakeDamage( pev, pev, 2, DMG_GENERIC );
			duration = 5;
		}
		else if ( i == itbd_DrownRecover )
		{
			if ( m_idrowndmg > m_idrownrestored )
			{
				int amount = min( m_idrowndmg - m_idrownrestored, 10 );
				TakeHealth( amount, DMG_GENERIC );
				m_idrownrestored += amount;
			}
			duration = 4;
		}
		if ( !m_rgbTimeBasedDamage[i] )
		{
			m_rgbTimeBasedDamage[i] = duration;
			continue;
		}
		if ( ( ( i == itbd_NerveGas && m_rgbTimeBasedDamage[i] <= 1 )
			|| ( i == itbd_Poison && m_rgbTimeBasedDamage[i] <= 4 ) ) && m_rgItems[ITEM_ANTIDOTE] )
		{
			m_rgbTimeBasedDamage[i] = 0;
			m_rgItems[ITEM_ANTIDOTE]--;
			SetSuitUpdate( "!HEV_HEAL4", FALSE, 0 );
		}
		if ( m_rgbTimeBasedDamage[i] )
			m_rgbTimeBasedDamage[i]--;
		if ( !m_rgbTimeBasedDamage[i] )
			m_bitsDamageType &= ~damageType;
	}
}

void CBasePlayer::UpdatePlayerSound()
{
	CSound* sound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	if ( !sound )
	{
		ALERT( at_console, "Client lost reserved sound!\n" );
		return;
	}
	sound->m_iType = 0;
	int volume = 0;
	if ( pev->flags & FL_ONGROUND )
	{
		volume = (int)pev->velocity.Length();
		if ( volume > 512 )
			volume = 512;
		if ( ( volume >= 18 && volume <= 150 ) || ( ( pev->flags & FL_DUCKING ) && volume > 17 ) )
			volume = 18;
	}
	if ( pev->button & IN_JUMP )
		volume += 100;
	if ( volume < m_iWeaponVolume )
	{
		m_iTargetVolume = m_iWeaponVolume;

		// OR in the bits for COMBAT sound if the weapon is being louder than the player. 
		sound->m_iType |= bits_SOUND_COMBAT;
	}
	else
	{
		m_iTargetVolume = volume;
	}
	m_iWeaponVolume = (int)( m_iWeaponVolume - 250 * gpGlobals->frametime );
	volume = sound->m_iVolume;
	if ( volume < m_iTargetVolume )
	{
		volume = m_iTargetVolume;
	}
	else if ( volume > m_iTargetVolume )
	{
		volume = (int)( volume - 250 * gpGlobals->frametime );
		if ( volume < m_iTargetVolume )
			volume = 0;
	}
	if ( m_fNoPlayerSound )
		volume = 0;
	if ( gpGlobals->time > m_flStopExtraSoundTime )
		m_iExtraSoundTypes = 0;
	sound->m_vecOrigin = pev->origin;
	sound->m_iVolume = volume;
	sound->m_iType |= m_iExtraSoundTypes | bits_SOUND_PLAYER;
	m_iWeaponFlash = max( 0, (int)( m_iWeaponFlash - 256 * gpGlobals->frametime ) );
}


//==============================================
// !!!UNDONE:ultra temporary SprayCan entity to apply
// decal frame at a time. For PreAlpha CD
//==============================================
class CSprayCan : public CBaseEntity
{
public:
	void Spawn( entvars_t* owner );
	void Think() override;
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
};

void CSprayCan::Spawn( entvars_t* owner )
{
	pev->origin = owner->origin + Vector( 0, 0, 32 );
	pev->angles = owner->v_angle;
	pev->owner = ENT( owner );
	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;
	EMIT_SOUND( edict(), CHAN_VOICE, "player/sprayer.wav", 1, ATTN_NORM );
}

void CSprayCan::Think()
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pev->owner );
	int frames = player ? player->GetCustomDecalFrames() : -1;
	int index = ENTINDEX( pev->owner );
	UTIL_MakeVectors( pev->angles );
	TraceResult trace;
	UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128,
		ignore_monsters, pev->owner, &trace );
	if ( frames == -1 )
	{
		UTIL_DecalTrace( &trace, DECAL_LAMBDA6 );
		UTIL_Remove( this );
	}
	else
	{
		UTIL_PlayerDecalTrace( &trace, index, (int)pev->frame, TRUE );
		float frame = pev->frame;
		pev->frame = frame + 1;
		if ( frame >= frames - 1 )
			UTIL_Remove( this );
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

class CBloodSplat : public CBaseEntity
{
public:
	void Spawn( entvars_t* owner );
	void Spray();
};

void CBloodSplat::Spawn( entvars_t* owner )
{
	pev->origin = owner->origin + Vector( 0, 0, 32 );
	pev->angles = owner->v_angle;
	pev->owner = ENT( owner );
	SetThink( &CBloodSplat::Spray );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CBloodSplat::Spray()
{
	if ( g_Language != LANGUAGE_GERMAN )
	{
		UTIL_MakeVectors( pev->angles );
		TraceResult trace;
		UTIL_TraceLine( pev->origin, pev->origin + gpGlobals->v_forward * 128,
			ignore_monsters, pev->owner, &trace );
		UTIL_BloodDecalTrace( &trace, BLOOD_COLOR_RED );
	}
	SetThink( &CBloodSplat::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

//=========================================================
// Dead HEV suit prop
//=========================================================
class CDeadHEV : public CBaseMonster
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData* data ) override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }

	int m_iPose;
	static const char* m_szPoses[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

const char* CDeadHEV::m_szPoses[] = { "deadback", "deadsitting", "deadstomach", "deadtable" };

LINK_ENTITY_TO_CLASS( monster_hevsuit_dead, CDeadHEV );

void CDeadHEV::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "pose" ) )
	{
		m_iPose = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue( data );
	}
}

void CDeadHEV::Spawn()
{
	PRECACHE_MODEL( "models/player.mdl" );
	SET_MODEL( edict(), "models/player.mdl" );
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	pev->body = 1;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
	{
		ALERT( at_console, "Dead hevsuit with bad pose\n" );
		pev->sequence = 0;
		pev->effects = EF_BRIGHTFIELD;
	}

	// Corpses have less health
	pev->health = 8;
	MonsterInitDead();
}

class CRevertSaved : public CPointEntity
{
public:
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT MessageThink();
	void EXPORT LoadThink();

	float m_messageTime;
	float m_loadTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( player_loadsaved, CRevertSaved );

TYPEDESCRIPTION CRevertSaved::m_SaveData[] =
{
	DEFINE_FIELD( CRevertSaved, m_messageTime, FIELD_FLOAT ),	// These are not actual times, but durations, so save as floats
	DEFINE_FIELD( CRevertSaved, m_loadTime, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CRevertSaved, CPointEntity );

void CRevertSaved::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "duration" ) )
		pev->dmg_take = atof( data->szValue );
	else if ( FStrEq( data->szKeyName, "holdtime" ) )
		pev->dmg_save = atof( data->szValue );
	else if ( FStrEq( data->szKeyName, "messagetime" ) )
		m_messageTime = atof( data->szValue );
	else if ( FStrEq( data->szKeyName, "loadtime" ) )
		m_loadTime = atof( data->szValue );
	else
	{
		data->fHandled = FALSE;
		return;
	}
	data->fHandled = TRUE;
}

void CRevertSaved::Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value )
{
	UTIL_ScreenFadeAll( pev->rendercolor, pev->dmg_take, pev->dmg_save, (int)pev->renderamt, FFADE_OUT );
	pev->nextthink = gpGlobals->time + m_messageTime;
	SetThink( &CRevertSaved::MessageThink );
}

void CRevertSaved::MessageThink()
{
	UTIL_ShowMessageAll( STRING( pev->message ) );
	float delay = m_loadTime - m_messageTime;
	if ( delay <= 0 )
	{
		LoadThink();
	}
	else
	{
		pev->nextthink = gpGlobals->time + delay;
		SetThink( &CRevertSaved::LoadThink );
	}
}

void CRevertSaved::LoadThink()
{
	if ( !gpGlobals->deathmatch )
		SERVER_COMMAND( "reload\n" );
}


//=========================================================
// Multiplayer intermission spots.
//=========================================================
class CInfoIntermission : public CPointEntity
{
public:
	void Spawn() override;
	void Think() override;
};

LINK_ENTITY_TO_CLASS( info_intermission, CInfoIntermission );

void CInfoIntermission::Spawn()
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;
	pev->nextthink = gpGlobals->time + 2; // let targets spawn!
}

void CInfoIntermission::Think()
{
	edict_t* target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
	if ( FNullEnt( target ) )
		return;
	pev->v_angle = UTIL_VecToAngles( ( target->v.origin - pev->origin ).Normalize() );
	pev->v_angle.x = -pev->v_angle.x;
}

class CInfoCamera : public CPointEntity
{
public:
	void Spawn() override;
	void Think() override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	Vector m_origin;
	Vector m_view_ofs;
	Vector m_angles;
	Vector m_v_angle;
	unsigned int m_afPhysicsFlags;
	int m_fixangle;
	int m_solid;
	float m_takedamage;
	int m_movetype;
	int m_modelindex;
	BOOL _using;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( info_camera, CInfoCamera );

TYPEDESCRIPTION CInfoCamera::m_SaveData[] =
{
	DEFINE_FIELD( CInfoCamera, m_origin, FIELD_VECTOR ),
	DEFINE_FIELD( CInfoCamera, m_view_ofs, FIELD_VECTOR ),
	DEFINE_FIELD( CInfoCamera, m_angles, FIELD_VECTOR ),
	DEFINE_FIELD( CInfoCamera, m_afPhysicsFlags, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoCamera, m_fixangle, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoCamera, m_solid, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoCamera, m_takedamage, FIELD_FLOAT ),
	DEFINE_FIELD( CInfoCamera, m_movetype, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoCamera, m_modelindex, FIELD_INTEGER ),
	DEFINE_FIELD( CInfoCamera, _using, FIELD_BOOLEAN )
};

IMPLEMENT_SAVERESTORE( CInfoCamera, CPointEntity );

void CInfoCamera::Spawn()
{
	UTIL_SetOrigin( pev, pev->origin );
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	pev->v_angle = g_vecZero;
	pev->nextthink = gpGlobals->time + 2;
	_using = FALSE;
}

void CInfoCamera::Think()
{
	edict_t* target = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
	if ( FNullEnt( target ) )
		return;
	pev->v_angle = UTIL_VecToAngles( ( target->v.origin - pev->origin ).Normalize() );
	pev->v_angle.x = -pev->v_angle.x;
}

void CInfoCamera::Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value )
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) );
	if ( _using )
	{
		_using = FALSE;
		player->StopFiberCamera();
		player->m_afPhysicsFlags = m_afPhysicsFlags;
		// czeror: the eyes back to the height of the player (VEC_VIEW / VEC_DUCK_VIEW of
		// pm_shared). Left at zero, the player looked from his waist until he ducked once
		player->pev->view_ofs = Vector( 0, 0, ( player->pev->flags & FL_DUCKING ) ? 12 : 28 );
		player->pev->fixangle = 1;
		player->pev->solid = m_solid;
		player->pev->takedamage = m_takedamage;
		player->pev->movetype = m_movetype;
		player->pev->modelindex = m_modelindex;
		player->pev->angles = m_angles;
		player->pev->v_angle = m_v_angle;
		UTIL_SetOrigin( player->pev, m_origin );
	}
	else
	{
		m_angles = player->pev->angles;
		player->pev->view_ofs = g_vecZero;
		m_v_angle = player->pev->v_angle;
		m_afPhysicsFlags = player->m_afPhysicsFlags;
		m_origin = player->pev->origin;
		m_modelindex = player->pev->modelindex;
		m_movetype = player->pev->movetype;
		m_takedamage = player->pev->takedamage;
		m_solid = player->pev->solid;
		player->StartFiberCamera( pev->origin, pev->v_angle, entindex() );
		_using = TRUE;
	}
}

class CStripWeapons : public CPointEntity
{
public:
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	string_t m_item;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( player_weaponstrip, CStripWeapons );

TYPEDESCRIPTION CStripWeapons::m_SaveData[] =
{
	DEFINE_FIELD( CStripWeapons, m_item, FIELD_STRING )
};

IMPLEMENT_SAVERESTORE( CStripWeapons, CPointEntity );

void CStripWeapons::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "item" ) )
	{
		m_item = ALLOC_STRING( data->szValue );
		data->fHandled = TRUE;
	}
	else
	{
		data->fHandled = FALSE;
	}
}

void CStripWeapons::Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value )
{
	CBasePlayer* player;
	if ( activator && activator->IsPlayer() )
	{
		player = (CBasePlayer*)activator;
	}
	else
	{
		if ( g_pGameRules->IsDeathmatch() )
			return;
		player = (CBasePlayer*)CBaseEntity::Instance( INDEXENT( 1 ) );
		if ( !player )
			return;
	}
	if ( !m_item )
	{
		player->RemoveAllItems( FALSE );
		player->m_iFOV = 90;
		return;
	}
	for ( int i = 0; i < ARRAYSIZE( player->m_rgpPlayerItems ); i++ )
	{
		for ( CBasePlayerItem* item = player->m_rgpPlayerItems[i]; item; item = item->m_pNext )
		{
			if ( strcmp( STRING( m_item ), STRING( item->pev->classname ) ) )
				continue;
			if ( player->m_pActiveItem == item )
			{
				player->m_iFOV = 90;
				g_pGameRules->GetNextBestWeapon( player, item );
			}
			if ( item->m_bIsAccessory )
				player->m_iAccessoryBits &= ~( 1 << item->m_iId );
			else
				player->pev->weapons &= ~( 1 << item->m_iId );
			item->Drop();
			player->m_rgpPlayerItems[i] = NULL;
			if ( player->m_pLastItem == item )
				player->m_pLastItem = NULL;
			return;
		}
	}
}

void CBasePlayer::Spawn()
{
	m_flStartCharge = gpGlobals->time;
	pev->classname = MAKE_STRING( "player" );
	pev->health = 100;
	if ( !m_bNotKilled )
	{
		pev->armorvalue = 0;
		m_iKevlar = 0;
	}
	pev->maxspeed = 1000;
	pev->takedamage = DAMAGE_AIM;
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_WALK;
	pev->max_health = pev->health;
	pev->flags = ( pev->flags & FL_PROXY ) | FL_CLIENT;
	pev->air_finished = gpGlobals->time + 12;
	pev->dmg = 2;				// initial water damage
	pev->effects = 0;
	pev->deadflag = DEAD_NO;
	pev->dmg_take = 0;
	pev->dmg_save = 0;
	m_bitsHUDDamage = -1;
	m_bitsDamageType = 0;
	m_afPhysicsFlags = 0;
	m_fLongJump = FALSE; // no longjump module. 
	m_iClientFOV = -1;
	m_bFirstConnected = FALSE;
	pev->gamestate = !( pev->weapons & ( 1 << WEAPON_SHIELDGUN ) );
	pev->friction = 1;
	pev->gravity = 1;
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "slj", "0" );
	g_engfuncs.pfnSetPhysicsKeyValue( edict(), "hl", "1" );
	m_hintMessageQueue.Reset();
	m_bHasC4 = FALSE;
	m_bKilledByBomb = FALSE;
	m_bKilledByGrenade = FALSE;
	m_iChaseTarget = 1;
	m_bEscaped = FALSE;
	m_iLastZoom = 90;
	m_iRadioMessages = 60;
	m_flVelocityModifier = 1;
	m_flLastTalk = 0;
	m_flIdleCheckTime = 0;
	m_flRadioTime = 0;
	m_fCamSwitch = 0;
	m_iCurrentKickVote = 0;
	m_bJustKilledTeammate = FALSE;
	m_tmNextRadarUpdate = gpGlobals->time;
	m_vLastOrigin = g_vecZero;
	m_flNextVoteTime = 0;
	SET_VIEW( edict(), edict() );
	pev->iuser3 = 0;
	pev->iuser2 = 0;
	pev->iuser1 = 0;
	m_bHeadshotKilled = FALSE;
	m_bShieldDrawn = FALSE;
	for ( int i = 0; i < ARRAYSIZE( m_vRecentPath ); i++ )
		m_vRecentPath[i] = g_vecZero;
	m_flLastFired = -15;
	m_SbarString0[0] = 0;
	m_SbarString1[0] = 0;
	if ( m_pActiveItem && !pev->viewmodel )
	{
		switch ( m_pActiveItem->m_iId )
		{
		case WEAPON_SCOUT:
			pev->viewmodel = MAKE_STRING( "models/v_scout.mdl" );
			break;
		case WEAPON_SG550:
			pev->viewmodel = MAKE_STRING( "models/v_sg550.mdl" );
			break;
		case WEAPON_AWP:
			pev->viewmodel = MAKE_STRING( "models/v_awp.mdl" );
			break;
		case WEAPON_G3SG1:
			pev->viewmodel = MAKE_STRING( "models/v_g3sg1.mdl" );
			break;
		}
	}
	m_iFOV = 90;
	m_flNextDecalTime = 0; // let this player decal as soon as he spawns.
												// are recieved by all clients

	m_flTimeStepSound = 0;
	m_iStepLeft = 0;
	m_flFieldOfView = 0.5; // some monsters use this to determine whether or not the player is looking at them.
	m_bloodColor = BLOOD_COLOR_RED;
	m_flgeigerDelay = gpGlobals->time + 2.0;	// wait a few seconds until user-defined message registrations
	m_flNextAttack = UTIL_WeaponTimeBase();
	StartSneaking();
	m_iFlashBattery = 99;
	m_flFlashLightTime = 1; // force first message
	pev->body = m_bHasDefuser;
	m_flFallVelocity = 0;
	g_pGameRules->GetPlayerSpawnSpot( this );
	SET_MODEL( edict(), "models/player.mdl" );
	g_ulModelIndexPlayer = pev->modelindex;
	pev->sequence = LookupActivity( ACT_IDLE );
	if ( pev->flags & FL_DUCKING )
		UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	pev->view_ofs = VEC_VIEW;
	Precache();
	m_HackedGunPos = Vector( 0, 32, 0 );
	if ( m_iPlayerSound == SOUNDLIST_EMPTY )
		ALERT( at_console, "Couldn't alloc player sound slot!\n" );
	m_pClientActiveItem = NULL;
	m_iClientBattery = -1;
	m_fInitHUD = TRUE;
	m_iHideHUD &= ~( HIDEHUD_WEAPONS | HIDEHUD_HEALTH | HIDEHUD_TIMER | HIDEHUD_MONEY );
	m_fNoPlayerSound = FALSE;
	m_pLastItem = NULL;
	m_fWeapon = FALSE;
	if ( m_bNotKilled )
	{
		memset( m_rgAmmoLast, -1, sizeof( m_rgAmmoLast ) );
	}
	else
	{
		m_iClientHideHUD = -1;
		memset( m_rgAmmo, 0, sizeof( m_rgAmmo ) );
		m_bHasPrimary = FALSE;
	}
	m_lasty = 0;
	m_lastx = 0;
	g_pGameRules->PlayerSpawn( this );
	m_bNotKilled = TRUE;
	m_bIsDefusing = FALSE;
	ResetMaxSpeed();
	UTIL_SetOrigin( pev, pev->origin );
	if ( m_bIsVIP )
	{
		m_iKevlar = 2;
		pev->armorvalue = 200;
		m_hintMessageQueue.AddMessage( "#Hint_you_are_the_vip" );
	}
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( entindex() );
		switch ( m_iTeam )
		{
		case CT:
			WRITE_STRING( "CT" );
			break;
		case TERRORIST:
			WRITE_STRING( "TERRORIST" );
			break;
		case SPECTATOR:
			WRITE_STRING( "SPECTATOR" );
			break;
		default:
			WRITE_STRING( "UNASSIGNED" );
			break;
		}
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( entindex() );
		WRITE_SHORT( (int)pev->frags );
		WRITE_SHORT( m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( m_iTeam );
	MESSAGE_END();
	UTIL_ScreenFade( this, g_vecZero, 0.001, 0, 0, FFADE_IN );
	pev->weapons |= 1 << WEAPON_SUIT;
	m_iTeam = CT;
	m_LevelLocks = gLevelLocks;
	m_iHideHUD |= HIDEHUD_FLASHLIGHT;
	savedHudBits = m_iHideHUD;
}

void CBasePlayer::TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType )
{
	if ( !pev->takedamage )
		return;
	m_LastHitGroup = trace->iHitgroup;
	Vector source = CBaseEntity::Instance( attacker )->pev->origin;
	if ( m_pActiveItem && FClassnameIs( m_pActiveItem->pev, "weapon_shieldgun" ) && FInViewCone( &source )
		&& ( ((CBasePlayerWeapon*)m_pActiveItem)->m_iWeaponState & WPNSTATE_SHIELD_DRAWN ) )
	{
		trace->iHitgroup = HITGROUP_SHIELD;
	}
	bool blood = true;
	switch ( trace->iHitgroup )
	{
	case HITGROUP_HEAD:
		if ( m_iKevlar == 2 )
		{
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, trace->vecEndPos );
				WRITE_BYTE( TE_STREAK_SPLASH );
				WRITE_COORD( trace->vecEndPos.x );
				WRITE_COORD( trace->vecEndPos.y );
				WRITE_COORD( trace->vecEndPos.z );
				WRITE_COORD( trace->vecPlaneNormal.x );
				WRITE_COORD( trace->vecPlaneNormal.y );
				WRITE_COORD( trace->vecPlaneNormal.z );
				WRITE_BYTE( 5 );
				WRITE_SHORT( 22 );
				WRITE_SHORT( 25 );
				WRITE_SHORT( 65 );
			MESSAGE_END();
			blood = false;
		}
		else
		{
			pev->punchangle.x = max( -12, damage * -0.5 );
			pev->punchangle.z = RANDOM_FLOAT( -1, 1 ) * damage;
			pev->punchangle.z = max( -9, min( pev->punchangle.z, 9 ) );
		}
		break;
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ( m_iKevlar )
			blood = false;
		else
			pev->punchangle.x = max( -4, damage * -0.1 );
		break;
	case HITGROUP_LEFTARM:
	case HITGROUP_RIGHTARM:
		blood = !m_iKevlar;
		break;
	case HITGROUP_SHIELD:
		EMIT_SOUND( edict(), CHAN_WEAPON, RANDOM_LONG( 0, 1 ) ? "weapons/ric_metal-1.wav" : "weapons/ric_metal-2.wav",
			1, ATTN_NORM );
		UTIL_Sparks( trace->vecEndPos );
		pev->punchangle.x = RANDOM_FLOAT( -0.15, 0.15 ) * damage;
		pev->punchangle.z = RANDOM_FLOAT( -0.15, 0.15 ) * damage;
		if ( pev->punchangle.x < 4 )
			pev->punchangle.x = -4;
		pev->punchangle.z = max( -5, min( pev->punchangle.z, 5 ) );
		blood = false;
		break;
	}
	if ( blood )
	{
		BloodSplat( trace->vecEndPos, direction, trace->iHitgroup, (int)( damage * 5 ) );
		SpawnBlood( trace->vecEndPos, BloodColor(), damage );
		TraceBleed( damage, direction, trace, damageType );
	}
	AddMultiDamage( attacker, this, damage, damageType );
}

void CBasePlayer::SendWindInfo()
{
	CWorld* world = (CWorld*)CBaseEntity::Instance( ENT( 0 ) );
	MESSAGE_BEGIN( MSG_ONE, gmsgWindInfo, NULL, pev );
		WRITE_COORD( world->m_fWindDirectionMinX );
		WRITE_COORD( world->m_fWindDirectionMaxX );
		WRITE_COORD( world->m_fWindDirectionMinY );
		WRITE_COORD( world->m_fWindDirectionMaxY );
		WRITE_COORD( world->m_fWindChangeTimeMin );
		WRITE_COORD( world->m_fWindChangeTimeMax );
	MESSAGE_END();
}

void CBasePlayer::SendGrassInfo()
{
	CWorld* world = (CWorld*)CBaseEntity::Instance( ENT( 0 ) );
	MESSAGE_BEGIN( MSG_ONE, gmsgGrassInfo, NULL, pev );
		WRITE_STRING( world->m_iGrassSpriteName ? STRING( world->m_iGrassSpriteName ) : "" );
		WRITE_COORD( world->m_fGrassFadeDist );
		WRITE_COORD( world->m_fGrassMinHeight );
		WRITE_COORD( world->m_fGrassMaxHeight );
		WRITE_LONG( (int)world->m_iGrassDensity );
	MESSAGE_END();
}

void CBasePlayer::SendFogMessage()
{
	MESSAGE_BEGIN( MSG_ONE, gmsgInitHUD, NULL, pev );
	CClientFog* fog = (CClientFog*)UTIL_FindEntityByClassname( NULL, "env_fog" );
	if ( fog )
	{
		ALERT( at_console, "Map has fog!\n" );
		WRITE_SHORT( (int)fog->pev->rendercolor.x );
		WRITE_SHORT( (int)fog->pev->rendercolor.y );
		WRITE_SHORT( (int)fog->pev->rendercolor.z );
		WRITE_COORD( fog->m_flDensity );
		WRITE_COORD( fog->m_fStartDistance );
		WRITE_COORD( fog->m_fStopDistance );
		WRITE_SHORT( fog->pev->spawnflags );
	}
	else
	{
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_SHORT( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_SHORT( 0 );
	}
	MESSAGE_END();
}

void CBasePlayer::SendWeatherInfo()
{
	CBaseEntity* rain = UTIL_FindEntityByClassname( NULL, "env_rain" );
	CBaseEntity* rainBrush = UTIL_FindEntityByClassname( NULL, "func_rain" );
	if ( rain || rainBrush )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgReceiveW, NULL, pev );
			WRITE_BYTE( 1 );
		MESSAGE_END();
		return;
	}
	CBaseEntity* snow = UTIL_FindEntityByClassname( NULL, "env_snow" );
	CBaseEntity* snowBrush = UTIL_FindEntityByClassname( NULL, "func_snow" );
	if ( snow || snowBrush )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgReceiveW, NULL, pev );
			WRITE_BYTE( 2 );
		MESSAGE_END();
	}
}

void CBasePlayer::SendLevelLockInfo()
{
	if ( CVAR_GET_FLOAT( "cl_newgame" ) != 0 )
	{
		m_LevelLocks = LEVEL_BonusLevels | LEVEL_Training | LEVEL_Recoil;
		gLevelLocks = m_LevelLocks;
		CVAR_SET_FLOAT( "cl_newgame", 0 );
		CVAR_SET_FLOAT( "cl_levellocks", (float)gLevelLocks );
	}
	MESSAGE_BEGIN( MSG_ONE, gmsgLevelLocks, NULL, pev );
		WRITE_LONG( m_LevelLocks );
	MESSAGE_END();
}

void CBasePlayer::DisplayObjectives()
{
	int line = 0;
	CBaseEntity* entity = NULL;
	while ( ( entity = UTIL_FindEntityByClassname( entity, "trigger_objective" ) ) != NULL )
	{
		if ( ((CTriggerObjective*)entity)->DisplayObjective( line ) )
			line++;
	}
}

BOOL CBasePlayer::HandleBombDefuse()
{
	if ( !m_toolTrigger || !m_toolTrigger->IsBombDefuseTrigger() )
		return FALSE;
	if ( m_afButtonReleased & IN_USE )
	{
		SetProgressBarTime( 0 );
		m_defuseTime = 0;
		EMIT_SOUND_DYN( edict(), CHAN_STATIC, "ambience/defuse_loop.wav", 0, 0, SND_STOP, PITCH_NORM );
		return TRUE;
	}
	if ( m_toolTrigger->IsPlayerInPosition() && ( m_afButtonPressed & IN_USE ) )
	{
		SetProgressBarTime( m_toolTrigger->m_iDefuseTime );
		m_defuseTime = gpGlobals->time + m_toolTrigger->m_iDefuseTime;
		EMIT_SOUND( edict(), CHAN_STATIC, "ambience/defuse_loop.wav", 1, ATTN_NORM );
		return TRUE;
	}
	return FALSE;
}

void CBasePlayer::ItemPostFrame()
{
	if ( m_pTank )
		return;
	if ( m_cameraTrigger && ( pev->button & ( IN_ATTACK | IN_USE ) ) && m_cameraTrigger->MinimumTimeMet() )
	{
		m_cameraTrigger->Use( this, this, USE_TOGGLE, 1 );
		m_cameraTrigger = NULL;
	}
	else if ( m_flNextAttack <= gpGlobals->time )
	{
		ImpulseCommands();
		if ( m_pActiveItem )
			m_pActiveItem->ItemPostFrame();
	}
}

void CBasePlayer::RemoveAllItems( BOOL removeSuit )
{
	if ( m_pActiveItem )
	{
		ResetAutoaim();
		m_pActiveItem->Holster();
		m_pActiveItem = NULL;
	}
	FlashlightTurnOff();
	if ( m_bHasNightVision )
	{
		NightVisionTurnOff();
		m_bHasNightVision = FALSE;
	}
	m_pLastItem = NULL;
	for ( int i = 0; i < ARRAYSIZE( m_rgpPlayerItems ); i++ )
	{
		m_pActiveItem = m_rgpPlayerItems[i];
		while ( m_pActiveItem )
		{
			CBasePlayerItem* next = m_pActiveItem->m_pNext;
			m_pActiveItem->Drop();
			m_pActiveItem = next;
		}
		m_rgpPlayerItems[i] = NULL;
	}
	m_pActiveItem = NULL;
	pev->viewmodel = 0;
	pev->weaponmodel = 0;
	if ( removeSuit )
	{
		pev->weapons = 0;
		m_iAccessoryBits = 0;
	}
	else
	{
		pev->weapons &= 1 << WEAPON_SUIT;
		m_iAccessoryBits &= 1 << WEAPON_SUIT;
	}
	memset( m_rgAmmo, 0, sizeof( m_rgAmmo ) );
	UpdateClientData();
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 0 );
	MESSAGE_END();
}

void CBasePlayer::PackDeadPlayerItems()
{
	CBasePlayerItem* weapons[20] = {};
	int ammo[33];
	memset( ammo, -1, sizeof( ammo ) );
	int weaponRules = g_pGameRules->DeadPlayerWeapons( this );
	int ammoRules = g_pGameRules->DeadPlayerAmmo( this );
	if ( weaponRules != GR_PLR_DROP_GUN_NO || ammoRules != GR_PLR_DROP_AMMO_NO )
	{
		int weaponCount = 0;
		for ( int i = 0; i < ARRAYSIZE( m_rgpPlayerItems ); i++ )
		{
			for ( CBasePlayerItem* item = m_rgpPlayerItems[i]; item; item = item->m_pNext )
			{
				if ( weaponRules == GR_PLR_DROP_GUN_ALL
					|| ( weaponRules == GR_PLR_DROP_GUN_ACTIVE && item == m_pActiveItem && m_pActiveItem ) )
				{
					weapons[weaponCount++] = item;
				}
			}
		}

// now go through ammo and make a list of which types to pack.
		if ( ammoRules != GR_PLR_DROP_AMMO_NO )
		{
			int ammoCount = 0;
			for ( int i = 0; i < ARRAYSIZE( m_rgAmmo ); i++ )
			{
				if ( m_rgAmmo[i] <= 0 )
					continue;
				if ( ammoRules == GR_PLR_DROP_AMMO_ALL || ( ammoRules == GR_PLR_DROP_AMMO_ACTIVE && m_pActiveItem
					&& ( m_pActiveItem->PrimaryAmmoIndex() == i || m_pActiveItem->SecondaryAmmoIndex() == i ) ) )
				{
					// this is the secondary ammo type for the active weapon
					ammo[ammoCount++] = i;
				}
			}
		}
		CWeaponBox* box = (CWeaponBox*)CBaseEntity::Create( "weaponbox", pev->origin, pev->angles, edict() );
		box->pev->angles.x = 0;
		box->pev->angles.z = 0;
		box->SetThink( &CWeaponBox::Kill );
		box->pev->nextthink = gpGlobals->time + 120;
		for ( int i = 0; ammo[i] != -1; i++ )
		{
			int index = ammo[i];
			box->PackAmmo( MAKE_STRING( CBasePlayerItem::AmmoInfoArray[index].pszName ), m_rgAmmo[index] );
		}
		for ( int i = 0; weapons[i]; i++ )
			box->PackWeapon( weapons[i] );
		box->pev->velocity = pev->velocity * 1.2f;
	}
	RemoveAllItems( TRUE ); // now strip off everything that wasn't handled by the code above.
}

void CBasePlayer::DropPlayerItem( const char* name )
{
	if ( !*name )
		name = NULL;
	for ( int i = 0; i < ARRAYSIZE( m_rgpPlayerItems ); i++ )
	{
		for ( CBasePlayerItem* item = m_rgpPlayerItems[i]; item; item = item->m_pNext )
		{
			if ( name ? strcmp( name, STRING( item->pev->classname ) ) != 0 : item != m_pActiveItem )
				continue;
			g_pGameRules->GetNextBestWeapon( this, item );
			UTIL_MakeVectors( pev->angles );
			if ( item->m_bIsAccessory )
				m_iAccessoryBits &= ~( 1 << item->m_iId );
			else
				pev->weapons &= ~( 1 << item->m_iId );
			edict_t* entity = CREATE_NAMED_ENTITY( item->pev->classname );
			entity->v.origin = pev->origin + gpGlobals->v_forward * 10;
			entity->v.velocity = gpGlobals->v_forward * 400;
			entity->v.angles = pev->angles;
			entity->v.spawnflags |= SF_NORESPAWN;
			DispatchSpawn( entity );
			((CBasePlayerWeapon*)CBaseEntity::Instance( entity ))->m_iClip = ((CBasePlayerWeapon*)item)->m_iClip;
			DispatchTouch( entity, edict() );
			item->m_pPlayer->RemovePlayerItem( item );
			return; // we're done, so stop searching with the FOR loop.
		}
	}
}

void CBasePlayer::StartObserver( Vector position, Vector viewAngle )
{
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	pev->view_ofs = g_vecZero;
	pev->v_angle = viewAngle;
	pev->angles = pev->v_angle;
	pev->fixangle = 1;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	pev->movetype = MOVETYPE_NONE;
	pev->modelindex = 0;
	UTIL_SetOrigin( pev, position );
}

void CBasePlayer::StartDeathCam()
{
	if ( pev->view_ofs == g_vecZero )
		return;
	edict_t* camera = FIND_ENTITY_BY_CLASSNAME( NULL, "info_intermission" );
	if ( camera && !FNullEnt( camera ) )
	{
		int count = RANDOM_LONG( 0, 3 );
		while ( count-- > 0 )
		{
			edict_t* next = FIND_ENTITY_BY_CLASSNAME( camera, "info_intermission" );
			if ( next )
				camera = next;
		}
		if ( !camera )
			return;
		CopyToBodyQue( pev );
		StartObserver( camera->v.origin, camera->v.v_angle );
	}
	else
	{
		CopyToBodyQue( pev );
		TraceResult trace;
		UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 128 ), ignore_monsters, edict(), &trace );
		Vector angles = UTIL_VecToAngles( trace.vecEndPos - pev->origin );
		StartObserver( trace.vecEndPos, angles );
	}
}

void CBasePlayer::PlayerDeathThink()
{
	if ( pev->flags & FL_ONGROUND )
	{
		float speed = pev->velocity.Length() - 20;
		pev->velocity = speed <= 0 ? g_vecZero : pev->velocity.Normalize() * speed;
	}
	if ( HasWeapons() )
		PackDeadPlayerItems();
	if ( pev->modelindex && !m_fSequenceFinished && pev->deadflag == DEAD_DYING )
	{
		StudioFrameAdvance();
		m_iRespawnFrames++;
		if ( m_iRespawnFrames < 120 )
			return;
	}

	// once we're done animating our death and we're on the ground, we want to set movetype to None so our dead body won't do collisions and stuff anymore
	// this prevents a bug where the dead body would go to a player's head if he walked over it while the dead player was clicking their button to respawn
	if ( pev->movetype != MOVETYPE_NONE && ( pev->flags & FL_ONGROUND ) )
		pev->movetype = MOVETYPE_NONE;
	if ( pev->deadflag == DEAD_DYING && ( !pev->modelindex || m_fSequenceFinished || m_iRespawnFrames >= 120 ) )
		pev->deadflag = DEAD_DEAD;
	pev->framerate = 0;
	pev->effects |= EF_NOINTERP;
	pev->framerate = 0;
	int buttons = pev->button & ~IN_SCORE;

	// wait for all buttons released
	if ( pev->deadflag == DEAD_DEAD )
	{
		if ( !buttons && g_pGameRules->FPlayerCanRespawn( this ) )
		{
			m_fDeadTime = gpGlobals->time;
			pev->deadflag = DEAD_RESPAWNABLE;
		}
		return;
	}
	if ( g_pGameRules->IsMultiplayer() && gpGlobals->time > m_fDeadTime + 6 && !( m_afPhysicsFlags & PFLAG_OBSERVER ) )
		StartDeathCam();
	if ( buttons || ( g_pGameRules->IsMultiplayer() && forcerespawn.value > 0 && gpGlobals->time > m_fDeadTime + 5 ) )
	{
		pev->button = 0;
		m_iRespawnFrames = 0;

		//ALERT(at_console, "Respawn\n");

		respawn( pev, !( m_afPhysicsFlags & PFLAG_OBSERVER ) ); // don't copy a corpse if we're in deathcam.
		pev->nextthink = -1;
	}
}

void CBasePlayer::WaterMove()
{
	if ( pev->movetype == MOVETYPE_NOCLIP || pev->health < 0 )
		return;

	// waterlevel 0 - not in water
	// waterlevel 1 - feet in water
	// waterlevel 2 - waist in water
	// waterlevel 3 - head in water

	if ( pev->waterlevel == 3 )
	{
		m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
		m_bitsDamageType &= ~DMG_DROWNRECOVER;
		if ( gpGlobals->time <= pev->air_finished )
		{
			m_bitsDamageType &= ~DMG_DROWN;
		}
		else if ( gpGlobals->time > pev->pain_finished )
		{
			pev->dmg = min( pev->dmg + 1, 5 );
			TakeDamage( VARS( ENT( 0 ) ), VARS( ENT( 0 ) ), pev->dmg, DMG_DROWN );
			pev->pain_finished = gpGlobals->time + 1;
			m_idrowndmg = (int)( m_idrowndmg + pev->dmg );
		}
	}
	else
	{
		if ( gpGlobals->time > pev->air_finished )
			EMIT_SOUND( edict(), CHAN_VOICE, "player/pl_wade1.wav", 1, ATTN_NORM );
		else if ( gpGlobals->time + 9 > pev->air_finished )
			EMIT_SOUND( edict(), CHAN_VOICE, "player/pl_wade2.wav", 1, ATTN_NORM );
		pev->air_finished = gpGlobals->time + 12;
		pev->dmg = 2;

		// if we took drowning damage, give it back slowly
		if ( m_idrowndmg > m_idrownrestored )
		{
			m_rgbTimeBasedDamage[itbd_DrownRecover] = 0;
			m_bitsDamageType = ( m_bitsDamageType & ~( DMG_DROWN | DMG_DROWNRECOVER ) ) | DMG_DROWNRECOVER;
		}
	}
	if ( !pev->waterlevel )
	{
		pev->flags &= ~FL_INWATER;
		return;
	}
	float airFinished = pev->air_finished;
	float time = gpGlobals->time;
	if ( !RANDOM_LONG( 0, 31 ) && (int)( airFinished - time ) <= RANDOM_LONG( 0, 11 ) )
	{
		switch ( RANDOM_LONG( 0, 3 ) )
		{
		case 0:
			EMIT_SOUND( edict(), CHAN_STATIC, "player/pl_swim1.wav", 0.8, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND( edict(), CHAN_STATIC, "player/pl_swim2.wav", 0.8, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND( edict(), CHAN_STATIC, "player/pl_swim3.wav", 0.8, ATTN_NORM );
			break;
		case 3:
			EMIT_SOUND( edict(), CHAN_STATIC, "player/pl_swim4.wav", 0.8, ATTN_NORM );
			break;
		}
	}
	if ( pev->watertype == CONTENTS_LAVA )		// do damage
	{
		if ( gpGlobals->time > pev->dmgtime )
			TakeDamage( VARS( ENT( 0 ) ), VARS( ENT( 0 ) ), 10 * pev->waterlevel, DMG_BURN );
	}
	else if ( pev->watertype == CONTENTS_SLIME )		// do damage
	{
		pev->dmgtime = gpGlobals->time + 1;
		TakeDamage( VARS( ENT( 0 ) ), VARS( ENT( 0 ) ), 4 * pev->waterlevel, DMG_ACID );
	}
	if ( !( pev->flags & FL_INWATER ) )
	{
		pev->flags |= FL_INWATER;
		pev->dmgtime = 0;
	}
}

void CBasePlayer::UpdateClientData()
{
	if ( m_fInitHUD )
	{
		m_fInitHUD = FALSE;
		gInitHUD = FALSE;
		MESSAGE_BEGIN( MSG_ONE, gmsgResetHUD, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();
		unsigned short event = PRECACHE_EVENT( 1, "events/blowtorchholster.sc" );
		PLAYBACK_EVENT_FULL( 0, edict(), event, 0, (float*)&g_vecZero, (float*)&g_vecZero, 0, 0,
			(int)( 100 * pev->punchangle.x ), (int)( 100 * pev->punchangle.y ), 0, 0 );
		MESSAGE_BEGIN( MSG_ONE, gmsgSecurityCamera, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();
		if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
		{
			if ( WorldGraph.FSetGraphPointers() )
				ALERT( at_console, "**Graph Pointers Set!\n" );
			else
				ALERT( at_console, "**Graph pointers were not set!\n" );
		}
		if ( !m_fGameHUDInitialized )
		{
			SendInitClientMessages();
			g_pGameRules->InitHUD( this );
			m_fGameHUDInitialized = TRUE;
			if ( g_pGameRules->IsMultiplayer() )
				FireTargets( "game_playerjoin", this, this, USE_TOGGLE, 0 );
		}
		CZ_SpawnSandstormCTs();
		CZ_EquipDirectStart( this );
		FireTargets( "game_playerspawn", this, this, USE_TOGGLE, 0 );
		InitStatusBar();
	}
	else if ( m_bInitClientMsgs )
	{
		if ( WorldGraph.m_fGraphPresent && !WorldGraph.m_fGraphPointersSet )
		{
			if ( WorldGraph.FSetGraphPointers() )
				ALERT( at_console, "**Graph Pointers Set!\n" );
			else
				ALERT( at_console, "**Graph pointers were not set!\n" );
		}
		SendInitClientMessages();
	}
	if ( m_iHideHUD != m_iClientHideHUD )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHideWeapon, NULL, pev );
			WRITE_BYTE( m_iHideHUD );
		MESSAGE_END();
		m_iClientHideHUD = m_iHideHUD;
	}
	if ( m_iFOV != m_iClientFOV )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
			WRITE_BYTE( m_iFOV );
		MESSAGE_END();

		// cache FOV change at end of function, so weapon updates can see that FOV has changed
	}

	// HACKHACK -- send the message to display the game title
	if ( gDisplayTitle )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgShowGameTitle, NULL, pev );
			WRITE_BYTE( 0 );
		MESSAGE_END();
		gDisplayTitle = 0;
	}
	if ( pev->health != m_iClientHealth )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
			WRITE_BYTE( pev->health > 0 ? (int)pev->health : 0 );
		MESSAGE_END();
		m_iClientHealth = (int)pev->health;
	}
	if ( pev->armorvalue != m_iClientBattery )
	{
		m_iClientBattery = (int)pev->armorvalue;
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgBattery, NULL, pev );
			WRITE_SHORT( (int)pev->armorvalue );
		MESSAGE_END();
	}
	if ( pev->dmg_take != 0 || pev->dmg_save != 0 || m_bitsHUDDamage != m_bitsDamageType )
	{
		Vector source = pev->origin;
		if ( pev->dmg_inflictor && pev->dmg_inflictor->pvPrivateData )
			source = ((CBaseEntity*)pev->dmg_inflictor->pvPrivateData)->Center();
		MESSAGE_BEGIN( MSG_ONE, gmsgDamage, NULL, pev );
			WRITE_BYTE( (int)pev->dmg_save );
			WRITE_BYTE( (int)pev->dmg_take );
			WRITE_LONG( m_bitsDamageType & PLAYER_HUD_DAMAGE_MASK );
			WRITE_COORD( source.x );
			WRITE_COORD( source.y );
			WRITE_COORD( source.z );
		MESSAGE_END();
		pev->dmg_take = 0;
		pev->dmg_save = 0;
		m_bitsHUDDamage = m_bitsDamageType;

		// Clear off non-time-based damage indicators
		m_bitsDamageType &= DMG_TIMEBASED;
	}
	if ( m_flFlashLightTime != 0 && !( gpGlobals->time < m_flFlashLightTime ) )
	{
		if ( pev->effects & EF_DIMLIGHT )
		{
			if ( m_iFlashBattery )
			{
				m_iFlashBattery--;
				m_flFlashLightTime = gpGlobals->time + 1.2;
				if ( !m_iFlashBattery )
					FlashlightTurnOff();
			}
		}
		else if ( m_iFlashBattery > 99 )
		{
			m_flFlashLightTime = 0;
		}
		else
		{
			m_flFlashLightTime = gpGlobals->time + 0.2;
			m_iFlashBattery++;
		}
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashBattery, NULL, pev );
			WRITE_BYTE( m_iFlashBattery );
		MESSAGE_END();
	}
	if ( m_iTrain & 0xc0 )
	{
		// send "health" update message
		MESSAGE_BEGIN( MSG_ONE, gmsgTrain, NULL, pev );
			WRITE_BYTE( m_iTrain & 0xf );
		MESSAGE_END();
		m_iTrain &= ~0xc0;
	}

	//
	// New Weapon?
	//
	if ( !m_fKnownItem )
	{
		m_fKnownItem = TRUE;
		for ( int i = 0; i < ARRAYSIZE( CBasePlayerItem::ItemInfoArray ); i++ )
		{
			ItemInfo& item = CBasePlayerItem::ItemInfoArray[i];
			if ( !item.iId )
				continue;
			MESSAGE_BEGIN( MSG_ONE, gmsgWeaponList, NULL, pev );
				WRITE_STRING( item.pszName ? item.pszName : "Empty" );
				WRITE_BYTE( GetAmmoIndex( item.pszAmmo1 ) );
				WRITE_BYTE( item.iMaxAmmo1 );
				WRITE_BYTE( GetAmmoIndex( item.pszAmmo2 ) );
				WRITE_BYTE( item.iMaxAmmo2 );
				WRITE_BYTE( item.iSlot );
				WRITE_BYTE( item.iPosition );
				WRITE_BYTE( item.iId );
				WRITE_BYTE( item.bIsAccessory );
				WRITE_BYTE( item.iFlags );
			MESSAGE_END();
		}
	}
	SendAmmoUpdate();
	if ( m_iAccessoryBits != m_iOldAccessoryBits )
	{
		m_iOldAccessoryBits = m_iAccessoryBits;
		MESSAGE_BEGIN( MSG_ONE, gmsgAccesBits, NULL, pev );
			WRITE_LONG( m_iAccessoryBits );
		MESSAGE_END();
	}
	for ( int i = 0; i < ARRAYSIZE( m_rgpPlayerItems ); i++ )
	{
		if ( m_rgpPlayerItems[i] )  // each item updates it's successors
			m_rgpPlayerItems[i]->UpdateClientData( this );
	}

	// Cache and client weapon change
	m_pClientActiveItem = m_pActiveItem;
	m_iClientFOV = m_iFOV;

	// Update Status Bar
	if ( gpGlobals->time > m_flNextSBarUpdateTime )
	{
		UpdateStatusBar();
		m_flNextSBarUpdateTime = gpGlobals->time + 0.2;
	}
}

void CBasePlayer::UpdateStatusBar()
{
	int state[SBAR_END] = {};
	char text0[SBAR_STRING_SIZE];
	char text1[SBAR_STRING_SIZE];
	strcpy( text0, m_SbarString0 );
	strcpy( text1, m_SbarString1 );
	UTIL_MakeVectors( pev->v_angle + pev->punchangle );
	Vector source = EyePosition();
	TraceResult trace;
	UTIL_TraceLine( source, source + gpGlobals->v_forward * MAX_ID_RANGE, dont_ignore_monsters, edict(), &trace );
	if ( trace.flFraction != 1 )
	{
		if ( !FNullEnt( trace.pHit ) )
		{
			CBaseEntity* entity = CBaseEntity::Instance( trace.pHit );
			if ( entity->Classify() == CLASS_PLAYER )
			{
				state[SBAR_ID_TARGETNAME] = entity->entindex();
				strcpy( text1, "1 %p1\n2 Health: %i2%%\n3 Armor: %i3%%" );
				if ( g_pGameRules->PlayerRelationship( this, entity ) == GR_TEAMMATE )
				{
					state[SBAR_ID_TARGETHEALTH] = (int)( entity->pev->health / entity->pev->max_health * 100 );
					state[SBAR_ID_TARGETARMOR] = (int)entity->pev->armorvalue;
				}
				m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
			}
		}
		else if ( m_flStatusBarDisappearDelay > gpGlobals->time )
		{
			for ( int i = 1; i < SBAR_END; i++ )
				state[i] = m_izSBarState[i];
		}
	}
	bool changed = false;
	if ( strcmp( text0, m_SbarString0 ) )
	{
		changed = true;
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 0 );
			WRITE_STRING( text0 );
		MESSAGE_END();
		strcpy( m_SbarString0, text0 );
	}
	if ( strcmp( text1, m_SbarString1 ) )
	{
		changed = true;
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( 1 );
			WRITE_STRING( text1 );
		MESSAGE_END();
		strcpy( m_SbarString1, text1 );
	}

	// Check values and send if they don't match
	for ( int i = 1; i < SBAR_END; i++ )
	{
		if ( state[i] != m_izSBarState[i] || changed )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
				WRITE_BYTE( i );
				WRITE_SHORT( state[i] );
			MESSAGE_END();
			m_izSBarState[i] = state[i];
		}
	}
}

int TrainSpeed( int speed, int maximum )
{
	if ( speed < 0 )
		return 5;
	if ( !speed )
		return 1;
	float ratio = (float)speed / (float)maximum;
	if ( ratio < 0.33 )
		return 2;
	return ratio >= 0.66 ? 4 : 3;
}

void CBasePlayer::Killed( entvars_t* attacker, int gib )
{
	if ( m_LastHitGroup == HITGROUP_HEAD )
		m_bHeadshotKilled = TRUE;
	if ( !m_bKilledByBomb )
		g_pGameRules->PlayerKilled( this, attacker, g_pevLastInflictor );
	if ( m_pTank )
	{
		m_pTank->Use( this, this, USE_OFF, 0 );
		m_pTank = NULL;
	}
	CSound* sound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( edict() ) );
	if ( sound )
		sound->Reset();
	SetAnimation( PLAYER_DIE );
	pev->modelindex = g_ulModelIndexPlayer;    // don't use eyes
	pev->deadflag = DEAD_DYING;
	pev->movetype = MOVETYPE_TOSS;
	pev->takedamage = DAMAGE_NO;
	pev->gamestate = 1;
	m_bShieldDrawn = FALSE;
	pev->flags &= ~FL_FROZEN;
	pev->flags &= ~FL_ONGROUND;
	if ( m_iThrowDirection )
	{
		switch ( m_iThrowDirection )
		{
		case 1:
		case 2:
			UTIL_MakeVectors( pev->angles );
			pev->velocity = gpGlobals->v_forward * ( m_iThrowDirection == 1
				? RANDOM_FLOAT( 100, 200 ) : RANDOM_FLOAT( -100, -200 ) );
			pev->velocity.z = RANDOM_FLOAT( 50, 100 );
			break;
		case 3:
			if ( FClassnameIs( attacker, "player" ) )
			{
				UTIL_MakeVectors( attacker->angles );
				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT( 200, 300 );
				pev->velocity.z = RANDOM_FLOAT( 200, 300 );
			}
			break;
		case 4:
			pev->velocity = m_vBlastVector / m_vBlastVector.Length() * ( 2300 - m_vBlastVector.Length() ) * 0.25;
			pev->velocity.z = ( 2300 - m_vBlastVector.Length() ) / 2.75;
			break;
		case 5:
			pev->velocity = m_vBlastVector / m_vBlastVector.Length() * ( 500 - m_vBlastVector.Length() );
			pev->velocity.z = ( 350 - m_vBlastVector.Length() ) * 1.5;
			break;
		case 6:
			if ( FClassnameIs( attacker, "player" ) )
			{
				UTIL_MakeVectors( attacker->angles );
				pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT( 200, 300 );
			}
			break;
		}
		m_iThrowDirection = 0;
	}

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate( NULL, FALSE, 0 );

	// send "health" update message to zero
	m_iClientHealth = 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pev );
		WRITE_BYTE( m_iClientHealth );
	MESSAGE_END();

	// Tell Ammo Hud that the player is dead
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
	MESSAGE_END();
	m_iFOV = 0;
	m_iClientFOV = 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	m_bNotKilled = FALSE;
	if ( m_bHasC4 )
	{
		DropPlayerItem( "weapon_c4" );
	}
	else if ( m_bHasDefuser )
	{
		m_bHasDefuser = FALSE;
		pev->body = 0;
		GiveNamedItem( "item_thighpack" );
	}
	m_bIsDefusing = FALSE;
	SetThink( &CBasePlayer::PlayerDeathThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->solid = SOLID_NOT;
	if ( m_bPunishedForTK )
		m_bPunishedForTK = FALSE;
	if ( ( pev->health < -9000 && gib != GIB_NEVER ) || ( gib == GIB_ALWAYS && g_iSkillLevel ) )
	{
		pev->solid = SOLID_NOT;
		GibMonster();	// This clears pev->model
		pev->effects |= EF_NODRAW;
		return;
	}
	DeathSound();
	pev->angles.x = 0;
	pev->angles.z = 0;
}

void CBasePlayer::PreThink()
{
	CZ_MusicPlayerThink( edict() );

	int changed = pev->button ^ m_afButtonLast;
	m_afButtonPressed = changed & pev->button;
	m_afButtonReleased = changed & ~pev->button;
	g_pGameRules->PlayerThink( this );
	if ( g_fGameOver )
		return;         // intermission or finale
	UTIL_MakeVectors( pev->v_angle );             // is this still used?
	ItemPreFrame();
	WaterMove();
	if ( !m_bHasNightVision )
		m_iHideHUD |= HIDEHUD_NIGHTVISION;


	// JOHN: checks if new client data (for HUD and view control) needs to be sent to the client
	UpdateClientData();
	CheckTimeBasedDamage();
	CheckSuitUpdate();
	if ( m_bFirstRestore )
	{
		m_bFirstRestore = FALSE;
		if ( ( pev->effects & EF_NIGHTVISION ) && m_bHasNightVision )
			NightVisionTurnOn();
		if ( m_inFiberCam )
			FireWeapon();
	}
	if ( m_bFirstConnected )
	{
		m_bFirstConnected = FALSE;
		UpdateEntitiesOnClientConnect( edict() );
	}
	if ( pev->deadflag > DEAD_NO )
	{
		PlayerDeathThink();
		return;
	}

	// So the correct flags get sent to client asap.
	//
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		pev->flags |= FL_ONTRAIN;
	else
		pev->flags &= ~FL_ONTRAIN;

	// Train speed control
	if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
	{
		CBaseEntity* train = CBaseEntity::Instance( pev->groundentity );
		if ( !train )
		{
			TraceResult trace;
			UTIL_TraceLine( pev->origin, pev->origin - Vector( 0, 0, 38 ), ignore_monsters, edict(), &trace );
			if ( trace.flFraction == 1 || !trace.pHit )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = 0xc0;
				return;
			}
			train = CBaseEntity::Instance( trace.pHit );
			if ( !train || !( train->ObjectCaps() & FCAP_DIRECTIONAL_USE )
				|| !train->OnControls( pev ) )
			{
				m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
				m_iTrain = 0xc0;
				return;
			}
		}
		else if ( !( pev->flags & FL_ONGROUND ) || ( train->pev->spawnflags & 2 )
			|| ( pev->button & ( IN_MOVELEFT | IN_MOVERIGHT ) ) )
		{
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = 0xc0;
			return;
		}
		pev->velocity = g_vecZero;
		if ( m_afButtonPressed & ( IN_FORWARD | IN_BACK ) )
		{
			train->Use( this, this, USE_SET, ( m_afButtonPressed & IN_FORWARD ) ? 1 : -1 );
			m_iTrain = TrainSpeed( (int)train->pev->speed, train->pev->impulse ) | 0xc0;
		}
	}
	else if ( (signed char)m_iTrain < 0 )
	{
		m_iTrain = 0xc0;
	}
	if ( pev->button & IN_JUMP )
		Jump();
	if ( ( pev->button & IN_DUCK ) || ( pev->flags & FL_DUCKING ) || ( m_afPhysicsFlags & PFLAG_DUCKING ) )
		Duck();
	if ( !( pev->flags & FL_ONGROUND ) )
		m_flFallVelocity = -pev->velocity.z;

	// StudioFrameAdvance( );//!!!HACKHACK!!! Can't be hit by traceline when not animating?

	// Clear out ladder pointer
	m_hEnemy = NULL;
	if ( m_afPhysicsFlags & PFLAG_ONBARNACLE )
		pev->velocity = g_vecZero;
}

void CBasePlayer::PostThink()
{
	if ( g_fGameOver || !IsAlive() )
		return;
	if ( m_iHudIcons )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHudIcons, NULL, pev );
			WRITE_SHORT( m_iHudIcons );
			WRITE_SHORT( m_iHudIconsCorrectWeapon );
			WRITE_SHORT( m_iHudIconsInPosition );
		MESSAGE_END();
		m_iHudIcons = 0;
		m_iHudIconsCorrectWeapon = 0;
		m_iHudIconsInPosition = 0;
	}
	if ( m_toolTrigger && !m_toolTrigger->IsPlayerInTrigger() )
	{
		m_toolTrigger = NULL;
		hudtextparms_t text;
		text.effect = 0;
		text.channel = 0;
		text.fadeinTime = 0.05;
		text.fadeoutTime = 0.05;
		text.holdTime = 1;
		text.fxTime = 0;
		text.x = -1;
		text.y = 0.01;
		text.r1 = 0;
		text.g1 = 0;
		text.b1 = 255;
		text.a1 = 255;
		text.r2 = 255;
		text.g2 = 255;
		text.b2 = 255;
		text.a2 = 255;
		UTIL_HudMessageAll( text, "" );
	}
	bool stopDefuse = !m_toolTrigger && m_defuseTime != 0;
	if ( m_toolTrigger && m_toolTrigger->IsBombDefuseTrigger() && m_defuseTime != 0 )
	{
		if ( !m_toolTrigger->IsPlayerInPosition() )
		{
			stopDefuse = true;
		}
		else if ( !( gpGlobals->time <= m_defuseTime ) )
		{
			m_toolTrigger->FireUseTarget( this, this, USE_TOGGLE, 1 );
			stopDefuse = true;
		}
	}
	if ( stopDefuse )
	{
		SetProgressBarTime( 0 );
		m_defuseTime = 0;
		EMIT_SOUND_DYN( edict(), CHAN_STATIC, "ambience/defuse_loop.wav", 0, 0, SND_STOP, PITCH_NORM );
	}
	if ( m_pTank )
	{
		if ( !m_pTank->OnControls( pev ) || pev->weaponmodel )
		{
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
		}
		else
		{
			m_pTank->Use( this, this, USE_SET, 2 );
		}
	}

// do weapon stuff
	ItemPostFrame();
	if ( pev->flags & FL_ONGROUND )
	{
		if ( pev->health > 0 && m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHHOLD )
		{
			if ( pev->watertype != CONTENTS_WATER && m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
			{
				float damage = g_pGameRules->FlPlayerFallDamage( this );
				if ( damage > pev->health )
					EMIT_SOUND( edict(), CHAN_ITEM, "common/bodysplat.wav", 1, ATTN_NORM );
				if ( damage > 0 )
				{
					TakeDamage( VARS( ENT( 0 ) ), VARS( ENT( 0 ) ), damage, DMG_FALL );
					pev->punchangle.x = 0;
				}
			}
			if ( IsAlive() )
				SetAnimation( PLAYER_WALK );
		}
		if ( pev->flags & FL_ONGROUND )
		{
			if ( m_flFallVelocity > 64 && !g_pGameRules->IsMultiplayer() )
				CSoundEnt::InsertSound( bits_SOUND_PLAYER, pev->origin, (int)m_flFallVelocity, 0.2 );
			m_flFallVelocity = 0;
		}
	}
	if ( IsAlive() )
	{
		if ( pev->velocity.x == 0 && pev->velocity.y == 0 )
			SetAnimation( PLAYER_IDLE );
		else if ( ( pev->flags & FL_ONGROUND ) || pev->waterlevel > 1 )
			SetAnimation( PLAYER_WALK );
	}
	StudioFrameAdvance();
	if ( pev->health > 0 )
		pev->modelindex = g_ulModelIndexPlayer;
	UpdatePlayerSound();

	// Track button info so we can detect 'pressed' and 'released' buttons next frame
	m_afButtonLast = pev->button;
}

void CBasePlayer::PlayerUse()
{
	if ( !( ( m_afButtonReleased | m_afButtonPressed | pev->button ) & IN_USE ) || HandleBombDefuse() )
		return;

	// Hit Use on a train?
	if ( m_afButtonPressed & IN_USE )
	{
		if ( m_pTank )
		{
			// Stop controlling the tank
			// TODO: Send HUD Update
			m_pTank->Use( this, this, USE_OFF, 0 );
			m_pTank = NULL;
			return;
		}
		if ( m_afPhysicsFlags & PFLAG_ONTRAIN )
		{
			m_afPhysicsFlags &= ~PFLAG_ONTRAIN;
			m_iTrain = 0xc0;
			return;
		}
		CBaseEntity* train = CBaseEntity::Instance( pev->groundentity );
		if ( train && !( pev->button & IN_JUMP ) && ( pev->flags & FL_ONGROUND )
			&& ( train->ObjectCaps() & FCAP_DIRECTIONAL_USE ) && train->OnControls( pev ) )
		{
			m_afPhysicsFlags |= PFLAG_ONTRAIN;
			m_iTrain = TrainSpeed( (int)train->pev->speed, train->pev->impulse ) | 0xc0;
			EMIT_SOUND( edict(), CHAN_ITEM, "plats/train_use1.wav", 0.8, ATTN_NORM );
			return;
		}
	}
	CBaseEntity* nearest = NULL;
	CBaseEntity* entity = NULL;
	UTIL_MakeVectors( pev->v_angle );
	float nearestDot = 0.7;
	while ( ( entity = UTIL_FindEntityInSphere( entity, pev->origin, 64 ) ) != NULL )
	{
		if ( !( entity->ObjectCaps() & ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE ) ) )
			continue;
		Vector direction = VecBModelOrigin( entity->pev ) - ( pev->origin + pev->view_ofs );
		direction = UTIL_ClampVectorToBox( direction, entity->pev->size * 0.5 );
		float dot = DotProduct( direction, gpGlobals->v_forward );
		if ( dot > nearestDot )
		{
			nearestDot = dot;
			nearest = entity;
		}
	}
	if ( !nearest )
	{
		if ( m_afButtonPressed & IN_USE )
			EMIT_SOUND( edict(), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_NORM );
		return;
	}
	int caps = nearest->ObjectCaps();
	if ( m_afButtonPressed & IN_USE )
		EMIT_SOUND( edict(), CHAN_ITEM, "common/wpn_select.wav", 0.4, ATTN_NORM );
	if ( ( ( pev->button & IN_USE ) && ( caps & FCAP_CONTINUOUS_USE ) )
		|| ( ( m_afButtonPressed & IN_USE ) && ( caps & ( FCAP_IMPULSE_USE | FCAP_ONOFF_USE ) ) ) )
	{
		if ( caps & FCAP_CONTINUOUS_USE )
			m_afPhysicsFlags |= PFLAG_USING;
		nearest->Use( this, this, USE_SET, 1 );
	}
	// UNDONE: Send different USE codes for ON/OFF.  Cache last ONOFF_USE object to send 'off' if you turn away
	else if ( ( m_afButtonReleased & IN_USE ) && ( nearest->ObjectCaps() & FCAP_ONOFF_USE ) )	// BUGBUG This is an "off" use
	{
		nearest->Use( this, this, USE_SET, 0 );
	}
}

void CBasePlayer::SetAnimation( PLAYER_ANIM animation )
{
	if ( !pev->modelindex )
		return;
	if ( animation == PLAYER_FLINCH || animation == PLAYER_LARGE_FLINCH )
	{
		if ( pev->weapons & ( 1 << WEAPON_SHIELDGUN ) )
			return;
	}
	else if ( m_flFlinchTime >= gpGlobals->time && pev->health > 0 )
	{
		return;
	}
	float speed = pev->velocity.Length2D();
	int hop = LookupActivity( ACT_HOP );
	int leap = LookupActivity( ACT_LEAP );
	if ( pev->flags & FL_FROZEN )
	{
		speed = 0;
		animation = PLAYER_IDLE;
	}
	switch ( animation )
	{
	case PLAYER_IDLE:
	case PLAYER_WALK:
		if ( !( pev->flags & FL_ONGROUND ) && ( m_Activity == ACT_HOP || m_Activity == ACT_LEAP ) )
			m_IdealActivity = m_Activity;
		else if ( pev->waterlevel > 1 )
			m_IdealActivity = speed == 0 ? ACT_HOVER : ACT_SWIM;
		else
			m_IdealActivity = ACT_WALK;
		break;
	case PLAYER_JUMP:
	case PLAYER_SUPERJUMP:
	case PLAYER_ATTACK1:
	case PLAYER_ATTACK2:
	case PLAYER_RELOAD:
		if ( m_Activity == ACT_DIESIMPLE || m_Activity == ACT_HOVER || m_Activity == ACT_SWIM )
		{
			m_IdealActivity = m_Activity;
			return;
		}
		if ( animation == PLAYER_JUMP )
			m_IdealActivity = ACT_HOP;
		else if ( animation == PLAYER_SUPERJUMP )
			m_IdealActivity = ACT_LEAP;
		else if ( animation == PLAYER_ATTACK1 )
			m_IdealActivity = ACT_RANGE_ATTACK1;
		else if ( animation == PLAYER_ATTACK2 )
			m_IdealActivity = ACT_RANGE_ATTACK2;
		else
			m_IdealActivity = ACT_RELOAD;
		break;
	case PLAYER_DIE:
		m_IdealActivity = ACT_DIESIMPLE;
		DeathSound();
		break;
	case PLAYER_FLINCH:
		m_IdealActivity = ACT_FLINCH;
		m_Activity = ACT_FLINCH;
		break;
	case PLAYER_LARGE_FLINCH:
		m_IdealActivity = ACT_LARGE_FLINCH;
		m_Activity = ACT_LARGE_FLINCH;
		break;
	}
	char name[64];
	int sequence = 0;
	switch ( m_IdealActivity )
	{
	case ACT_HOP:
	case ACT_LEAP:
		if ( m_IdealActivity == m_Activity )
			return;
		strcpy( name, "ref_" );
		if ( m_Activity == ACT_RANGE_ATTACK2 )
			strcat( name, "shoot2_" );
		else if ( m_Activity == ACT_RELOAD )
			strcat( name, "reload_" );
		else if ( m_Activity == ACT_RANGE_ATTACK1 )
			strcat( name, "shoot_" );
		else
			strcat( name, "aim_" );
		strcat( name, m_szAnimExtention );
		sequence = LookupSequence( name );
		if ( sequence == -1 )
			sequence = 0;
		if ( sequence != pev->sequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
			if ( !m_fSequenceLoops )
				pev->effects |= EF_NOINTERP;
		}
		pev->gaitsequence = LookupActivity( m_IdealActivity == ACT_LEAP ? ACT_LEAP : ACT_HOP );
		m_Activity = m_IdealActivity;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
		m_flLastFired = gpGlobals->time;
		if ( m_IdealActivity == ACT_RANGE_ATTACK1 )
			strcpy( name, ( pev->flags & FL_DUCKING ) ? "crouch_shoot_" : "ref_shoot_" );
		else
			strcpy( name, ( pev->flags & FL_DUCKING ) ? "crouch_shoot2_" : "ref_shoot2_" );
		strcat( name, m_szAnimExtention );
		sequence = LookupSequence( name );
		if ( sequence == -1 )
			sequence = 0;
		pev->sequence = sequence;
		pev->frame = 0;
		ResetSequenceInfo();
		m_Activity = m_IdealActivity;
		break;
	case ACT_RELOAD:
		strcpy( name, ( pev->flags & FL_DUCKING ) ? "crouch_reload_" : "ref_reload_" );
		strcat( name, m_szAnimExtention );
		sequence = LookupSequence( name );
		if ( sequence == -1 )
			sequence = 0;
		if ( sequence != pev->sequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
			if ( !m_fSequenceLoops )
				pev->effects |= EF_NOINTERP;
		}
		m_Activity = m_IdealActivity;
		break;
	case ACT_WALK:
		if ( ( m_Activity != ACT_FLINCH && m_Activity != ACT_LARGE_FLINCH && m_Activity != ACT_RANGE_ATTACK1
			&& m_Activity != ACT_RANGE_ATTACK2 && m_Activity != ACT_RELOAD ) || m_fSequenceFinished )
		{
			if ( speed > 135 && gpGlobals->time > m_flLastFired + 4 )
			{
				strcpy( name, "run_" );
				strcat( name, m_szAnimExtention );
				sequence = LookupSequence( name );
				if ( sequence == -1 )
				{
					strcpy( name, ( pev->flags & FL_DUCKING ) ? "crouch_aim_" : "ref_aim_" );
					if ( !m_szAnimExtention[0] )
						strcpy( m_szAnimExtention, "knife" );
					strcat( name, m_szAnimExtention );
					sequence = LookupSequence( name );
					if ( sequence == -1 )
						sequence = 0;
					m_Activity = ACT_RUN;
					pev->gaitsequence = LookupActivity( ACT_RUN );
				}
				else
				{
					pev->gaitsequence = sequence;
				}
				m_Activity = ACT_RUN;
			}
			else
			{
				strcpy( name, ( pev->flags & FL_DUCKING ) ? "crouch_aim_" : "ref_aim_" );
				strcat( name, m_szAnimExtention );
				sequence = LookupSequence( name );
				if ( sequence == -1 )
					sequence = 0;
				m_Activity = ACT_WALK;
			}
		}
		else
		{
			sequence = pev->sequence;
		}
		pev->gaitsequence = LookupActivity( speed <= 135 ? ACT_WALK : ACT_RUN );
		break;
	case ACT_DIESIMPLE:
		if ( m_Activity == ACT_DIESIMPLE )
			return;
		m_Activity = ACT_DIESIMPLE;
		m_flDeathThrowTime = 0;
		m_iThrowDirection = 0;
		switch ( m_LastHitGroup )
		{
		case HITGROUP_GENERIC:
			switch ( RANDOM_LONG( 0, 8 ) )
			{
			case 0:
				sequence = LookupActivity( ACT_DIE_HEADSHOT );
				m_iThrowDirection = 2;
				break;
			case 1:
			case 7:
				sequence = LookupActivity( ACT_DIE_GUTSHOT );
				break;
			case 2:
				sequence = LookupActivity( ACT_DIE_BACKSHOT );
				m_iThrowDirection = 3;
				break;
			case 4:
				sequence = LookupActivity( ACT_DIEBACKWARD );
				m_iThrowDirection = 3;
				break;
			case 5:
				sequence = LookupActivity( ACT_DIEFORWARD );
				m_iThrowDirection = 1;
				break;
			case 6:
				sequence = LookupActivity( ACT_DIE_CHESTSHOT );
				break;
			case 8:
				sequence = LookupActivity( ACT_DIE_HEADSHOT );
				break;
			default:
				sequence = LookupActivity( ACT_DIESIMPLE );
				break;
			}
			break;
		case HITGROUP_HEAD:
		{
			int direction = RANDOM_LONG( 0, 8 ) + m_bHighDamage;
			m_bHeadshotKilled = TRUE;
			if ( direction == 1 || direction == 2 )
				m_iThrowDirection = 2;
			else if ( direction == 3 || direction == 4 )
				m_iThrowDirection = 1;
			else if ( direction == 5 || direction == 6 )
				m_iThrowDirection = 3;
			sequence = LookupActivity( ACT_DIE_HEADSHOT );
			break;
		}
		case HITGROUP_CHEST:
			sequence = LookupActivity( ACT_DIE_CHESTSHOT );
			break;
		case HITGROUP_STOMACH:
			sequence = LookupActivity( ACT_DIE_GUTSHOT );
			break;
		case HITGROUP_LEFTARM:
			sequence = LookupSequence( "left" );
			break;
		case HITGROUP_RIGHTARM:
			m_iThrowDirection = RANDOM_LONG( 0, 1 ) ? 3 : 6;
			sequence = LookupSequence( "right" );
			break;
		default:
			sequence = LookupActivity( ACT_DIESIMPLE );
			break;
		}
		if ( pev->flags & FL_DUCKING )
		{
			sequence = LookupSequence( "crouch_die" );
			m_iThrowDirection = 2;
		}
		else if ( m_bKilledByBomb || m_bKilledByGrenade )
		{
			UTIL_MakeVectors( pev->angles );
			if ( DotProduct( gpGlobals->v_forward, m_vBlastVector ) <= 0 )
				sequence = RANDOM_LONG( 0, 1 ) ? LookupSequence( "crouch_die" ) : LookupActivity( ACT_DIE_HEADSHOT );
			else
				sequence = LookupSequence( "left" );
			m_iThrowDirection = m_bKilledByBomb ? 4 : 5;
		}
		if ( sequence == -1 )
			sequence = 0;
		if ( sequence != pev->sequence )
		{
			pev->gaitsequence = 0;
			pev->sequence = sequence;
			pev->frame = 0;
			ResetSequenceInfo();
		}
		return;
	case ACT_FLINCH:
	case ACT_LARGE_FLINCH:
		m_Activity = m_IdealActivity;
		if ( ( animation == PLAYER_FLINCH || animation == PLAYER_LARGE_FLINCH )
			&& ( pev->weapons & ( 1 << WEAPON_SHIELDGUN ) ) && m_bShieldDrawn )
			return;
		if ( m_LastHitGroup == HITGROUP_HEAD || m_LastHitGroup == HITGROUP_CHEST
			|| ( m_LastHitGroup == HITGROUP_GENERIC && RANDOM_LONG( 0, 1 ) ) )
			sequence = LookupSequence( "head_flinch" );
		else if ( m_LastHitGroup != HITGROUP_SHIELD )
			sequence = LookupSequence( "gut_flinch" );
		if ( sequence == -1 )
			sequence = 0;
		break;
	default:
		if ( m_IdealActivity != m_Activity )
		{
			m_Activity = m_IdealActivity;
			sequence = LookupActivity( m_IdealActivity );
			if ( sequence != pev->sequence )
			{
				pev->gaitsequence = 0;
				pev->sequence = sequence;
				pev->frame = 0;
				ResetSequenceInfo();
			}
		}
		return;
	}
	if ( pev->gaitsequence != leap && pev->gaitsequence != hop )
	{
		if ( pev->flags & FL_DUCKING )
		{
			pev->gaitsequence = LookupActivity( speed == 0 ? ACT_CROUCHIDLE : ACT_CROUCH );
		}
		else if ( speed <= 135 )
		{
			pev->gaitsequence = LookupActivity( speed <= 0 ? ACT_IDLE : ACT_WALK );
		}
		else
		{
			bool running = false;
			if ( gpGlobals->time > m_flLastFired + 4 && m_Activity != ACT_FLINCH && m_Activity != ACT_LARGE_FLINCH )
			{
				strcpy( name, "run_" );
				strcat( name, m_szAnimExtention );
				sequence = LookupSequence( name );
				if ( sequence != -1 )
				{
					m_Activity = ACT_RUN;
					pev->gaitsequence = sequence;
					running = true;
				}
				else
				{
					strcpy( name, ( pev->flags & FL_DUCKING ) ? "crouch_aim_" : "ref_aim_" );
					strcat( name, m_szAnimExtention );
					sequence = LookupSequence( name );
					m_Activity = ACT_RUN;
				}
			}
			if ( !running )
				pev->gaitsequence = LookupActivity( ACT_RUN );
		}
	}
	if ( sequence != pev->sequence )
	{
		pev->sequence = sequence;
		pev->frame = 0;
		ResetSequenceInfo();
	}
}

Vector VecVelocityForDamage( float damage )
{
	Vector velocity( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
	if ( damage > -50 )
		return velocity * 0.7f;
	if ( damage > -200 )
		return velocity * 2;
	return velocity * 10;
}

void LogAttack( CBasePlayer* attacker, CBasePlayer* victim, int teamAttack, int healthHit,
	int armorHit, int newHealth, int newArmor, const char* weaponName )
{
}

const char* GetWeaponName( entvars_t* inflictor, entvars_t* killer )
{
	const char* name;
	if ( killer->flags & FL_CLIENT )
	{
		ENTINDEX( ENT( killer ) );
		if ( !inflictor )
			return "world";
		if ( killer == inflictor )
		{
			CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( killer );
			if ( !player || !player->m_pActiveItem )
				return "world";
			CBasePlayerItem* weapon = player->m_pActiveItem;
			name = CBasePlayerItem::ItemInfoArray[weapon->m_iId + ( weapon->m_bIsAccessory ? 32 : 0 )].pszName;
		}
		else
		{
			name = STRING( inflictor->classname );
		}
	}
	else
	{
		name = STRING( inflictor->classname );
	}
	if ( !memcmp( name, "weapon_", 7 ) )
		return name + 7;
	if ( !memcmp( name, "monster_", 8 ) )
		return name + 8;
	if ( !memcmp( name, "func_", 5 ) )
		return name + 5;
	return name;
}

CBaseEntity* FindEntityForward( CBaseEntity* entity )
{
	UTIL_MakeVectors( entity->pev->v_angle );
	Vector source = entity->pev->origin + entity->pev->view_ofs;
	TraceResult trace;
	UTIL_TraceLine( source, source + gpGlobals->v_forward * 8192, dont_ignore_monsters, entity->edict(), &trace );
	if ( trace.flFraction != 1 && !FNullEnt( trace.pHit ) )
		return CBaseEntity::Instance( trace.pHit );
	return NULL;
}

BOOL IsSpawnPointValid( CBaseEntity* player, CBaseEntity* spot )
{
	if ( !spot->IsTriggered( player ) )
		return FALSE;
	CBaseEntity* entity = NULL;
	while ( ( entity = UTIL_FindEntityInSphere( entity, spot->pev->origin, 128 ) ) != NULL )
	{
		BOOL isPlayer = entity->IsPlayer();
		if ( entity != player && isPlayer )
			return FALSE;
	}
	return TRUE;
}

edict_t* EntSelectSpawnPoint( CBaseEntity* player )
{
	CBaseEntity* spot = NULL;

// choose a info_player_deathmatch point
	if ( g_pGameRules->IsCoOp() )
	{
		spot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_coop" );
		if ( !spot || FNullEnt( spot->edict() ) )
			spot = UTIL_FindEntityByClassname( g_pLastSpawn, "info_player_start" );
		if ( spot && !FNullEnt( spot->edict() ) )
		{
			g_pLastSpawn = spot;
			return spot->edict();
		}
	}
	else if ( g_pGameRules->IsDeathmatch() )
	{
		CBaseEntity* start = g_pLastSpawn;
		int count = RANDOM_LONG( 1, 5 );
		while ( count-- > 0 )
			start = UTIL_FindEntityByClassname( start, "info_player_deathmatch" );
		if ( !start || FNullEnt( start->edict() ) )
			start = UTIL_FindEntityByClassname( start, "info_player_deathmatch" );
		spot = start;
		do
		{
			if ( spot && IsSpawnPointValid( player, spot ) && spot->pev->origin != g_vecZero )
			{
				g_pLastSpawn = spot;
				return spot->edict();
			}
			spot = UTIL_FindEntityByClassname( spot, "info_player_deathmatch" );
		} while ( spot != start );
		if ( start && !FNullEnt( start->edict() ) )
		{
			CBaseEntity* entity = NULL;
			while ( ( entity = UTIL_FindEntityInSphere( entity, start->pev->origin, 128 ) ) != NULL )
			{
				if ( entity->IsPlayer() && entity->edict() != player->edict() )
					entity->TakeDamage( VARS( INDEXENT( 0 ) ), VARS( INDEXENT( 0 ) ), 300, DMG_GENERIC );
			}
			g_pLastSpawn = start;
			return start->edict();
		}
	}
	if ( gpGlobals->startspot && STRING( gpGlobals->startspot )[0] )
		spot = UTIL_FindEntityByTargetname( NULL, STRING( gpGlobals->startspot ) );
	else
		spot = UTIL_FindEntityByClassname( NULL, "info_player_start" );
	if ( !spot || FNullEnt( spot->edict() ) )
	{
		ALERT( at_error, "PutClientInServer: no info_player_start on level" );
		return INDEXENT( 0 );
	}
	g_pLastSpawn = spot;
	return spot->edict();
}

void FixPlayerCrouchStuck( edict_t* player )
{

	// Move up as many as 18 pixels if the player is stuck.
	for ( int i = 0; i < 18; i++ )
	{
		TraceResult trace;
		UTIL_TraceHull( player->v.origin, player->v.origin, dont_ignore_monsters, head_hull, player, &trace );
		if ( !trace.fStartSolid )
			break;
		player->v.origin.z++;
	}
}

void UpdateEntitiesOnClientConnect( edict_t* player )
{
	edict_t* entity = INDEXENT( 1 );
	if ( !player )
		return;
	if ( !entity )
		entity = player;
	int count = 0;
	for ( int i = 1; i < gpGlobals->maxEntities; i++, entity++ )
	{
		if ( count > NUMBER_OF_ENTITIES() )
			break;
		if ( entity->free )
			continue;
		count++;
		if ( entity != player && entity->pvPrivateData )
			((CBaseEntity*)entity->pvPrivateData)->UpdateOnClientConnect( player );
	}
}

void LinkUserMessages()
{
	// Already taken care of?
	if ( gmsgSelAmmo )
		return;
	gmsgSelAmmo = REG_USER_MSG( "SelAmmo", 4 );
	gmsgCurWeapon = REG_USER_MSG( "CurWeapon", 4 );
	gmsgGeigerRange = REG_USER_MSG( "Geiger", 1 );
	gmsgFlashlight = REG_USER_MSG( "Flashlight", 2 );
	gmsgFlashBattery = REG_USER_MSG( "FlashBat", 1 );
	gmsgHealth = REG_USER_MSG( "Health", 1 );
	gmsgDamage = REG_USER_MSG( "Damage", 12 );
	gmsgBattery = REG_USER_MSG( "Battery", 2 );
	gmsgTrain = REG_USER_MSG( "Train", 1 );
	//gmsgHudText = REG_USER_MSG( "HudTextPro", -1 );
	gmsgHudText = REG_USER_MSG( "HudText", -1 ); // we don't use the message but 3rd party addons may!
	gmsgSayText = REG_USER_MSG( "SayText", -1 );
	gmsgTextMsg = REG_USER_MSG( "TextMsg", -1 );
	gmsgWeaponList = REG_USER_MSG( "WeaponList", -1 );
	gmsgResetHUD = REG_USER_MSG( "ResetHUD", 1 );		// called every respawn
	gmsgInitHUD = REG_USER_MSG( "InitHUD", 14 );		// called every time a new player joins the server
	gmsgShowGameTitle = REG_USER_MSG( "GameTitle", 1 );
	gmsgDeathMsg = REG_USER_MSG( "DeathMsg", -1 );
	gmsgScoreInfo = REG_USER_MSG( "ScoreInfo", 9 );
	gmsgTeamInfo = REG_USER_MSG( "TeamInfo", -1 );  // sets the name of a player's team
	gmsgTeamScore = REG_USER_MSG( "TeamScore", -1 );  // sets the score of a team on the scoreboard
	gmsgGameMode = REG_USER_MSG( "GameMode", 1 );
	gmsgMOTD = REG_USER_MSG( "MOTD", -1 );
	gmsgServerName = REG_USER_MSG( "ServerName", -1 );
	gmsgAmmoPickup = REG_USER_MSG( "AmmoPickup", 2 );
	gmsgWeapPickup = REG_USER_MSG( "WeapPickup", 2 );
	gmsgItemPickup = REG_USER_MSG( "ItemPickup", -1 );
	gmsgHideWeapon = REG_USER_MSG( "HideWeapon", 1 );
	gmsgSetFOV = REG_USER_MSG( "SetFOV", 1 );
	gmsgShowMenu = REG_USER_MSG( "ShowMenu", -1 );
	gmsgShake = REG_USER_MSG( "ScreenShake", 6 );
	gmsgFade = REG_USER_MSG( "ScreenFade", 10 );
	gmsgAmmoX = REG_USER_MSG( "AmmoX", 2 );
	gmsgTeamNames = REG_USER_MSG( "TeamNames", -1 );
	gmsgParticle = REG_USER_MSG( "Particles", -1 );
	gmsgPartState = REG_USER_MSG( "PartState", -1 );
	gmsgReceiveW = REG_USER_MSG( "ReceiveW", 1 );
	gmsgStatusText = REG_USER_MSG( "StatusText", -1 );
	gmsgStatusValue = REG_USER_MSG( "StatusValue", 3 );
	gmsgAccesBits = REG_USER_MSG( "AccesBits", -1 );
	gmsgNightVision = REG_USER_MSG( "NightVision", 1 );
	gmsgSniperLaser = REG_USER_MSG( "SniperLaser", 1 );
	gmsgFiberCamera = REG_USER_MSG( "FiberCamera", -1 );
	gmsgSecurityCamera = REG_USER_MSG( "SecurityCam", -1 );
	gmsgHudIcons = REG_USER_MSG( "HudIcons", 6 );
	gmsgBarTime = REG_USER_MSG( "BarTime", 2 );
	gmsgLevelLocks = REG_USER_MSG( "LevelLocks", 4 );
	gmsgWorldMap = REG_USER_MSG( "MapInfo", 0 );
	gmsgWindInfo = REG_USER_MSG( "WindInfo", -1 );
	gmsgGrassInfo = REG_USER_MSG( "GrassInfo", -1 );
}

/*
============
ImpulseCommands
============
*/
void CBasePlayer::ImpulseCommands()
{
	if ( !( gpGlobals->frametime > 0 ) )
		return;

	// Handle use events
	PlayerUse();
	switch ( pev->impulse )
	{
	case 98:
		DisplayObjectives();
		break;
	case 99:
		if ( gmsgLogo )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgLogo, NULL, pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();
			gmsgLogo = 0;
		}
		else
		{
			gmsgLogo = REG_USER_MSG( "Logo", 1 );
			MESSAGE_BEGIN( MSG_ONE, gmsgLogo, NULL, pev );
				WRITE_BYTE( 1 );
			MESSAGE_END();
		}
		break;
	case 100:
		if ( pev->effects & EF_NIGHTVISION )
			NightVisionTurnOff();
		else if ( m_bHasNightVision )
			NightVisionTurnOn();
		break;
	case 201:
		if ( !( m_flNextDecalTime > gpGlobals->time ) )
		{
			UTIL_MakeVectors( pev->v_angle );
			TraceResult trace;
			Vector start = pev->origin + pev->view_ofs;
			UTIL_TraceLine( start, start + gpGlobals->v_forward * 128, ignore_monsters, edict(), &trace );
			if ( trace.flFraction != 1 )
			{
				m_flNextDecalTime = gpGlobals->time + decalfrequency.value;
				CSprayCan* spray = GetClassPtr( (CSprayCan*)NULL );
				spray->Spawn( pev );
			}
		}
		break;
	default:
		CheatImpulseCommands( pev->impulse );
		break;
	}
	pev->impulse = 0;
}

//=========================================================
//=========================================================
void CBasePlayer::CheatImpulseCommands( int impulse )
{
	if ( CVAR_GET_FLOAT( "sv_cheats" ) == 0 )
		return;
	switch ( impulse )
	{
	case 76:
		UTIL_MakeVectors( Vector( 0, pev->v_angle.y, 0 ) );
		CBaseEntity::Create( "monster_terrorist_arctic_pistol", pev->origin + gpGlobals->v_forward * 128, pev->angles );
		break;
	case 101:
		m_bHasNightVision = TRUE;
		gEvilImpulse101 = 1;
		GiveNamedItem( "weapon_awp" );
		GiveNamedItem( "weapon_ak47" );
		GiveNamedItem( "weapon_deagle" );
		GiveNamedItem( "weapon_knife" );
		GiveNamedItem( "weapon_mp5navy" );
		GiveNamedItem( "weapon_flashbang" );
		GiveNamedItem( "weapon_hegrenade" );
		GiveNamedItem( "weapon_smokegrenade" );
		gEvilImpulse101 = 0;
		break;
	case 102:
		// Gibbage!!!
		CGib::SpawnRandomGibs( pev, 1, 1 );
		break;
	case 103:
	{
		CBaseEntity* entity = FindEntityForward( this );
		if ( entity && entity->MyMonsterPointer() )
			entity->MyMonsterPointer()->ReportAIState();
		break;
	}
	case 104:
		// Dump all of the global state varaibles (and global entity names)
		gGlobalState.DumpGlobals();
		break;
	case 105: // player makes no sound for monsters to hear.
		if ( m_fNoPlayerSound )
		{
			ALERT( at_console, "Player is audible\n" );
			m_fNoPlayerSound = FALSE;
		}
		else
		{
			ALERT( at_console, "Player is silent\n" );
			m_fNoPlayerSound = TRUE;
		}
		break;
	case 106:
	{
		CBaseEntity* entity = FindEntityForward( this );
		if ( entity )
		{
			ALERT( at_console, "Classname: %s", STRING( entity->pev->classname ) );
			if ( entity->pev->targetname )
				ALERT( at_console, " - Targetname: %s\n", STRING( entity->pev->targetname ) );
			else
				ALERT( at_console, " - TargetName: No Targetname\n" );
			ALERT( at_console, "Model: %s\n", STRING( entity->pev->model ) );
			if ( entity->pev->globalname )
				ALERT( at_console, "Globalname: %s\n", STRING( entity->pev->globalname ) );
		}
		break;
	}
	case 107:
	{
		edict_t* world = INDEXENT( 0 );
		Vector start = pev->origin + pev->view_ofs;
		Vector end = start + gpGlobals->v_forward * 1024;
		TraceResult trace;
		UTIL_TraceLine( start, end, ignore_monsters, edict(), &trace );
		const char* texture = TRACE_TEXTURE( trace.pHit ? trace.pHit : world, start, end );
		if ( texture )
			ALERT( at_console, "Texture: %s\n", texture );
		break;
	}
	case 108:
		for ( int i = 0; i < 3; i++ )
		{
			GiveNamedItem( "ammo_338magnum" );
			GiveNamedItem( "ammo_762nato" );
			GiveNamedItem( "ammo_762natobox" );
			GiveNamedItem( "ammo_556natobox" );
			GiveNamedItem( "ammo_556nato" );
			GiveNamedItem( "ammo_buckshot" );
			GiveNamedItem( "ammo_45acp" );
			GiveNamedItem( "ammo_57mm" );
			GiveNamedItem( "ammo_50ae" );
			GiveNamedItem( "ammo_357sig" );
			GiveNamedItem( "ammo_9mm" );
		}
		// fall through
	case 195:
		CBaseEntity::Create( "node_viewer_fly", pev->origin, pev->angles );
		break;
	case 196:
		CBaseEntity::Create( "node_viewer_large", pev->origin, pev->angles );
		break;
	case 197:
		CBaseEntity::Create( "node_viewer_human", pev->origin, pev->angles );
		break;
	case 199:
		ALERT( at_console, "%d\n", WorldGraph.FindNearestNode( pev->origin, 7 ) );
		WorldGraph.ShowNodeConnections( WorldGraph.FindNearestNode( pev->origin, 7 ) );
		break;
	case 202:
	{
		UTIL_MakeVectors( pev->v_angle );
		Vector start = pev->origin + pev->view_ofs;
		TraceResult trace;
		UTIL_TraceLine( start, start + gpGlobals->v_forward * 128, ignore_monsters, edict(), &trace );
		if ( trace.flFraction != 1 )
		{
			CBloodSplat* blood = GetClassPtr( (CBloodSplat*)NULL );
			blood->Spawn( pev );
		}
		break;
	}
	case 203:
	{
		CBaseEntity* entity = FindEntityForward( this );
		if ( entity && entity->pev->takedamage )
			entity->SetThink( &CBaseEntity::SUB_Remove );
		break;
	}
	}
}

int CBasePlayer::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	float previousHealth = pev->health;
	ALERT( at_aiconsole, "player damage: t %.2f inflictor %s (%s) attacker %s dmg %.1f type 0x%x health %.0f vel %.0f %.0f %.0f\n", gpGlobals->time,
		inflictor ? STRING( inflictor->classname ) : "none", inflictor ? STRING( inflictor->targetname ) : "",
		attacker ? STRING( attacker->classname ) : "none", damage, damageType, pev->health, pev->velocity.x, pev->velocity.y, pev->velocity.z );
	Vector inflictorOrigin = CBaseEntity::Instance( inflictor )->pev->origin;
	if ( m_pActiveItem && FClassnameIs( m_pActiveItem->pev, "weapon_shieldgun" )
		&& FInViewCone( &inflictorOrigin )
		&& ( ((CBasePlayerWeapon*)m_pActiveItem)->m_iWeaponState & WPNSTATE_SHIELD_DRAWN ) )
		m_LastHitGroup = HITGROUP_SHIELD;
	if ( m_LastHitGroup == HITGROUP_SHIELD && !( damageType & ( DMG_BLAST | DMG_EXPLOSION ) ) )
	{
		float distance = ( pev->origin - inflictor->origin ).Length();
		if ( distance > 256 )
			return 0;
		damage = (int)( 48 / distance * damage * 0.75f );
	}
	float shieldRatio = ( pev->weapons & ( 1 << WEAPON_SHIELDGUN ) ) ? 0.2f : 0;
	float armourRatio = m_bIsVIP ? 0.25f : 0.5f;
	if ( damageType & ( DMG_BLAST | DMG_EXPLOSION ) )
	{
		if ( !IsAlive() )
			return 0;
		CBaseEntity* source = GetClassPtr( (CBaseEntity*)inflictor );
		BOOL molotov = FClassnameIs( source->pev, "molotov" );
		if ( FClassnameIs( source->pev, "grenade" ) || FClassnameIs( source->pev, "law_rocket" ) || molotov )
		{
			CGrenade* grenade = GetClassPtr( (CGrenade*)inflictor );
			if ( molotov )
				UTIL_ScreenFade( this, Vector( 209, 48, 7 ), 0.2, 0.8, 102, 0 );
			if ( CVAR_GET_FLOAT( "mp_friendlyfire" ) == 0 )
			{
				if ( grenade->m_iTeam == m_iTeam && attacker != VARS( edict() ) )
					return 0;
			}
			else
			{
				ENT( attacker );
			}
		}
		if ( inflictor->pContainingEntity && !FNullEnt( inflictor->pContainingEntity ) )
			m_vBlastVector = pev->origin - inflictor->origin;
		if ( m_iKevlar )
		{
			if ( m_iKevlar == 1 && ( m_LastHitGroup == HITGROUP_GENERIC
				|| ( m_LastHitGroup >= HITGROUP_CHEST && m_LastHitGroup <= HITGROUP_RIGHTARM ) ) )
			{
				float healthDamage = armourRatio * damage;
				float armourDamage = damage - healthDamage;
				if ( armourDamage <= pev->armorvalue )
				{
					pev->armorvalue -= armourDamage;
					damage = healthDamage;
				}
				else
				{
					damage -= pev->armorvalue;
					pev->armorvalue = 0;
				}
				if ( pev->armorvalue <= 0 )
					m_iKevlar = 0;
			}
			Pain( m_LastHitGroup, true );
		}
		else
		{
			Pain( m_LastHitGroup, false );
		}
		m_lastDamageAmount = (int)damage;
		damage = (int)damage;
		if ( damage < pev->health )
		{
			SetAnimation( PLAYER_FLINCH );
			Pain( m_LastHitGroup, false );
		}
		else if ( damageType & DMG_BLAST )
		{
			m_bKilledByBomb = TRUE;
		}
		else if ( damageType & DMG_EXPLOSION )
		{
			m_bKilledByGrenade = TRUE;
		}
		GetWeaponName( inflictor, attacker );
		float tookDamage = (float)CBaseMonster::TakeDamage( inflictor, attacker, damage, damageType );
		for ( int i = 0; i < CDMG_TIMEBASED; i++ )
		{
			if ( damageType & ( DMG_PARALYZE << i ) )
				m_rgbTimeBasedDamage[i] = 0;
		}
		MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
			WRITE_BYTE( 9 );
			WRITE_BYTE( DRC_CMD_EVENT );
			WRITE_SHORT( entindex() );
			WRITE_SHORT( ENTINDEX( ENT( inflictor ) ) );
			WRITE_LONG( 5 );
		MESSAGE_END();
		return (int)tookDamage;
	}
	if ( !g_pGameRules->FPlayerCanTakeDamage( this, CBaseEntity::Instance( attacker ) )
		&& !FClassnameIs( inflictor, "grenade" ) )
		return 0;
	float armourBonus = 0.5;
	float armourInverse = 2;
	if ( ( damageType & DMG_BLAST ) && g_pGameRules->IsMultiplayer() )
	{
		armourBonus = 1;
		armourInverse = 1;
	}

	// Already dead
	if ( !IsAlive() )
		return 0;
	CBaseEntity* attackingEntity = GetClassPtr( (CBaseEntity*)attacker );
	if ( attackingEntity && attackingEntity->IsPlayer() )
	{
		CBasePlayer* player = GetClassPtr( (CBasePlayer*)attacker );
		if ( player->m_iTeam == m_iTeam )
		{
			if ( !( m_flDisplayHistory & 0x200 ) )
			{
				m_flDisplayHistory |= 0x200;
				if ( player->IsAlive() && player->m_bShowHints )
					player->m_hintMessageQueue.AddMessage( "#Hint_try_not_to_injure_teammates" );
			}
			if ( gpGlobals->time > player->m_flLastAttackedTeammate + 0.6 )
			{
				player->m_flLastAttackedTeammate = gpGlobals->time;
				CBaseEntity* entity = NULL;
				while ( ( entity = UTIL_FindEntityByClassname( entity, "player" ) ) != NULL )
				{
					if ( FNullEnt( entity->edict() ) )
						break;
					CBasePlayer* teammate = GetClassPtr( (CBasePlayer*)entity->pev );
					if ( teammate->m_iTeam == m_iTeam )
						ClientPrint( teammate->pev, HUD_PRINTTALK, "#Game_teammate_attack", STRING( player->pev->netname ) );
				}
			}
			damage *= 0.35;
		}
#ifdef _WIN32
		unsigned int gunType = WEAPON_NONE;
#else
		unsigned int gunType;
#endif
		if ( player->m_pActiveItem )
		{
			gunType = player->m_pActiveItem->m_iId;
			armourRatio += shieldRatio;
			if ( !player->m_pActiveItem->m_bIsAccessory )
			{
				switch ( gunType )
				{
				case WEAPON_P228:
					armourRatio *= 1.25;
					break;
				case WEAPON_SCOUT:
					armourRatio *= 1.7;
					break;
				case WEAPON_MAC10:
					armourRatio *= 0.95;
					break;
				case WEAPON_AUG:
				case WEAPON_M4A1:
				case WEAPON_SG552:
				case WEAPON_FAMAS:
					armourRatio *= 1.4;
					break;
				case WEAPON_ELITE:
				case WEAPON_GLOCK18:
					armourRatio *= 1.05;
					break;
				case WEAPON_FIVESEVEN:
				case WEAPON_M249:
				case WEAPON_DEAGLE:
				case WEAPON_P90:
				case WEAPON_SHIELDGUN:
					armourRatio *= 1.5;
					break;
				case WEAPON_SG550:
					armourRatio *= 1.45;
					break;
				case WEAPON_AWP:
					armourRatio *= 1.95;
					break;
				case WEAPON_G3SG1:
					armourRatio *= 1.65;
					break;
				case WEAPON_AK47:
				case WEAPON_GALIL:
					armourRatio *= 1.55;
					break;
				case WEAPON_M60:
					armourRatio *= 1.8;
					break;
				}
			}
			else if ( gunType == 9 || gunType == 10 )
			{
				armourRatio *= 1.7;
			}
			else if ( gunType == ACCESSORY_LAWS )
			{
				armourRatio *= 1.5;
			}
		}
		if ( !( pev->flags & FL_DUCKING ) && gunType <= WEAPON_GALIL
			&& ( ( PLAYER_LARGE_FLINCH_WEAPONS >> gunType ) & 1 ) )
		{
			if ( pev->velocity.Length() < 300 )
			{
				pev->velocity = pev->velocity + ( pev->origin - player->pev->origin ).Normalize() * 170;
				m_flVelocityModifier = 0.65;
			}
			SetAnimation( PLAYER_LARGE_FLINCH );
		}
		else
		{
			m_flVelocityModifier = 0.5;
			m_bHighDamage = damage > ( m_LastHitGroup == HITGROUP_HEAD ? 60 : 20 );
			SetAnimation( PLAYER_FLINCH );
		}
	}
	m_lastDamageAmount = (int)damage;
	if ( !m_iKevlar || ( damageType & ( DMG_FALL | DMG_DROWN ) ) )
	{
		Pain( m_LastHitGroup, false );
	}
	else
	{
		float healthDamage = armourRatio * damage;
		float armourDamage = ( damage - healthDamage ) * armourBonus;
		if ( armourDamage > pev->armorvalue )
		{
			healthDamage = damage - pev->armorvalue * armourInverse;
			pev->armorvalue = 0;
		}
		else
		{
			pev->armorvalue -= armourDamage;
		}
		if ( pev->armorvalue <= 0 )
			m_iKevlar = 0;
		Pain( m_LastHitGroup, true );
		damage = healthDamage;
	}
	GetWeaponName( inflictor, attacker );
	int tookDamage = CBaseMonster::TakeDamage( inflictor, attacker, (int)damage, damageType );
	for ( int i = 0; i < CDMG_TIMEBASED; i++ )
	{
		if ( damageType & ( DMG_PARALYZE << i ) )
			m_rgbTimeBasedDamage[i] = 0;
	}
	GetCTHelp( CBaseEntity::Instance( attacker ) );

	// tell director about it
	MESSAGE_BEGIN( MSG_SPEC, SVC_DIRECTOR );
		WRITE_BYTE( 9 );	// command length in bytes
		WRITE_BYTE( DRC_CMD_EVENT );	// take damage event
		WRITE_SHORT( entindex() );
		WRITE_SHORT( ENTINDEX( ENT( inflictor ) ) );
		WRITE_LONG( 5 );
	MESSAGE_END();
	BOOL trivial = pev->health > 75 || m_lastDamageAmount < 5;
	BOOL major = m_lastDamageAmount > 25;
	BOOL critical = pev->health < 30;
	if ( damageType & DMG_SHOCK )
		EMIT_SOUND( edict(), CHAN_ITEM, RANDOM_LONG( 0, 99 ) <= 49 ? "player/electro1.wav" : "player/electro2.wav", 1, ATTN_NORM );
	if ( damageType & DMG_BURN )
		EMIT_SOUND( edict(), CHAN_ITEM, RANDOM_LONG( 0, 99 ) <= 49 ? "player/burn1.wav" : "player/burn2.wav", 1, ATTN_NORM );
	m_bitsDamageType |= damageType;
	m_bitsHUDDamage = -1;
	int remainingDamage = damageType;
	BOOL found = TRUE;
	while ( tookDamage && ( !trivial || ( remainingDamage & DMG_TIMEBASED ) ) && found && remainingDamage )
	{
		found = FALSE;
		if ( remainingDamage & DMG_CLUB )
		{
			if ( major )
				SetSuitUpdate( "!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC );
			remainingDamage &= ~DMG_CLUB;
			found = TRUE;
		}
		if ( remainingDamage & ( DMG_FALL | DMG_CRUSH ) )
		{
			SetSuitUpdate( major ? "!HEV_DMG5" : "!HEV_DMG4", FALSE, SUIT_NEXT_IN_30SEC );
			remainingDamage &= ~( DMG_FALL | DMG_CRUSH );
			found = TRUE;
		}
		if ( remainingDamage & DMG_BULLET )
		{
			if ( m_lastDamageAmount > 5 )
				SetSuitUpdate( "!HEV_DMG6", FALSE, SUIT_NEXT_IN_30SEC );	// blood loss detected
			remainingDamage &= ~DMG_BULLET;
			found = TRUE;
		}
		if ( remainingDamage & DMG_SLASH )
		{
			SetSuitUpdate( major ? "!HEV_DMG1" : "!HEV_DMG0", FALSE, SUIT_NEXT_IN_30SEC );
			remainingDamage &= ~DMG_SLASH;
			found = TRUE;
		}
		if ( remainingDamage & DMG_SONIC )
		{
			if ( major )
				SetSuitUpdate( "!HEV_DMG2", FALSE, SUIT_NEXT_IN_1MIN );	// internal bleeding
			remainingDamage &= ~DMG_SONIC;
			found = TRUE;
		}
		if ( remainingDamage & ( DMG_POISON | DMG_PARALYZE ) )
		{
			SetSuitUpdate( "!HEV_DMG3", FALSE, SUIT_NEXT_IN_1MIN );	// blood toxins detected
			remainingDamage &= ~( DMG_POISON | DMG_PARALYZE );
			found = TRUE;
		}
		if ( remainingDamage & DMG_ACID )
		{
			SetSuitUpdate( "!HEV_DET1", FALSE, SUIT_NEXT_IN_1MIN );	// hazardous chemicals detected
			remainingDamage &= ~DMG_ACID;
			found = TRUE;
		}
		if ( remainingDamage & DMG_NERVEGAS )
		{
			SetSuitUpdate( "!HEV_DET0", FALSE, SUIT_NEXT_IN_1MIN );	// biohazard detected
			remainingDamage &= ~DMG_NERVEGAS;
			found = TRUE;
		}
		if ( remainingDamage & DMG_RADIATION )
		{
			SetSuitUpdate( "!HEV_DET2", FALSE, SUIT_NEXT_IN_1MIN );	// radiation detected
			remainingDamage &= ~DMG_RADIATION;
			found = TRUE;
		}
		if ( remainingDamage & DMG_SHOCK )
		{
			remainingDamage &= ~DMG_SHOCK;
			found = TRUE;
		}
	}
	if ( tookDamage && !trivial && major && previousHealth >= 75 )
	{
		// first time we take major damage...
		// turn automedic on if not on
		SetSuitUpdate( "!HEV_MED1", FALSE, SUIT_NEXT_IN_30MIN );	// automedic on

		// give morphine shot if not given recently
		SetSuitUpdate( "!HEV_HEAL7", FALSE, SUIT_NEXT_IN_30MIN );	// morphine shot
	}
	if ( tookDamage && !trivial && critical && previousHealth < 75 )
	{

		// already took major damage, now it's critical...
		if ( pev->health < 6 )
			SetSuitUpdate( "!HEV_HLTH3", FALSE, SUIT_NEXT_IN_10MIN );	// near death
		else if ( pev->health < 20 )
			SetSuitUpdate( "!HEV_HLTH2", FALSE, SUIT_NEXT_IN_10MIN );	// health critical

		// give critical health warnings
		if ( !RANDOM_LONG( 0, 3 ) && previousHealth < 50 )
			SetSuitUpdate( "!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN );
	}

	// if we're taking time based damage, warn about its continuing effects
	if ( tookDamage && ( damageType & DMG_TIMEBASED ) && previousHealth < 75 )
	{
		if ( previousHealth < 50 )
		{
			if ( !RANDOM_LONG( 0, 3 ) )
				SetSuitUpdate( "!HEV_DMG7", FALSE, SUIT_NEXT_IN_5MIN ); //seek medical attention
		}
		else
		{
			SetSuitUpdate( "!HEV_HLTH1", FALSE, SUIT_NEXT_IN_10MIN );	// health dropping
		}
	}
	return tookDamage;
}
