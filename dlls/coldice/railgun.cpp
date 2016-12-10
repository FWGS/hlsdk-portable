/*
	Copyright (c) 1999, Cold Ice Modification. 
	
	This code has been written by SlimShady ( darcuri@optonline.net )

    Use, distribution, and modification of this source code and/or resulting
    object code is restricted to non-commercial enhancements to products from
    Valve LLC.  All other use, distribution, or modification is prohibited
    without written permission from Valve LLC and from the Cold Ice team.

    Please if you use this code in any public form, please give us credit.

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "shake.h"
#include "gamerules.h"
#include "effects.h"
#include "customentity.h"


#define RAILGUN_PRIMARY_FIRE_VOLUME	450
#define RAIL_BEAM_SPRITE "sprites/xbeam1.spr"

enum railgun_e {
	GAUSS_IDLE = 0,
	GAUSS_IDLE2,
	GAUSS_FIDGET,
	GAUSS_SPINUP,
	GAUSS_SPIN,
	GAUSS_FIRE,
	GAUSS_FIRE2,
	GAUSS_HOLSTER,
	GAUSS_DRAW
};

class CRailgun : public CBasePlayerWeapon
{
public:

	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( void );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void WeaponIdle( void );

	void StartFire( void );
	void Fire( Vector vecOrigSrc, Vector vecDirShooting, float flDamage );
	int m_iBalls;
	int m_iGlow;
	int m_iBeam;

	// rail, rail, rail
	void CreateTrail(Vector,Vector);
	void EXPORT DestroyTrail(void);
	void EXPORT UpdateTrail(void);	
	int current_phase;
	CBeam* m_pBeam, *m_inBeam, *m_lBeam;

 
};
LINK_ENTITY_TO_CLASS( weapon_railgun, CRailgun );
LINK_ENTITY_TO_CLASS( weapon_gauss, CRailgun );

//=========================================================
//=========================================================

void CRailgun::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_railgun"); 
	Precache( );
	m_iId = WEAPON_RAILGUN;
	SET_MODEL(ENT(pev), "models/wmodels/w_railgun.mdl");

	m_iDefaultAmmo = RAILGUN_DEFAULT_GIVE;

	FallInit();
}


void CRailgun::Precache( void )
{
	PRECACHE_MODEL("models/wmodels/w_railgun.mdl");
	PRECACHE_MODEL("models/vmodels/v_railgun.mdl");
	PRECACHE_MODEL("models/pmodels/p_railgun.mdl");

	PRECACHE_SOUND("weapons/railgun1.wav");
	
	PRECACHE_MODEL( RAIL_BEAM_SPRITE );	
	m_iGlow = PRECACHE_MODEL ( "sprites/blueflare1.spr" );
	m_iBalls = PRECACHE_MODEL( "sprites/blueflare1.spr" );
	m_iBeam = PRECACHE_MODEL ( "sprites/smoke.spr" );
}

int CRailgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
		WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CRailgun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "slug";
	p->iMaxAmmo1 = RAILGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_RAILGUN;
	p->iFlags = 0;
	p->iWeight = RAILGUN_WEIGHT;

	return 1;
}


BOOL CRailgun::Deploy( )
{
	return DefaultDeploy( "models/vmodels/v_railgun.mdl", "models/pmodels/p_railgun.mdl", GAUSS_DRAW, "gauss" );
}


void CRailgun::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim( GAUSS_HOLSTER );
}
void CRailgun::PrimaryAttack()
{

	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 2)
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 2;

	StartFire();

	m_flTimeWeaponIdle = gpGlobals->time + 1.0;
	m_flNextPrimaryAttack = gpGlobals->time + 1.0; 
}
void CRailgun::SecondaryAttack()
{

}
void CRailgun::CreateTrail(Vector a,Vector b)
{
	current_phase = 0;
	m_pBeam = CBeam::BeamCreate( RAIL_BEAM_SPRITE, 40 );
	m_pBeam->PointsInit(a,b);
	m_pBeam->SetFlags( BEAM_FSINE );
	m_pBeam->SetColor( 0, 113, 230 );
	m_pBeam->LiveForTime( 1 );
	m_pBeam->SetScrollRate ( 10 );
	m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_inBeam = CBeam::BeamCreate( RAIL_BEAM_SPRITE, 20 );
	m_inBeam->PointsInit(a,b);
	m_inBeam->SetFlags( BEAM_FSINE );
	m_inBeam->SetColor( 200, 200, 200 );
	m_inBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_inBeam->LiveForTime( 1 );
	m_inBeam->SetScrollRate ( 10 );
	m_lBeam = CBeam::BeamCreate( RAIL_BEAM_SPRITE, 50 );
	m_lBeam->PointsInit(a,b);
	m_lBeam->SetFlags( BEAM_FSINE );
	m_lBeam->SetColor( 200, 200, 200 );
	m_lBeam->SetScrollRate ( 10 );
	m_lBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	m_lBeam->LiveForTime( 1 );
	UpdateTrail();
}
void CRailgun::UpdateTrail()
{
	m_pBeam->SetNoise( 10 + current_phase * 2 );
	m_pBeam->SetBrightness( 200 - current_phase * 20 );
	m_inBeam->SetNoise( 5 + current_phase );
	m_inBeam->SetBrightness( 200 - current_phase * 20 );
	m_lBeam->SetNoise( 0 );
	m_lBeam->SetBrightness( 200 );
	
	current_phase++;
	
	if(current_phase == 8) 
	{
		SetThink(DestroyTrail);
	}
	else 
	{
		SetThink(UpdateTrail);
	}

	pev->nextthink=gpGlobals->time + 0.1;
}
void CRailgun::DestroyTrail()
{
	current_phase = 0;
	
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_inBeam )
	{
		UTIL_Remove( m_inBeam );
		m_inBeam = NULL;
	}
	if ( m_lBeam )
	{
		UTIL_Remove( m_lBeam );
		m_inBeam = NULL;
	}
}

void CRailgun::StartFire( void )
{
	float flDamage;
	
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc = m_pPlayer->GetGunPosition( ); 
	
	flDamage = gSkillData.plrDmgRailgun;
	m_pPlayer->pev->punchangle.x = -2;
	
	float flZVel = m_pPlayer->pev->velocity.z;

	SendWeaponAnim( GAUSS_FIRE2 );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/railgun1.wav", 0.5 + flDamage * (1.0 / 400.0), ATTN_NORM, 0, 85 + RANDOM_LONG(0,0x1f)); 

	Fire( vecSrc, vecAiming, flDamage );
}
void CRailgun::Fire( Vector vecOrigSrc, Vector vecDir, float flDamage )
{
	m_pPlayer->m_iWeaponVolume = RAILGUN_PRIMARY_FIRE_VOLUME;

	Vector vecSrc = vecOrigSrc;
	Vector vecDest = vecSrc + vecDir * 8192;
	edict_t		*pentIgnore;
	TraceResult tr, beam_tr;

	pentIgnore = ENT( m_pPlayer->pev );

	UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);

	CreateTrail(vecSrc + gpGlobals->v_up * -8 + gpGlobals->v_right * 3 ,tr.vecEndPos);
		
	UTIL_TraceLine(vecSrc, vecDest, dont_ignore_monsters, pentIgnore, &tr);

	if (tr.fAllSolid)
		return;

	CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

	if (pEntity == NULL)
		return;

	if (pEntity->pev->takedamage)
	{
		ClearMultiDamage();
		pEntity->TraceAttack( m_pPlayer->pev, flDamage, vecDir, &tr, DMG_BULLET | DMG_ALWAYSGIB );
		ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);
	}
	else
	{
		// Make some balls and a decal
		DecalGunshot( &tr, BULLET_MONSTER_12MM );
		
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_GLOWSPRITE );
			WRITE_COORD( tr.vecEndPos.x);	// pos
			WRITE_COORD( tr.vecEndPos.y);
			WRITE_COORD( tr.vecEndPos.z);
			WRITE_SHORT( m_iGlow );		    // model
			WRITE_BYTE( 20 );				// life * 10
			WRITE_BYTE( 3 );				// size * 10
			WRITE_BYTE( 200 );			    // brightness
		MESSAGE_END();


		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos );
			WRITE_BYTE( TE_SPRITETRAIL );
			WRITE_COORD( tr.vecEndPos.x );
			WRITE_COORD( tr.vecEndPos.y );
			WRITE_COORD( tr.vecEndPos.z );
			WRITE_COORD( tr.vecEndPos.x + tr.vecPlaneNormal.x );
			WRITE_COORD( tr.vecEndPos.y + tr.vecPlaneNormal.y );
			WRITE_COORD( tr.vecEndPos.z + tr.vecPlaneNormal.z );
			WRITE_SHORT( m_iBalls );		    // model
			WRITE_BYTE( 8 );				    // count
			WRITE_BYTE( 6 );				    // life * 10
			WRITE_BYTE( RANDOM_LONG( 1, 2 ) );	// size * 10
			WRITE_BYTE( 10 );				    // amplitude * 0.1
			WRITE_BYTE( 20 );				    // speed * 100
		MESSAGE_END();	
			
	}

		vecSrc = tr.vecEndPos + vecDir;
		pentIgnore = ENT( pEntity->pev );
}


void CRailgun::WeaponIdle( void )
{
	ResetEmptySound( );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	int iAnim;
	float flRand = RANDOM_FLOAT(0, 1);
	if (flRand <= 0.5)
	{
		iAnim = GAUSS_IDLE;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
	else if (flRand <= 0.75)
	{
		iAnim = GAUSS_IDLE2;
		m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
	}
	else
	{
		iAnim = GAUSS_FIDGET;
		m_flTimeWeaponIdle = gpGlobals->time + 3;
	}

	return;
	SendWeaponAnim( iAnim );
		
	
}

class CRailgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_gaussammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_gaussammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_RAILSLUG_GIVE, "slug", RAILGUN_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_railslug, CRailgunAmmo );

