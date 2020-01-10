//=========================================================
// Opposing Forces Monster Shocktrooper\Shockrifle blast
//
// Made by Demiurge
//
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"shock.h"
#include	"gamerules.h"
#include	"customentity.h"
#include	"decals.h"

LINK_ENTITY_TO_CLASS( shock, CShock );

CShock *CShock::ShockCreate( void )
{
	// Create a new entity with CShock private data
	CShock *pShock = GetClassPtr( (CShock *)NULL );
	pShock->pev->classname = MAKE_STRING("shock");
	pShock->Spawn();

	return pShock;
}

void CShock::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags		|= FL_MONSTER;
	pev->health		= 1;// weak!

	SET_MODEL(ENT(pev), "models/shock_effect.mdl");

//	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	BlastOn();
	
	SetTouch( &CShock::ShockTouch );

	if ( !FNullEnt(pev->owner) && (pev->owner->v.flags & FL_CLIENT) )
	{
		if ( g_pGameRules->IsMultiplayer() )
		{
		pev->dmg = gSkillData.plrDmgShockm;
		}
		else
		{
		pev->dmg = gSkillData.plrDmgShocks;
		}
	}
	else
	{
		// no real owner, or owner isn't a client. 
		pev->dmg = gSkillData.monDmgShock;
	}
	UTIL_MakeAimVectors( pev->angles );

	m_vecForward = gpGlobals->v_forward;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.1f;
}


void CShock::Precache( )
{
	PRECACHE_MODEL("models/shock_effect.mdl");

	PRECACHE_SOUND( "weapons/shock_fire.wav" );
	PRECACHE_SOUND( "weapons/shock_impact.wav" );

	PRECACHE_MODEL( "sprites/plasma.spr" );
	PRECACHE_MODEL( "sprites/glow02.spr" );
}

int	CShock :: Classify ( void )
{
	if ( pev->owner && pev->owner->v.flags & FL_CLIENT)
	{
		return CLASS_PLAYER_BIOWEAPON;
	}

	return	CLASS_ALIEN_BIOWEAPON;
}

void CShock::ShockTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

			pOther->TraceAttack(pevOwner, gSkillData.monDmgShock, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 

		ApplyMultiDamage( pev, pevOwner );

//		pev->velocity = Vector( 0, 0, 0 );
		// play body "thwack" sound
			EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", 1, ATTN_NORM);

	}

			UTIL_Sparks( pev->origin );
			ExplodeThink();
}

void CShock::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	BlastOff();
	iScale = 10;

	EMIT_SOUND(ENT(pev), CHAN_BODY, "weapons/shock_impact.wav", 1, ATTN_NORM);

	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE(TE_DLIGHT);
		WRITE_COORD(pev->origin.x);	// X
		WRITE_COORD(pev->origin.y);	// Y
		WRITE_COORD(pev->origin.z);	// Z
		WRITE_BYTE( 108 );		// radius * 0.1
		WRITE_BYTE( 201 );		// r
		WRITE_BYTE( 236 );		// g
		WRITE_BYTE( 255 );		// b
		WRITE_BYTE( 1 );		// time * 10
		WRITE_BYTE( 0 );		// decay * 0.1
	MESSAGE_END( );


	entvars_t *pevOwner;

	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 16, CLASS_ALIEN_BIOWEAPON, DMG_BLAST | DMG_ALWAYSGIB );

		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2));

	UTIL_Remove(this);
}

void CShock::BlastOff ( void )
{
//	UTIL_Remove( m_pBeam );
//	m_pBeam = NULL;
//	if ( m_pBeam )
//	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
//	}
//	if ( m_pNoise )
//	{
		UTIL_Remove( m_pNoise );
		m_pNoise = NULL;
//	}
/*	if ( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}*/

}

void CShock::BlastOn ( void )
{
	Vector		posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	
			m_pBeam = CBeam::BeamCreate( "sprites/plasma.spr", 30 );
	 
			GetAttachment( 1, posGun, angleGun );
			GetAttachment( 2, posGun, angleGun );

			Vector vecEnd = (gpGlobals->v_forward * 40) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pBeam->EntsInit( entindex(), entindex() );
			m_pBeam->SetStartAttachment( 1 );
			m_pBeam->SetEndAttachment( 2 );
			m_pBeam->SetBrightness( 190 );
			m_pBeam->SetScrollRate( 20 );
			m_pBeam->SetNoise( 20 );
			m_pBeam->DamageDecal( 1 );
			m_pBeam->SetFlags( BEAM_FSHADEOUT );
			m_pBeam->SetColor( 35, 214, 177 );


			m_pNoise = CBeam::BeamCreate( "sprites/plasma.spr", 30 );

			GetAttachment( 1, posGun, angleGun );
			GetAttachment( 2, posGun, angleGun );

			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pNoise->EntsInit( entindex(), entindex() );
			m_pNoise->SetStartAttachment( 1 );
			m_pNoise->SetEndAttachment( 2 );
			m_pNoise->SetBrightness( 190 );
			m_pNoise->SetScrollRate( 20 );
			m_pNoise->SetNoise( 65 );
			m_pNoise->DamageDecal( 1 );
			m_pNoise->SetFlags( BEAM_FSHADEOUT );
			m_pNoise->SetColor( 255, 255, 173 );


/*			m_pSprite = CSprite::SpriteCreate( "sprites/glow02.spr", pev->origin, FALSE );
			m_pSprite->pev->scale = 1.0;
			m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
			m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
			m_pSprite->pev->flags |= FL_SKIPLOCALHOST;
//			m_pSprite->pev->owner = m_pPlayer->edict();*/

			//EXPORT RelinkBeam();
}

void CShock::UpdateOnRemove()
{
	CBaseEntity::UpdateOnRemove();

	BlastOff();
}
