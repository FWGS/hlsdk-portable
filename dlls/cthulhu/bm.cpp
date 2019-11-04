
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

//=========================================================
// monster template
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"decals.h"
#include	"weapons.h"
#include	"game.h"


#include "bm.h"

extern int gSpitSprite;

Vector VecCheckSplatToss( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float maxHeight )
{
	TraceResult		tr;
	Vector			vecMidPoint;// halfway point between Spot1 and Spot2
	Vector			vecApex;// highest point 
	Vector			vecScale;
	Vector			vecGrenadeVel;
	Vector			vecTemp;
	float			flGravity = g_psv_gravity->value;

	// calculate the midpoint and apex of the 'triangle'
	vecMidPoint = vecSpot1 + (vecSpot2 - vecSpot1) * 0.5;
	UTIL_TraceLine(vecMidPoint, vecMidPoint + Vector(0,0,maxHeight), ignore_monsters, ENT(pev), &tr);
	vecApex = tr.vecEndPos;

	UTIL_TraceLine(vecSpot1, vecApex, dont_ignore_monsters, ENT(pev), &tr);
	if (tr.flFraction != 1.0)
	{
		// fail!
		return g_vecZero;
	}

	// Don't worry about actually hitting the target, this won't hurt us!

	// How high should the grenade travel (subtract 15 so the grenade doesn't hit the ceiling)?
	float height = (vecApex.z - vecSpot1.z) - 15;
	// How fast does the grenade need to travel to reach that height given gravity?
	float speed = sqrt( 2 * flGravity * height );
	
	// How much time does it take to get there?
	float time = speed / flGravity;
	vecGrenadeVel = (vecSpot2 - vecSpot1);
	vecGrenadeVel.z = 0;
	float distance = vecGrenadeVel.Length();
	
	// Travel half the distance to the target in that time (apex is at the midpoint)
	vecGrenadeVel = vecGrenadeVel * ( 0.5 / time );
	// Speed to offset gravity at the desired height
	vecGrenadeVel.z = speed;

	return vecGrenadeVel;
}




// ---------------------------------
//
// Mortar
//
// ---------------------------------
void MortarSpray( const Vector &position, const Vector &direction, int spriteModel, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( position.x);	// pos
		WRITE_COORD( position.y);	
		WRITE_COORD( position.z);	
		WRITE_COORD( direction.x);	// dir
		WRITE_COORD( direction.y);	
		WRITE_COORD( direction.z);	
		WRITE_SHORT( spriteModel );	// model
		WRITE_BYTE ( count );			// count
		WRITE_BYTE ( 130 );			// speed
		WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
	MESSAGE_END();
}

// UNDONE: right now this is pretty much a copy of the squid spit with minor changes to the way it does damage
void CBMortar:: Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING( "bmortar" );
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/mommaspit.spr");
	pev->frame = 0;
	pev->scale = 0.5;

	UTIL_SetSize( pev, Vector( 0, 0, 0), Vector(0, 0, 0) );

	m_maxFrame = (float) MODEL_FRAMES( pev->modelindex ) - 1;
	pev->dmgtime = gpGlobals->time + 0.4;
}

void CBMortar::Animate( void )
{
	SetNextThink( 0.1 );

	if ( gpGlobals->time > pev->dmgtime )
	{
		pev->dmgtime = gpGlobals->time + 0.2;
		MortarSpray( pev->origin, -pev->velocity.Normalize(), gSpitSprite, 3 );
	}
	if ( pev->frame++ )
	{
		if ( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

CBMortar *CBMortar::Shoot( edict_t *pOwner, Vector vecStart, Vector vecVelocity )
{
	CBMortar *pSpit = GetClassPtr( (CBMortar *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = pOwner;
	pSpit->pev->scale = 2.5;
	pSpit->SetThink ( Animate );
	pSpit->SetNextThink( 0.1 );

	return pSpit;
}


void CBMortar::Touch( CBaseEntity *pOther )
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT( 90, 110 );

	EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );	

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	case 1:
		EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );	
		break;
	}

	if ( pOther->IsBSPModel() )
	{

		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace(&tr, DECAL_MOMMASPLAT);
	}
	else
	{
		tr.vecEndPos = pev->origin;
		tr.vecPlaneNormal = -1 * pev->velocity.Normalize();
	}
	// make some flecks
	MortarSpray( tr.vecEndPos, tr.vecPlaneNormal, gSpitSprite, 24 );

	entvars_t *pevOwner = NULL;
	if ( pev->owner )
		pevOwner = VARS(pev->owner);

	RadiusDamage( pev->origin, pev, pevOwner, gSkillData.bigmommaDmgBlast, gSkillData.bigmommaRadiusBlast, CLASS_NONE, DMG_ACID );
	UTIL_Remove( this );
}



#endif
