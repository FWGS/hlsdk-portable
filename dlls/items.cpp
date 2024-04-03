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
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "decals.h"
#include "actanimating.h"

#define seqEmitterClosed   0
#define seqEmitterDeploy   1
#define seqEmitterIdleOpen 2
#define seqEmitterBroken1  3
#define seqEmitterBroken2  4
#define seqEmitterDeath    5

extern int gmsgItemPickup;

class CWorldItem : public CBaseEntity
{
public:
	void KeyValue( KeyValueData *pkvd ); 
	void Spawn( void );
	int m_iType;
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem )

void CWorldItem::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch( m_iType ) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if( !pEntity )
	{
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	}
	else
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY( edict() );
}

void CItem::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CItem::ItemTouch );

	if( DROP_TO_FLOOR(ENT( pev ) ) == 0 )
	{
		ALERT(at_error, "Item %s fell out of level at %f,%f,%f\n", STRING( pev->classname ), (double)pev->origin.x, (double)pev->origin.y, (double)pev->origin.z);
		UTIL_Remove( this );
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if( !pOther->IsPlayer() )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if( MyTouch( pPlayer ) )
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if( gEvilImpulse101 )
	{
		UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
	SetThink( NULL );
}

#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_suit.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_suit.mdl" );
	}
	void KeyValue(KeyValueData *pkvd)
	{
		if (FStrEq(pkvd->szKeyName, "skin"))
		{
			pev->skin = atoi(pkvd->szValue);
			pkvd->fHandled = TRUE;
		}
		else
			CBaseEntity::KeyValue( pkvd );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->weapons & ( 1<<WEAPON_SUIT ) )
			return FALSE;

		if (!g_startSuit)
		{
			if( pev->spawnflags & SF_SUIT_SHORTLOGON )
				EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A0" );		// short version of suit logon,
			else
				EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_AAx" );	// long version of suit logon
		}

		pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_suit, CItemSuit )

class CItemBattery : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_battery.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_battery.mdl" );
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->pev->deadflag != DEAD_NO )
		{
			return FALSE;
		}

		if( ( pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY ) &&
			( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = Q_min( pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY );

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)( (float)( pPlayer->pev->armorvalue * 100.0f ) * ( 1.0f / MAX_NORMAL_BATTERY ) + 0.5f );
			pct = ( pct / 5 );
			if( pct > 0 )
				pct--;

			sprintf( szcharge,"!HEV_%1dP", pct );

			//EMIT_SOUND_SUIT( ENT( pev ), szcharge );
			pPlayer->SetSuitUpdate( szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_battery, CItemBattery )

class CItemAntidote : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_antidote.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_antidote.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->SetSuitUpdate( "!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN );

		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_antidote, CItemAntidote )

class CItemSecurity : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_security.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_security.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_security, CItemSecurity )

class CItemLongJump : public CItem
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_longjump.mdl" );
		CItem::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_longjump.mdl" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if( ( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING( pev->classname ) );
			MESSAGE_END();

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump )

//
//	Decay's item_slave_collar for mission ht11lasers (Gamma labs)
//

class CItemSlaveCollar : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	EXPORT Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	EXPORT ZapThink( void );
	void    EXPORT OffThink( void );

	bool	m_bIsOn;
	int		m_iBeams;
	CBeam *m_pBeam[8];	// ISLAVE_MAX_BEAMS

	Vector	m_vecDir;
	Vector  m_vecEnd;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
};
LINK_ENTITY_TO_CLASS( item_slave_collar, CItemSlaveCollar );

TYPEDESCRIPTION	CItemSlaveCollar::m_SaveData[] =
{
	DEFINE_FIELD( CItemSlaveCollar, m_bIsOn, FIELD_BOOLEAN ),
	DEFINE_ARRAY( CItemSlaveCollar, m_pBeam, FIELD_CLASSPTR, 8 ),
	DEFINE_FIELD( CItemSlaveCollar, m_iBeams, FIELD_INTEGER),
	DEFINE_FIELD( CItemSlaveCollar, m_vecDir, FIELD_VECTOR),
	DEFINE_FIELD( CItemSlaveCollar, m_vecEnd, FIELD_VECTOR),
};
IMPLEMENT_SAVERESTORE( CItemSlaveCollar, CBaseEntity );

