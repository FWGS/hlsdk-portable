//-------------------------------------------------
//-												---
//-			supergun.cpp						---
//-												---
//-------------------------------------------------
//			par Julien		-----------------------
//-------------------------------------------------
//- code de la mitrailleuse laser		-----------
//-------------------------------------------------


//----------------------------------------
// inclusions

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "effects.h"

extern int gmsgClientDecal;




enum supergun_e {
	SG_IDLE = 0,
	SG_SHOOT1,
	SG_BIGSHOOT,
	SG_DRAW,
	SG_RELOAD
};

class CSuperGun : public CBasePlayerWeapon
{
public:
/*	int		Save( CSave &save );
	int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
*/
	void Spawn( void );
	void Precache( void );
	int iItemSlot( void ) { return 4; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	BOOL Deploy( void );
	void Holster( int skiplocal = 0 );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	void Reload( void );
	void WeaponIdle( void );

	void Shoot ( int mode );

private:
	unsigned short m_usSG;

};
LINK_ENTITY_TO_CLASS( weapon_supergun, CSuperGun );

/*
TYPEDESCRIPTION	CGauss::m_SaveData[] = 
{
	DEFINE_FIELD( CGauss, m_fInAttack, FIELD_INTEGER ),
	DEFINE_FIELD( CGauss, m_flStartCharge, FIELD_TIME ),
	DEFINE_FIELD( CGauss, m_flPlayAftershock, FIELD_TIME ),
	DEFINE_FIELD( CGauss, m_flNextAmmoBurn, FIELD_TIME ),
	DEFINE_FIELD( CGauss, m_fPrimaryFire, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CGauss, CBasePlayerWeapon );
*/


//--------------------------------------------------------------------------


class CSGBall : public CBaseMonster
{
public :
	void Spawn( void );
	void Precache( void );
	void EXPORT AnimateThink( void );
	void EXPORT ExplodeTouch( CBaseEntity *pOther );

	int	Classify ( void );
	static CSGBall *CreateSGBall( Vector vecOrigin, Vector vecAngles, entvars_s *pevOwner );

	int m_iSprite;
};
LINK_ENTITY_TO_CLASS( sg_ball, CSGBall );



CSGBall *CSGBall :: CreateSGBall ( Vector vecOrigin, Vector vecAngles, entvars_s *pevOwner )
{
	CSGBall *pBall = GetClassPtr( (CSGBall *)NULL );

	UTIL_MakeAimVectors ( vecAngles );

	float x, y, z;
	do
	{
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		z = x*x+y*y;
	}
	while (z > 1);

	Vector vecDir = gpGlobals->v_forward +
					x * VECTOR_CONE_6DEGREES.x * gpGlobals->v_right +
					y * VECTOR_CONE_6DEGREES.y * gpGlobals->v_up;

	pBall->pev->angles = UTIL_VecToAngles ( vecDir.Normalize() );

	UTIL_SetOrigin( pBall->pev, vecOrigin );

	pBall->pev->owner = ENT ( pevOwner );

	pBall->Spawn();
	pBall->SetTouch( &CSGBall::ExplodeTouch );
	
	return pBall;
}

void CSGBall :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("sg_ball");

	SET_MODEL(ENT(pev), "sprites/cnt1.spr");
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->renderamt = 190;
	pev->scale = 0.1;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CSGBall::AnimateThink );
	SetTouch( &CSGBall::ExplodeTouch );

	pev->dmgtime = gpGlobals->time; // keep track of when ball spawned
	pev->nextthink = gpGlobals->time + 0.1;

	UTIL_MakeVectors ( pev->angles );
	pev->velocity = gpGlobals->v_forward * 1700;

	m_flFieldOfView = -1;
	m_hEnemy = 0;
}

int CSGBall::Classify ( void )
{
	return CLASS_PLAYER_BIOWEAPON;
}

void CSGBall :: Precache( void )
{
	PRECACHE_MODEL("sprites/cnt1.spr");
	PRECACHE_MODEL("sprites/xspark1.spr");
	PRECACHE_MODEL("sprites/xspark4.spr");
	PRECACHE_SOUND("debris/zap4.wav");
	PRECACHE_SOUND("weapons/electro4.wav");

	m_iSprite = PRECACHE_MODEL("sprites/cnt1.spr");
}


