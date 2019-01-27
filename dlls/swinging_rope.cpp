
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "customentity.h"
#include "customweapons.h"
#include "unpredictedweapon.h"

class CPiton;

class RopeGun : public CBasePlayerWeaponU
{
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 1; }
	int GetItemInfo(ItemInfo *p);

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	int AddToPlayer(CBasePlayer *pPlayer);
	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void SwingThink( void );
	unsigned short m_usCrossbow;
	CBeam* m_pBeam1;
#ifndef CLIENT_DLL
	CPiton *lastPiton;
#endif
public:
	void Clean();
};
LINK_ENTITY_TO_CLASS(weapon_rope, RopeGun)

#ifndef CLIENT_DLL
#define PITON_AIR_VELOCITY	2000

class CPiton : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	void EXPORT PitonTouch( CBaseEntity *pOther );
	
	RopeGun *gun;
public:
	static CPiton *PitonCreate( RopeGun *creator );

	float ropeLength;
};
LINK_ENTITY_TO_CLASS( rope_piton, CPiton )

CPiton *CPiton::PitonCreate( RopeGun *creator )
{
	CPiton *pPiton = GetClassPtr( (CPiton *)NULL );
	pPiton->pev->classname = MAKE_STRING( "rope_piton" );
	pPiton->gun = creator;
	pPiton->Spawn();

	return pPiton;
}

void CPiton::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	pev->gravity = 0.5;

	SET_MODEL( ENT( pev ), "models/crossbow_bolt.mdl" );

	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	SetTouch( &CPiton::PitonTouch );
	pev->nextthink = gpGlobals->time + 0.2;

	ropeLength = 0;
}

void CPiton::Precache()
{
	PRECACHE_MODEL( "models/crossbow_bolt.mdl" );
	PRECACHE_SOUND( "weapons/xbow_hit1.wav" );
	//m_iTrail = PRECACHE_MODEL( "sprites/streak.spr" );
}

int CPiton::Classify( void )
{
	return CLASS_NONE;
}

void CPiton::PitonTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if( pOther->pev->takedamage )
	{
		gun->Clean();
		UTIL_Remove( this );
	}
	else
	{
		if( UTIL_PointContents( pev->origin ) == CONTENTS_SKY){
			gun->Clean();
			UTIL_Remove( this );
			return;
		}

		EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "weapons/xbow_hit1.wav", RANDOM_FLOAT( 0.95, 1.0 ), ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 7 ) );

		//SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;// this will get changed below if the bolt is allowed to stick in what it hit.

		if( FClassnameIs( pOther->pev, "worldspawn" ) )
		{
			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			//pev->solid = SOLID_NOT;
			pev->movetype = MOVETYPE_FLY;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 360 );
			//pev->nextthink = gpGlobals->time + 60.0;

			ropeLength = (pev->origin - pev->owner->v.origin).Length();
		}
		else if( pOther->pev->movetype == MOVETYPE_PUSH || pOther->pev->movetype == MOVETYPE_PUSHSTEP )
		{
			Vector vecDir = pev->velocity.Normalize();
			UTIL_SetOrigin( pev, pev->origin - vecDir * 12 );
			pev->angles = UTIL_VecToAngles( vecDir );
			pev->solid = SOLID_NOT;
			pev->velocity = Vector( 0, 0, 0 );
			pev->avelocity.z = 0;
			pev->angles.z = RANDOM_LONG( 0, 360 );
			pev->nextthink = gpGlobals->time + 60.0;

			if (gPhysicsInterfaceInitialized) {
				// g-cont. Setup movewith feature
				pev->movetype = MOVETYPE_COMPOUND;	// set movewith type
				pev->aiment = ENT( pOther->pev );	// set parent
			}

			ropeLength = (pev->origin - pev->owner->v.origin).Length();
		}

		if( UTIL_PointContents( pev->origin ) != CONTENTS_WATER )
		{
			UTIL_Sparks( pev->origin );
		}
	}
}
#endif

enum crossbow_e
{
	CROSSBOW_IDLE1 = 0,	// full
	CROSSBOW_IDLE2,		// empty
	CROSSBOW_FIDGET1,	// full
	CROSSBOW_FIDGET2,	// empty
	CROSSBOW_FIRE1,		// full
	CROSSBOW_FIRE2,		// reload
	CROSSBOW_FIRE3,		// empty
	CROSSBOW_RELOAD,	// from empty
	CROSSBOW_DRAW1,		// full
	CROSSBOW_DRAW2,		// empty
	CROSSBOW_HOLSTER1,	// full
	CROSSBOW_HOLSTER2	// empty
};



void RopeGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_rope");
	Precache( );
	SET_MODEL( ENT( pev ), "models/w_crossbow.mdl" );
	m_iId = WEAPON_ROPE;

	m_iClip = -1;
	m_iDefaultAmmo = -1;

	FallInit();// get ready to fall down.
#ifndef CLIENT_DLL
	lastPiton = NULL;
#endif
}

void RopeGun::Precache(void)
{
	PRECACHE_MODEL( "models/w_crossbow.mdl" );
	PRECACHE_MODEL( "models/v_crossbow.mdl" );
	PRECACHE_MODEL( "models/p_crossbow.mdl" );

	UTIL_PrecacheOther( "rope_piton" );

	PRECACHE_MODEL("sprites/xbeam1.spr");

	//m_usCrossbow = PRECACHE_EVENT( 1, "events/crossbow1.sc" );
	PRECACHE_GENERIC("sprites/weapon_rope.txt");
}

int RopeGun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_ROPE;
	p->iFlags = 0;
	p->iWeight = 40;
	return 1;
}

int RopeGun::AddToPlayer(CBasePlayer *pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL RopeGun::Deploy()
{
	return DefaultDeploy( "models/v_crossbow.mdl", "models/p_crossbow.mdl", CROSSBOW_DRAW1, "bow" );
}

void RopeGun::Holster( int skiplocal /* = 0 */ )
{
	Clean();
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.2;
}

void RopeGun::PrimaryAttack(void)
{
#ifndef CLIENT_DLL
	if(lastPiton){
		Clean();
		m_flNextPrimaryAttack = gpGlobals->time + 0.1;
		return;
	}

	Vector anglesAim = m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle;
	UTIL_MakeVectors( anglesAim );
	anglesAim.x	= -anglesAim.x;
	
	//
	
	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	//Vector vecSrc	= m_pPlayer->GetGunPosition() - gpGlobals->v_up * 2;
	Vector vecDir	= gpGlobals->v_forward;

	CPiton *pPiton = CPiton::PitonCreate(this);
	pPiton->pev->origin = vecSrc;
	pPiton->pev->angles = anglesAim;
	pPiton->pev->owner = m_pPlayer->edict();
	pPiton->pev->velocity = vecDir * PITON_AIR_VELOCITY;
	pPiton->pev->speed = PITON_AIR_VELOCITY;
	pPiton->pev->avelocity.z = 10;
	
	lastPiton = pPiton;
	SetThink( &RopeGun::SwingThink );
	pev->nextthink = gpGlobals->time + 0.05;

	if(!m_pBeam1){
		m_pBeam1 = CBeam::BeamCreate("sprites/xbeam1.spr", 8);
		//m_pBeam1->PointEntInit(pPiton->pev->origin, m_pPlayer->entindex());
		m_pBeam1->EntsInit(pPiton->entindex(),m_pPlayer->entindex());
		m_pBeam1->SetFlags(BEAM_FSINE);
		m_pBeam1->pev->spawnflags |= SF_BEAM_TEMPORARY;
		m_pBeam1->pev->owner = m_pPlayer->edict();
		//m_pBeam1->SetEndAttachment(1);
	}
#endif

	m_flNextPrimaryAttack = gpGlobals->time + 0.2;
}

void RopeGun::SecondaryAttack(void)
{
	Clean();
}

void RopeGun::Clean()
{
#ifndef CLIENT_DLL
	if( m_pBeam1 ){
		UTIL_Remove(m_pBeam1);
		m_pBeam1 = NULL;
	}
	if(lastPiton){
		UTIL_Remove(lastPiton);
		lastPiton = NULL;
	}
	SetThink(NULL);
#endif
}

void RopeGun::SwingThink()
{
#ifndef CLIENT_DLL
	if(lastPiton->ropeLength<0.5){
		pev->nextthink = gpGlobals->time + 0.05;
		return;
	}

	//m_pPlayer->pev->velocity = m_pPlayer->pev->velocity - gpGlobals->v_forward * 25 * 5;

	Vector force(0,0,0);

	Vector tension = lastPiton->pev->origin - m_pPlayer->pev->origin;
	float stretch = tension.Length();

	if(lastPiton->ropeLength < stretch){
		Vector tension_norm = tension.Normalize();
		Vector user_vel = m_pPlayer->pev->velocity;
		float tension_vel_mag = DotProduct(tension_norm,user_vel);

		if(tension_vel_mag < 0){
			force = tension_norm * ((tension_vel_mag*-1.25)+(stretch-lastPiton->ropeLength));
		}else{
			force = tension_norm * (stretch-lastPiton->ropeLength);
		}
	}

	m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + force;
	pev->nextthink = gpGlobals->time + 0.05;
#endif
}