void	CItemSlaveCollar::Spawn( void )
{
    Precache();
	SET_MODEL(ENT(pev), "models/collar_test.mdl");

	UTIL_MakeAimVectors( pev->angles );

	m_vecDir = gpGlobals->v_forward;
	m_vecEnd = pev->origin + m_vecDir * 2048;

	for ( m_iBeams = 0; m_iBeams < 2; m_iBeams++ )
	{
		m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
		m_pBeam[m_iBeams]->pev->effects |= EF_NODRAW;
	}

	if (FBitSet(pev->spawnflags, 1)) // Start on
	{
		m_bIsOn = true;
		SetThink(&CItemSlaveCollar::ZapThink);	// start zapping
		pev->nextthink = gpGlobals->time;
	}
}

void	CItemSlaveCollar::Precache( void )
{
	PRECACHE_MODEL( "models/collar_test.mdl" );
	PRECACHE_SOUND( "weapons/electro4.wav" );
	PRECACHE_SOUND( "debris/zap4.wav" );
}

void	CItemSlaveCollar::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( useType == USE_TOGGLE )
		m_bIsOn = !m_bIsOn;
	if ( useType == USE_ON )
		m_bIsOn = true;
	if ( useType == USE_OFF )
		m_bIsOn = false;

	if ( m_bIsOn )
		SetThink( &CItemSlaveCollar::ZapThink );
	else
		SetThink( &CItemSlaveCollar::OffThink );
	pev->nextthink = gpGlobals->time + 0.01;
}

void    CItemSlaveCollar::OffThink( void )
{
	for ( m_iBeams = 0; m_iBeams < 2; m_iBeams++ )
	{
		m_pBeam[m_iBeams]->pev->effects |= EF_NODRAW;
	}
}

void	CItemSlaveCollar::ZapThink( void )
{
	// create alien slave beam here
	//ALERT( at_console, "ZapThink!\n" );

	TraceResult tr;
	UTIL_TraceLine( pev->origin, m_vecEnd, ignore_monsters, ENT( pev ), &tr ); // dont_ignore_monsters
	float	m_flBeamLength = tr.flFraction;

	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "debris/zap4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );

	Vector vecTmpEnd = pev->origin + m_vecDir * 2048 * m_flBeamLength;
	tr.vecEndPos.z += 50;

	UTIL_Sparks( tr.vecEndPos );
	//if ( !tr.pHit )
	DecalGunshot( &tr, BULLET_PLAYER_CROWBAR );
	//UTIL_DecalTrace( &tr, DECAL_BIGSHOT1 + RANDOM_LONG(0,4) );

	for ( m_iBeams = 0; m_iBeams < 2; m_iBeams++ )
	{
		m_pBeam[m_iBeams]->pev->effects &= ~EF_NODRAW;
		m_pBeam[m_iBeams]->PointEntInit( vecTmpEnd, entindex() );
		m_pBeam[m_iBeams]->SetEndAttachment( m_iBeams + 1 );
		m_pBeam[m_iBeams]->SetStartPos( tr.vecEndPos );
		m_pBeam[m_iBeams]->SetColor( 180, 255, 96 );
		m_pBeam[m_iBeams]->SetBrightness( 255 );
		m_pBeam[m_iBeams]->SetNoise( 20 );

		/*
			pEntity = CBaseEntity::Instance(tr.pHit);
			if (pEntity != NULL && pEntity->pev->takedamage)
			{
				pEntity->TraceAttack( pev, gSkillData.slaveDmgZap, vecAim, &tr, DMG_SHOCK );
			}
		*/
	}
	UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
	pev->nextthink = gpGlobals->time + 5;
}

//
// Decay's focus emitter code below
//

//
//	Decay's item_focusemitter for mission ht12fubar (Gamma labs)
//

class CFocusEmitter : public CActAnimating
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	EXPORT Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void    EXPORT EmitterThink( void );
	void	EXPORT DyingThink( void );
	void	KeyValue(KeyValueData *pkvd);
	void    LookAt( Vector inputangles );
	int  TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t *pevAttacker, int iGib );

	bool	m_bIsOn;

	Vector	m_vecDir;
	Vector  m_vecEnd;
	Vector  m_angGun;

	int	m_iszDeployedTarget;
	int	m_iszDeathTarget;
	int m_iszLaserTarget;
	CBeam	*bWhiteBeam;

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
};

LINK_ENTITY_TO_CLASS( item_focusemitter, CFocusEmitter );