void CSGBall :: AnimateThink( void  )
{
	pev->nextthink = gpGlobals->time + 0.05;

	// sprite
	
	pev->frame = ((int)pev->frame + 1) % 5;

	float delta = gpGlobals->time - pev->dmgtime;

	if ( delta > 5 || pev->velocity.Length() < 10)
	{
		SetTouch( NULL );
		UTIL_Remove( this );
	}

	// train

	if ( delta > 0 && delta < 0.9 )
	{
		CSprite *pTrail = CSprite::SpriteCreate ( "sprites/cnt1.spr", pev->origin, TRUE );
	//	pTrail->AnimateAndDie ( 7 );
		pTrail->AnimateAndDie ( 0.2 );
		pTrail->SetScale ( 0.07 );
		pTrail->SetTransparency ( kRenderTransAdd, 230, 255, 230, 60, kRenderFxNone );
		pTrail->Expand ( pTrail->pev->scale, 120 );

	}

	// ennemi

	if ( m_hEnemy == 0 )
	{
		Look( 600 );
		m_hEnemy = BestVisibleEnemy( );
	}
	
	if ( m_hEnemy != 0 && FVisible( m_hEnemy ))
	{
		Vector vecDirToEnemy = ( m_hEnemy->BodyTarget( pev->origin ) - pev->origin ).Normalize();

		Vector angEnnemy = UTIL_VecToAngles(vecDirToEnemy);
		Vector angBall = UTIL_VecToAngles(pev->velocity);

		float difX = UTIL_AngleDiff( angEnnemy.x, angBall.x );
		float difY = UTIL_AngleDiff( angEnnemy.y, angBall.y );

		if ( fabs ( difX ) < 15 && fabs ( difY ) < 15 )
		{
			pev->velocity = vecDirToEnemy * pev->velocity.Length();
		}
	}
	else
	{
		m_hEnemy = 0;
	}


}


void CSGBall::ExplodeTouch( CBaseEntity *pOther )
{
	// truc affreux pour creer une particule a travers ce message pour les decals

	TraceResult trace = UTIL_GetGlobalTrace( );
	Vector vecNorm = trace.vecPlaneNormal.Normalize();

	MESSAGE_BEGIN( MSG_ALL, gmsgClientDecal );

		WRITE_COORD( trace.vecEndPos.x );			// xyz source
		WRITE_COORD( trace.vecEndPos.y );
		WRITE_COORD( trace.vecEndPos.z );
		WRITE_COORD( vecNorm.x );						// xyz norme
		WRITE_COORD( vecNorm.y );
		WRITE_COORD( vecNorm.z );
		WRITE_CHAR ( 'a' );						// type de texture
		WRITE_BYTE ( 5 );						//  4 == electro-rocket

	MESSAGE_END();


	if (pOther->pev->takedamage)
	{
		if ( pOther->pev != VARS ( pev->owner ) )
		{
			TraceResult tr = UTIL_GetGlobalTrace( );

			ClearMultiDamage( );
			pOther->TraceAttack(pev, gSkillData.plrDmgSupergun, pev->velocity.Normalize(), &tr, DMG_ENERGYBEAM ); 
			ApplyMultiDamage( pev, pev );

			UTIL_EmitAmbientSound( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.3, ATTN_NORM, 0, RANDOM_LONG( 90, 99 ) );
		}
	}

	UTIL_Remove( this );
}


//--------------------------------------------------------------------------
//
// SuperGun
//
//--------------------------------------------------------------------------



void CSuperGun::Spawn( )
{
	Precache( );
	m_iId = WEAPON_SUPERGUN;
	SET_MODEL(ENT(pev), "models/w_supergun.mdl");

	m_iDefaultAmmo = SUPERGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void CSuperGun::Precache( void )
{
	PRECACHE_MODEL("models/w_supergun.mdl");
	PRECACHE_MODEL("models/v_supergun.mdl");
	PRECACHE_MODEL("models/p_gauss.mdl");

//	PRECACHE_MODEL("sprites/animglow01.spr");
	PRECACHE_MODEL("sprites/blueflare1.spr");

	PRECACHE_SOUND("items/9mmclip1.wav");
	PRECACHE_SOUND("turret/tu_fire1.wav");
	PRECACHE_SOUND("weapons/gauss2.wav");
	PRECACHE_SOUND("debris/beamstart14.wav");

	m_usSG = PRECACHE_EVENT( 1, "events/supergun.sc" );

	UTIL_PrecacheOther ( "sg_ball" );
}

int CSuperGun::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();

		m_pPlayer->TextAmmo( TA_SUPERGUN );

		return TRUE;
	}
	return FALSE;
}

int CSuperGun::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "chewinggum";
	p->iMaxAmmo1 = SUPERGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SUPERGUN_MAX_CLIP;
	p->iSlot = 3;
	p->iPosition = 3;
	p->iId = m_iId = WEAPON_SUPERGUN;
	p->iFlags = 0;
	p->iWeight = SUPERGUN_WEIGHT;

	return 1;
}


BOOL CSuperGun::Deploy( )
{
	BOOL bResult = DefaultDeploy( "models/v_supergun.mdl", "models/p_gauss.mdl", SG_DRAW, "supergun" );

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT ( 2, 5 );
	return bResult;
}


void CSuperGun::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( SG_DRAW );
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CSuperGun :: Reload ( void )
{
	DefaultReload( SUPERGUN_MAX_CLIP, SG_RELOAD, 69 / 25 );
	
	int iskin = 0;
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usSG, 1, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, iskin, 0, 0, 0 );
}


