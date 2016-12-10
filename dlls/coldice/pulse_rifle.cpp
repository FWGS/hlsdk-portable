

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "effects.h"


class CBlasterBeam : public CBaseEntity
{
	void Spawn( void );
	void Precache( void );
	int  Classify ( void );
	void EXPORT BubbleThink( void );
	void EXPORT PulseTouch( CBaseEntity *pOther );

    CBeam *m_pBeam;
	int m_iGlow;
	int m_iTrail;
	int m_iBalls;
	Vector beamendpos;

public:
	static CBlasterBeam *Fire( void );
};
LINK_ENTITY_TO_CLASS( blaster_beam, CBlasterBeam );

CBlasterBeam *CBlasterBeam::Fire( void )
{
	CBlasterBeam *pPulse = GetClassPtr( (CBlasterBeam *)NULL );
	pPulse->pev->classname = MAKE_STRING("blaster_beam");
	pPulse->Spawn();
	return pPulse;
}

void CBlasterBeam::Spawn( )
{
	Precache( );
	pev->movetype = MOVETYPE_FLY;
	pev->solid    = SOLID_BBOX;
	pev->gravity = 0.3;

	SET_MODEL(ENT(pev), "sprites/blueflare2.spr");
	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );

	SetTouch( PulseTouch );
	SetThink( BubbleThink );
	pev->nextthink = gpGlobals->time + 0.1;
}


void CBlasterBeam::Precache( )
{
	PRECACHE_MODEL ("sprites/blueflare2.spr");

	PRECACHE_SOUND("weapons/xbow_hitbod1.wav");
	PRECACHE_SOUND("weapons/xbow_hitbod2.wav");
	PRECACHE_SOUND("weapons/xbow_fly1.wav");

	PRECACHE_SOUND("fvox/beep.wav");

	m_iGlow = PRECACHE_MODEL( "sprites/blueflare1.spr" );
	m_iBalls = PRECACHE_MODEL( "sprites/blueflare1.spr" );
	m_iTrail = PRECACHE_MODEL( "sprites/streak.spr" );
}


int	CBlasterBeam :: Classify ( void )
{
	return	CLASS_NONE;
}

void CBlasterBeam::PulseTouch( CBaseEntity *pOther )
{
	SetTouch( NULL );
	SetThink( NULL );

	if (pOther->pev->takedamage)
	{
		TraceResult tr = UTIL_GetGlobalTrace( );
		entvars_t	*pevOwner;

		pevOwner = VARS( pev->owner );

		ClearMultiDamage( );

		if ( pOther->IsPlayer() )
		{
			pOther->TraceAttack(pevOwner, 20, pev->velocity.Normalize(), &tr, DMG_BLAST|DMG_ALWAYSGIB); 
		}
		else
		{
			pOther->TraceAttack(pevOwner, 20, pev->velocity.Normalize(), &tr, DMG_BULLET | DMG_NEVERGIB ); 
		}

		ApplyMultiDamage( pev, pevOwner );

		pev->velocity = Vector( 0, 0, 0 );

		//EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/hblaster_hit.wav", 1, ATTN_NORM); 
		
		Killed( pev, GIB_NEVER );
	}
	else
	{
		//EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hblaster_hit.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

		SetThink( SUB_Remove );
		pev->nextthink = gpGlobals->time;

		if ( FClassnameIs( pOther->pev, "worldspawn" ) )	
		{
			
			MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_GLOWSPRITE );
			WRITE_COORD( pev->origin.x);					// pos
			WRITE_COORD( pev->origin.y);
			WRITE_COORD( pev->origin.z);
			WRITE_SHORT( m_iGlow );							// model
			WRITE_BYTE( 4 );								// life * 10
			WRITE_BYTE( RANDOM_LONG( 1, 4 ) );				// size * 10
			WRITE_BYTE( 150 );								// brightness
			MESSAGE_END();

			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
			WRITE_BYTE( TE_SPRITETRAIL );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( pev->origin.x );
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_SHORT( m_iBalls );				// model
			WRITE_BYTE( 4 );						// count
			WRITE_BYTE( .5 );						// life * 10
			WRITE_BYTE( RANDOM_LONG( 1, 3 ) );		// size * 10
			WRITE_BYTE( 10 );						// amplitude * 0.1
			WRITE_BYTE( 20 );						// speed * 100
			MESSAGE_END();	

		}

		Killed(pev,GIB_NEVER);			
	}

}

void CBlasterBeam::BubbleThink( void )
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->waterlevel == 0)
	{
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT(entindex());	// entity
		WRITE_SHORT( m_iTrail );	// model
		WRITE_BYTE( 3 );            // life
		WRITE_BYTE( 1 );			// width
		WRITE_BYTE( 0 );			// r, g, b
		WRITE_BYTE( 113 );			// r, g, b
		WRITE_BYTE( 230 );			// r, g, b
		WRITE_BYTE( 200 );			// brightness
		MESSAGE_END();
		return;
	}

	UTIL_BubbleTrail( pev->origin - pev->velocity * 0.1, pev->origin, 1 );
}


//=========================================================
//=========================================================

enum pulserifle_e {
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

class CPulseRifle : public CBasePlayerWeapon
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

 
};
LINK_ENTITY_TO_CLASS( weapon_pulserifle, CPulseRifle  );