TYPEDESCRIPTION	CFocusEmitter::m_SaveData[] =
{
	DEFINE_FIELD( CFocusEmitter, m_bIsOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFocusEmitter, m_vecDir, FIELD_VECTOR),
	DEFINE_FIELD( CFocusEmitter, m_vecEnd, FIELD_VECTOR),
	DEFINE_FIELD( CFocusEmitter, m_iszDeployedTarget, FIELD_INTEGER),
	DEFINE_FIELD( CFocusEmitter, m_iszDeathTarget, FIELD_INTEGER),
	DEFINE_FIELD( CFocusEmitter, m_iszLaserTarget, FIELD_INTEGER),
	DEFINE_FIELD( CFocusEmitter, bWhiteBeam, FIELD_CLASSPTR ),
};
IMPLEMENT_SAVERESTORE( CFocusEmitter, CBaseEntity );

void	CFocusEmitter::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "deploy_target"))
	{
		     m_iszDeployedTarget = ALLOC_STRING(pkvd->szValue);
			 pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "death_target"))
	{
		     m_iszDeathTarget = ALLOC_STRING(pkvd->szValue);
			 pkvd->fHandled = TRUE;
	}
	if (FStrEq(pkvd->szKeyName, "lasertarget"))
	{
		     m_iszLaserTarget = ALLOC_STRING(pkvd->szValue);
			 pkvd->fHandled = TRUE;
	}
	else
       CBaseEntity::KeyValue( pkvd );
}

void	CFocusEmitter::Spawn( void )
{
    Precache();
	SET_MODEL(ENT(pev), "models/focus_emitter.mdl");

	pev->takedamage		= DAMAGE_YES;
	pev->health			= 150*4; // 4 HVR shots
	pev->solid			= SOLID_BBOX;
	pev->flags		   |= FL_MONSTER;
	UTIL_SetSize( pev, Vector(-30,-30,0), Vector(30,30,700));

	SetBodygroup( 1, 2 );

	SetSequence( seqEmitterClosed );
	SetThink( &CFocusEmitter::EmitterThink );
	pev->nextthink = gpGlobals->time;

	CBaseEntity *LasTarget;
	LasTarget = UTIL_FindEntityByTargetname( NULL, STRING( m_iszLaserTarget ));
	if (!LasTarget)
		LasTarget = this;
	bWhiteBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
	bWhiteBeam->PointEntInit( LasTarget->pev->origin, entindex( )  );
	bWhiteBeam->SetEndAttachment( 2 );
	bWhiteBeam->SetColor( 255, 255, 255 );
	bWhiteBeam->SetScrollRate( 35 );
	bWhiteBeam->SetNoise( 3 );
	bWhiteBeam->pev->effects |= EF_NODRAW;

	if (FBitSet(pev->spawnflags, 1)) // Start on
	{
		m_bIsOn = true;
		SetSequence( seqEmitterIdleOpen );
		bWhiteBeam->pev->effects &= ~EF_NODRAW;
		pev->nextthink = gpGlobals->time;
	}
}

void	CFocusEmitter::Precache( void )
{
	PRECACHE_MODEL( "models/focus_emitter.mdl" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_SOUND( "debris/beamstart4.wav" );
}

void	CFocusEmitter::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if (m_bIsOn)
		return;

    SetSequence( seqEmitterDeploy );
	m_bIsOn = true;
}

void	CFocusEmitter::EmitterThink( void )
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	//CBaseEntity *pPlayer;
	//pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
	//if (pPlayer)
	//	LookAt( pPlayer->pev->origin );

	switch( GetSequence() )
	{
	case seqEmitterClosed:	 // 0 - still
		break;
	case seqEmitterDeploy:	 // 1 - slosh
		if ( m_fSequenceFinished )
		{
			SetSequence( seqEmitterIdleOpen );
			FireTargets( STRING( m_iszDeployedTarget ), this, this, USE_ON, 1.0 );
			bWhiteBeam->pev->effects &= ~EF_NODRAW;
			UTIL_EmitAmbientSound( ENT(pev), pev->origin, "debris/beamstart4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ) );
			UTIL_ScreenFadeAll( Vector( 255, 255, 255), 1.0, 0.1, 150, 0 );
		}
		break;
	case seqEmitterIdleOpen:
		if (pev->health < 150*3)
			SetSequence( seqEmitterBroken1);
		break;
	case seqEmitterBroken1:
		if (pev->health < 150*2)
			SetSequence( seqEmitterBroken2);
		break;
	case seqEmitterBroken2: // 2 - to rest
		//if (pev->health = 0)
		//	SetSequence( seqEmitterDeath );
		break;
	case seqEmitterDeath:
		// random explosions
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_EXPLOSION);		// This just makes a dynamic light now
			WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ));
			WRITE_COORD( pev->origin.z + RANDOM_FLOAT( -150, -50 ));
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( RANDOM_LONG(0,29) + 30  ); // scale * 10
			WRITE_BYTE( 12  ); // framerate
			WRITE_BYTE( TE_EXPLFLAG_NONE );
		MESSAGE_END();
		if ( m_fSequenceFinished )
		{
			SetThink( &CFocusEmitter::DyingThink );
			pev->nextthink = gpGlobals->time + 0.1;
		}
		break;
	default:
		break;
	}
}

