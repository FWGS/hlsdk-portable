#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "gamerules.h"

extern int gEvilImpulse101;

TYPEDESCRIPTION CBasePlayerAmmo::m_SaveData[] =
{
	DEFINE_FIELD( CBasePlayerAmmo, m_numclips, FIELD_INTEGER )
};

int CBasePlayerAmmo::Save( CSave& save )
{
	if ( !CBaseEntity::Save( save ) )
		return 0;
	return save.WriteFields( "CBasePlayerAmmo", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
}

int CBasePlayerAmmo::Restore( CRestore& restore )
{
	if ( !CBaseEntity::Restore( restore ) )
		return 0;
	int result = restore.ReadFields( "CBasePlayerAmmo", this, m_SaveData, ARRAYSIZE( m_SaveData ) );
	if ( CVAR_GET_FLOAT( "lightmultiplier" ) >= 0.1f )
	{
		pev->renderfx = kRenderFxLightMultiplier;
		pev->iuser4 = (int)CVAR_GET_FLOAT( "lightmultiplier" );
	}
	return result;
}

void CBasePlayerAmmo::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "numclips" ) )
	{
		m_numclips = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		data->fHandled = FALSE;
}

void CBasePlayerAmmo::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch( &CBasePlayerAmmo::DefaultTouch );
	if ( CVAR_GET_FLOAT( "lightmultiplier" ) >= 0.1f )
	{
		pev->renderfx = kRenderFxLightMultiplier;
		pev->iuser4 = (int)CVAR_GET_FLOAT( "lightmultiplier" );
	}
}

void CBasePlayerAmmo::DefaultTouch( CBaseEntity* other )
{
	if ( !other->IsPlayer() )
		return;
	if ( AddAmmo( other ) )
	{
		if ( g_pGameRules->AmmoShouldRespawn( this ) == GR_AMMO_RESPAWN_YES )
		{
			Respawn();
			return;
		}
	}
	else if ( !gEvilImpulse101 )
		return;
	SetTouch( NULL );
	SetThink( &CBasePlayerAmmo::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1;
}

CBaseEntity* CBasePlayerAmmo::Respawn()
{
	pev->effects |= EF_NODRAW;
	SetTouch( NULL );
	UTIL_SetOrigin( pev, g_pGameRules->VecAmmoRespawnSpot( this ) );
	SetThink( &CBasePlayerAmmo::Materialize );
	pev->nextthink = g_pGameRules->FlAmmoRespawnTime( this );
	return this;
}

void CBasePlayerAmmo::Materialize()
{
	if ( pev->effects & EF_NODRAW )
	{
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}
	SetTouch( &CBasePlayerAmmo::DefaultTouch );
}

class C9MMAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_9mmclip.mdl" : "models/w_9mmclip_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_9mmclip.mdl" : "models/w_9mmclip_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 30 * m_numclips, "9mm", 120 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_9mm, C9MMAmmo );

class CBuckShotAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_shotbox.mdl" : "models/w_shotbox_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_shotbox.mdl" : "models/w_shotbox_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 8 * m_numclips, "buckshot", 32 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_buckshot, CBuckShotAmmo );

class C556NatoAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_556nato.mdl" : "models/w_556nato_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_556nato.mdl" : "models/w_556nato_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 30 * m_numclips, "556Nato", 90 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_556nato, C556NatoAmmo );

class C556NatoBoxAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_556natobox.mdl" : "models/w_556natobox_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_556natobox.mdl" : "models/w_556natobox_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 60 * m_numclips, "556NatoBox", 200 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_556natobox, C556NatoBoxAmmo );

class C762NatoAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_762nato.mdl" : "models/w_762nato_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_762nato.mdl" : "models/w_762nato_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 30 * m_numclips, "762Nato", 90 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_762nato, C762NatoAmmo );

class C762NatoBoxAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_762natobox.mdl" : "models/w_762natobox_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_762natobox.mdl" : "models/w_762natobox_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 60 * m_numclips, "762NatoBox", 200 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_762natobox, C762NatoBoxAmmo );

class C45ACPAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_45acp.mdl" : "models/w_45acp_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_45acp.mdl" : "models/w_45acp_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 12 * m_numclips, "45acp", 100 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_45acp, C45ACPAmmo );

class C50AEAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_50ae.mdl" : "models/w_50ae_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_50ae.mdl" : "models/w_50ae_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 7 * m_numclips, "50AE", 35 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_50ae, C50AEAmmo );

class C338MagnumAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_338magnum.mdl" : "models/w_338magnum_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_338magnum.mdl" : "models/w_338magnum_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 10 * m_numclips, "338Magnum", 30 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_338magnum, C338MagnumAmmo );

class C57MMAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_57mm.mdl" : "models/w_57mm_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_57mm.mdl" : "models/w_57mm_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 50, "57mm", 100 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_57mm, C57MMAmmo );

class C66MMAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_66mm.mdl" : "models/w_66mm_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_66mm.mdl" : "models/w_66mm_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 1, "66mm", 1 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_66mm, C66MMAmmo );

class C357SIGAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), m_numclips == 1 ? "models/w_357sig.mdl" : "models/w_357sig_big.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( m_numclips == 1 ? "models/w_357sig.mdl" : "models/w_357sig_big.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 13, "357SIG", 52 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_357sig, C357SIGAmmo );

class CLawsAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), "models/w_lawsammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( "models/w_lawsammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 1, "66mm", 5 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_lawsclip, CLawsAmmo );

class CBombAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 5, "bomb", 5 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_bomb, CBombAmmo );

class CSyringeAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), "models/w_9mmclip.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( "models/w_9mmclip.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		if ( other->GiveAmmo( 3, "syringe", 3 ) == -1 )
			return FALSE;
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_syringe, CSyringeAmmo );

class CGenericAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), "models/w_weaponbox.mdl" );
		CBasePlayerAmmo::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( "models/w_weaponbox.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}

	BOOL AddAmmo( CBaseEntity* other ) override
	{
		other->GiveAmmo( 90, "762Nato", 90 );
		other->GiveAmmo( 90, "556Nato", 90 );
		other->GiveAmmo( 30, "338Magnum", 30 );
		other->GiveAmmo( 35, "50AE", 35 );
		other->GiveAmmo( 120, "9mm", 120 );
		other->GiveAmmo( 100, "57mm", 100 );
		other->GiveAmmo( 200, "762NatoBox", 200 );
		other->GiveAmmo( 200, "556NatoBox", 200 );
		other->GiveAmmo( 32, "buckshot", 32 );
		other->GiveAmmo( 100, "45acp", 100 );
		other->GiveAmmo( 52, "357SIG", 52 );
		EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_generic, CGenericAmmo );