//=========================================================
//=========================================================

void CPulseRifle::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_pulserifle"); 
	Precache( );
	m_iId = WEAPON_PULSERIFLE;
	SET_MODEL(ENT(pev), "models/wmodels/w_pulsegun.mdl");

	m_iDefaultAmmo = PULSERIFLE_DEFAULT_GIVE;

	FallInit();
}


void CPulseRifle::Precache( void )
{
	PRECACHE_SOUND( "weapons/pulse1.wav" );
	PRECACHE_MODEL( "models/vmodels/v_pulsegun.mdl" );	
	PRECACHE_MODEL( "models/wmodels/w_pulsegun.mdl" );	
	PRECACHE_MODEL( "models/pmodels/p_pulsegun.mdl" );	


	UTIL_PrecacheOther("blaster_beam");
}

int CPulseRifle::AddToPlayer( CBasePlayer *pPlayer )
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

int CPulseRifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "pulse";
	p->iMaxAmmo1 = PULSERIFLE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_PULSERIFLE;
	p->iFlags = 0;
	p->iWeight = PULSERIFLE_WEIGHT;

	return 1;
}


BOOL CPulseRifle::Deploy( )
{
	return DefaultDeploy( "models/vmodels/v_pulsegun.mdl", "models/pmodels/p_pulsegun.mdl", GAUSS_DRAW, "gauss" );
}


void CPulseRifle::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim( GAUSS_HOLSTER );
}
void CPulseRifle::PrimaryAttack()
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

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0)
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	
	SendWeaponAnim( GAUSS_FIRE );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 2;


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( )+gpGlobals->v_up * -8 + gpGlobals->v_right * 4;
	
	EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/pulse1.wav", RANDOM_FLOAT(0.95, 1.0), ATTN_NORM, 0, 98 + RANDOM_LONG(0,7));

	CBlasterBeam  *m_pBeam = CBlasterBeam::Fire();
	m_pBeam->pev->origin = vecSrc;
	m_pBeam->pev->owner = m_pPlayer->edict();
	m_pBeam->pev->velocity = gpGlobals->v_forward * 2000;
	m_pBeam->pev->angles = UTIL_VecToAngles( m_pBeam->pev->velocity );
	
	m_flTimeWeaponIdle = gpGlobals->time + 1.0;
	m_flNextPrimaryAttack = gpGlobals->time + .09; 
}
void CPulseRifle::SecondaryAttack()
{

}

void CPulseRifle::WeaponIdle( void )
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


/*
enum gauss_e {
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

class CPulseRifle : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 2; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );
	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( void );
	void Holster( void );
	void Reload( void );
	void WeaponIdle( void );
	float m_flSoundDelay;
	int m_iShell;
	int fire_phase;
	int m_iBeam;
};
LINK_ENTITY_TO_CLASS( weapon_pulserifle, CPulseRifle );

int CPulseRifle::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "pulse";
	p->iMaxAmmo1 = PULSERIFLE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PULSERIFLE;
	p->iFlags = 0;
	p->iWeight = PULSERIFLE_WEIGHT;
	
	return 1;
}

int CPulseRifle::AddToPlayer( CBasePlayer *pPlayer )
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

void CPulseRifle::Spawn( )
{
	Precache( );
	m_iId = WEAPON_PULSERIFLE;
	
	SET_MODEL(ENT(pev), "models/w_gauss.mdl");
	
	m_iDefaultAmmo = 40;

	FallInit();
}


void CPulseRifle::Precache( void )
{
	m_iBeam = PRECACHE_MODEL( "sprites/smoke.spr" );
	UTIL_PrecacheOther("blaster_beam");

	PRECACHE_MODEL("models/w_gauss.mdl");
	PRECACHE_MODEL("models/vmodels/v_pulsegun.mdl");
}

BOOL CPulseRifle::Deploy( )
{
	return DefaultDeploy( "models/vmodels/v_pulsegun.mdl", "models/pmodels/p_railgun.mdl", GAUSS_DRAW, "gauss" );
}


void CPulseRifle::Holster( )
{
	m_pPlayer->m_flNextAttack = gpGlobals->time + 0.5;
	SendWeaponAnim( GAUSS_HOLSTER );
}

void CPulseRifle::SecondaryAttack(  )
{

}

void CPulseRifle::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] == 0) 
		return;

	if (m_pPlayer->pev->waterlevel == 3)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->time + 0.5;
		return;
	}
	
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	
	SendWeaponAnim( GAUSS_FIRE );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	//EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/hblaster_fire.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector vecAiming = gpGlobals->v_forward;
	Vector vecSrc	 = m_pPlayer->GetGunPosition( )+gpGlobals->v_up * -8 + gpGlobals->v_right * 4;
	
	CBlasterBeam  *m_pBeam = CBlasterBeam::Fire();
	m_pBeam->pev->origin = vecSrc;
	m_pBeam->pev->owner = m_pPlayer->edict();
	m_pBeam->pev->velocity = gpGlobals->v_forward * 1000;
	m_pBeam->pev->angles = UTIL_VecToAngles( m_pBeam->pev->velocity );

	m_flNextPrimaryAttack = gpGlobals->time + 0.1;
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}


void CPulseRifle::Reload( void )
{
	
}


void CPulseRifle::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );
	
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


		SendWeaponAnim( iAnim );
		
}
*/