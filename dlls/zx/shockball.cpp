//=========================================================
// Opposing Forces Monster Voltigore blast
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
#include	"shockball.h"
#include	"gamerules.h"
#include	"customentity.h"
#include	"decals.h"
#include	"effects.h"

LINK_ENTITY_TO_CLASS( shockball, CShockball );

CShockball *CShockball::ShockballCreate( void )
{
	// Create a new entity with CShockball private data
	CShockball *pShockball = GetClassPtr( (CShockball *)NULL );
	pShockball->pev->classname = MAKE_STRING("shockball");
	pShockball->Spawn();

	return pShockball;
}

void CShockball::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags		|= FL_MONSTER;
	pev->health		= 1;// weak!

	SET_MODEL(ENT(pev), "models/sprite.mdl");

	UTIL_SetSize(pev, Vector(-0, -0, -0), Vector(0, 0, 0));
//	UTIL_SetOrigin( pev, pev->origin );
	BlastOn();

	UTIL_MakeAimVectors( pev->angles + m_vecEnemyLKP );
	m_vecForward = gpGlobals->v_forward;
	m_vecUp = gpGlobals->v_up;
	
	SetThink( &CShockball::FlyThink );
	SetTouch( &CShockball::ShockballTouch );
	pev->nextthink = gpGlobals->time + 0.1;
	ResetSequenceInfo( );
}


void CShockball::Precache( )
{
	PRECACHE_MODEL("models/sprite.mdl");
 	PRECACHE_MODEL("sprites/glow02.spr");
	m_iBlastText = PRECACHE_MODEL("sprites/plasma.spr");
}

int	CShockball :: Classify ( void )
{
	return	CLASS_ALIEN_BIOWEAPON;
}

void CShockball::FlyThink( void )
{
	StudioFrameAdvance( );
	BlastOn();
	pev->nextthink = gpGlobals->time + 0.1;
}

void CShockball::ShockballTouch( CBaseEntity *pOther )
{	
	if ( pOther->edict() == pev->owner || pOther->pev->modelindex == pev->modelindex )
	{// bumped into the guy that shot it.
		pev->solid = SOLID_NOT;
		UTIL_Remove(this);
		return;
	}

	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		// UNDONE: this needs to call TraceAttack instead
		ClearMultiDamage( );

			pOther->TraceAttack(pevOwner, gSkillData.voltigoreDmgBeam, pev->velocity.Normalize(), &tr, DMG_BLAST | DMG_NEVERGIB ); 

		ApplyMultiDamage( pev, pevOwner );

	}

			UTIL_Sparks( pev->origin );
			ExplodeThink();
}

void CShockball::ExplodeThink( void )
{
	int iContents = UTIL_PointContents ( pev->origin );
	int iScale;
	
	BlastOff();
	pev->dmg = 40;
	iScale = 10;


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

	::RadiusDamage( pev->origin, pev, pevOwner, pev->dmg, 32, CLASS_ALIEN_BIOWEAPON, DMG_BLAST | DMG_ALWAYSGIB );

		TraceResult tr;

		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_SCORCH1 + RANDOM_LONG(0,1));

	UTIL_Remove(this);
}

void CShockball::BlastOff ( void )
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}

}

void CShockball::BlastOn ( void )
{
	TraceResult tr;
	Vector vecDest;
	CBeam *pBeam;

	UTIL_MakeVectors( pev->angles );

	Vector vecEnd = (gpGlobals->v_forward * 40) + pev->origin;
			UTIL_TraceLine( pev->origin, vecEnd, dont_ignore_monsters, edict(), &tr );

		vecDest = 90 * (Vector(RANDOM_FLOAT(-1,1), RANDOM_FLOAT(-1,1), RANDOM_FLOAT(-1,1)).Normalize());
		UTIL_TraceLine( pev->origin, pev->origin + vecDest, ignore_monsters, NULL, &tr);
			// we hit something.

			pBeam = CBeam::BeamCreate("sprites/plasma.spr",200);
			pBeam->PointEntInit( tr.vecEndPos, entindex() );
			pBeam->SetStartPos( tr.vecEndPos );
			pBeam->SetEndEntity( entindex() );
			pBeam->SetColor( 255, 0, 255 );
			pBeam->SetNoise( 65 );
			pBeam->SetBrightness( 255 );
			pBeam->SetWidth( 30 );
			pBeam->SetScrollRate( 35 );
			pBeam->LiveForTime( 1 );

			pev->nextthink = gpGlobals->time + 0.1;


		if(!m_pSprite)
	{
			m_pSprite = CSprite::SpriteCreate( "sprites/glow02.spr", pev->origin, FALSE );
			m_pSprite->SetAttachment( edict(), 0 );
			m_pSprite->pev->scale = 0.5;
			m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 64, kRenderFxNoDissipation );
			m_pSprite->pev->spawnflags |= SF_SPRITE_TEMPORARY;
			m_pSprite->pev->flags |= FL_SKIPLOCALHOST;

	}

}