void	CFocusEmitter::DyingThink( void )
{
	// lots of smoke
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SMOKE );
		WRITE_COORD( pev->origin.x + RANDOM_FLOAT( -150, 150 ) );
		WRITE_COORD( pev->origin.y + RANDOM_FLOAT( -150, 150 ) );
		WRITE_COORD( pev->origin.z + 200 + RANDOM_FLOAT( -150, -50 ) );
		WRITE_SHORT( g_sModelIndexSmoke );
		WRITE_BYTE( 50 );  // scale * 10
		WRITE_BYTE( 10 );  // framerate
	MESSAGE_END();

	pev->nextthink = gpGlobals->time + 0.1;
}

void CFocusEmitter :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	bWhiteBeam->pev->effects |= EF_NODRAW;
	FireTargets( STRING( m_iszDeathTarget ), this, this, USE_TOGGLE, 1.0 );
	SetSequence( seqEmitterDeath );
	pev->nextthink = gpGlobals->time + 0.1;

	//m_flDieCounter = gpGlobals->time + 2.5;

	//FireTargets( STRING(m_iszDeathTarget), this, this, USE_TOGGLE, 1.0 );
}

void	CFocusEmitter::LookAt( Vector inputangles )
{
	UTIL_MakeAimVectors( pev->angles );

	Vector posGun, angGun;
	GetAttachment( 2, posGun, angGun );

	Vector vecTarget = (inputangles - posGun).Normalize( );

	Vector vecOut;

	vecOut.x = DotProduct( gpGlobals->v_forward, vecTarget );
	vecOut.y = -DotProduct( gpGlobals->v_right, vecTarget );
	vecOut.z = DotProduct( gpGlobals->v_up, vecTarget );

	Vector angles = UTIL_VecToAngles (vecOut);

	angles.x = -angles.x;
	if (angles.x > 180)
		angles.x = angles.x - 360;
	if (angles.x < -180)
		angles.x = angles.x + 360;

	m_angGun.x = angles.x;
	m_angGun.y = angles.y;
/*
	if (angles.x > m_angGun.x)
		m_angGun.x = min( angles.x, m_angGun.x + 12 );
	if (angles.x < m_angGun.x)
		m_angGun.x = max( angles.x, m_angGun.x - 12 );
	if (angles.y > m_angGun.y)
		m_angGun.y = min( angles.y, m_angGun.y + 12 );
	if (angles.y < m_angGun.y)
		m_angGun.y = max( angles.y, m_angGun.y - 12 );
*/
	m_angGun.y = SetBoneController( 0, m_angGun.y );
	m_angGun.x = SetBoneController( 1, m_angGun.x );
}

int CFocusEmitter :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if (pevInflictor->owner == edict())
		return 0;

	if (bitsDamageType & DMG_BLAST)
	{
		flDamage *= 2;
	}

	// ALERT( at_console, "%.0f\n", flDamage );
	return CBaseEntity::TakeDamage(  pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//
// Emitter target
//

class CEmitterTarget : public CBaseEntity
{
public:
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
};

void CEmitterTarget :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// TODO: find emitter, call LookAt(this)
	CFocusEmitter *pEmitter; // = NULL;
	pEmitter = (CFocusEmitter*)UTIL_FindEntityByClassname( NULL, "item_focusemitter" );
	if (pEmitter)
		pEmitter->LookAt( pev->origin );

	FireTargets( STRING(pev->target), this, this, USE_TOGGLE, 0.0 );
	ALERT( at_console, "info_emittertarget called Use! Firing %s\n", STRING(pev->target) );
}
LINK_ENTITY_TO_CLASS( info_emittertarget, CEmitterTarget );