void CSuperGun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	// enflamme le gaz

	if ( m_pPlayer->IsInGaz() == TRUE )
		m_pPlayer->m_bFireInGaz = TRUE;


	if ( m_iClip < 1 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_iClip--;
	m_pPlayer->Gunflash ();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);


	// flash

	CSprite *pMuzzle = CSprite::SpriteCreate ( "sprites/blueflare1.spr"/*"sprites/animglow01.spr"*/, Vector(0,0,0), TRUE );
	pMuzzle->SetAttachment ( m_pPlayer->edict(), 1 );

	pMuzzle->SetScale ( 0.3 );
	pMuzzle->SetTransparency ( kRenderTransAdd, 128, 128, 220, 250, kRenderFxNone );

//	pMuzzle->pev->frame = RANDOM_LONG(3,6);
	pMuzzle->SetThink ( &CBaseEntity::SUB_Remove );
	pMuzzle->pev->nextthink = gpGlobals->time + 0.075;


	SendWeaponAnim( SG_SHOOT1 );

	Shoot ( 0 );

	int iskin = (int)( (SUPERGUN_MAX_CLIP - m_iClip) * 10 / SUPERGUN_MAX_CLIP );
	PLAYBACK_EVENT_FULL( 0, m_pPlayer->edict(), m_usSG, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, iskin, 1, 0, 0 );

	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 1,4 );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.1;
}



void CSuperGun::SecondaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextSecondaryAttack = m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	// enflamme le gaz

	if ( m_pPlayer->IsInGaz() == TRUE )
		m_pPlayer->m_bFireInGaz = TRUE;


	if ( m_iClip < 1 )
	{
		PlayEmptySound( );
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
		return;
	}

	int iProjectiles = Q_min ( m_iClip, 8 );	// n'en tire pas plus qu'il n'en a ...

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_iClip -= iProjectiles;
	m_pPlayer->Gunflash ();

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);


	// flash

	CSprite *pMuzzle = CSprite::SpriteCreate ( "sprites/blueflare1.spr", Vector(0,0,0), TRUE );
	pMuzzle->SetAttachment ( m_pPlayer->edict(), 1 );
	pMuzzle->SetScale ( 0.3 );
	pMuzzle->SetTransparency ( kRenderTransAdd, 128, 128, 220, 250, kRenderFxNone );
	pMuzzle->SetThink ( &CBaseEntity::SUB_Remove );
	pMuzzle->pev->nextthink = gpGlobals->time + 0.2;

	SendWeaponAnim( SG_BIGSHOOT );

	// sons
	EMIT_SOUND ( m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss2.wav", 1.0, ATTN_NORM );
	EMIT_SOUND ( m_pPlayer->edict(), CHAN_ITEM, "debris/beamstart14.wav", 1.0, ATTN_NORM );


	UTIL_MakeVectors ( m_pPlayer->pev->v_angle );

	Vector Point [8];

	Point [0] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 8 - gpGlobals->v_up * 8;
	Point [1] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 10 - gpGlobals->v_up * 9;
	Point [2] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 11 - gpGlobals->v_up * 11;
	Point [3] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 10 - gpGlobals->v_up * 13;
	Point [4] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 8 - gpGlobals->v_up * 14;
	Point [5] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 6 - gpGlobals->v_up * 13;
	Point [6] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 5 - gpGlobals->v_up * 11;
	Point [7] = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 + gpGlobals->v_right * 6 - gpGlobals->v_up * 9;


	for ( int i=0; i<iProjectiles; i++ )
	{

		CSGBall :: CreateSGBall ( Point[i],
			m_pPlayer->pev->v_angle, m_pPlayer->pev );
	}


	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 3,6 );
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 31 / 20;

}


void CSuperGun :: Shoot ( int mode )
{
	EMIT_SOUND ( m_pPlayer->edict(), CHAN_WEAPON, "turret/tu_fire1.wav", 1.0, ATTN_NORM );
	//CBaseMonster *pBall = (CBaseMonster*)Create( "controller_energy_ball", m_pPlayer->GetGunPosition(), m_pPlayer->pev->v_angle, edict() );

	UTIL_MakeVectors ( m_pPlayer->pev->v_angle );

	CSGBall :: CreateSGBall (
		m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 15 
		+ gpGlobals->v_right * 8
		- gpGlobals->v_up * 8,
		m_pPlayer->pev->v_angle, m_pPlayer->pev );
}


void CSuperGun::WeaponIdle( void )
{
	ResetEmptySound( );

	if (m_flTimeWeaponIdle > gpGlobals->time)
		return;

	SendWeaponAnim( SG_IDLE );
	m_flTimeWeaponIdle = gpGlobals->time + RANDOM_FLOAT ( 10, 15 );
}






class CSuperGunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_supergunammo.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_supergunammo.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_SUPERGUNCLIPGIVE, "chewinggum", SUPERGUN_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_supergun, CSuperGunAmmo );
