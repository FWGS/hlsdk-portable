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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"
#include "shake.h"
#include "displacerball.h"

#if !CLIENT_DLL

extern edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );

LINK_ENTITY_TO_CLASS(info_displacer_xen_target, CPointEntity)
LINK_ENTITY_TO_CLASS(info_displacer_earth_target, CPointEntity)

int iPortalSprite = 0;
int iRingSprite = 0;

LINK_ENTITY_TO_CLASS(displacer_ball, CDisplacerBall)

TYPEDESCRIPTION	CDisplacerBall::m_SaveData[] =
{
	DEFINE_FIELD(CDisplacerBall, m_iBeams, FIELD_INTEGER),
	DEFINE_ARRAY(CDisplacerBall, m_pBeam, FIELD_CLASSPTR, 8),
};

IMPLEMENT_SAVERESTORE(CDisplacerBall, CBaseEntity);

void CDisplacerBall::Spawn(void)
{
	pev->classname = MAKE_STRING("displacer_ball");
	pev->movetype = MOVETYPE_FLY;

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/exit1.spr");
	pev->frame = 0;
	pev->scale = 0.75;

	SetTouch( &CDisplacerBall::Touch );
	SetThink( &CDisplacerBall::FlyThink );
	pev->nextthink = gpGlobals->time + 0.2;
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_iBeams = 0;
}

void CDisplacerBall::FlyThink()
{
	ArmBeam( -1 );
	ArmBeam( 1 );
	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::ArmBeam( int iSide )
{
	//This method is identical to the Alien Slave's ArmBeam, except it treats m_pBeam as a circular buffer.
	if( m_iBeams > 7 )
		m_iBeams = 0;

	TraceResult tr;
	float flDist = 1.0;

	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = gpGlobals->v_forward * 32.0 + iSide * gpGlobals->v_right * 16.0 + gpGlobals->v_up * 36.0 + pev->origin;
	Vector vecAim = gpGlobals->v_up * RANDOM_FLOAT( -1.0, 1.0 );
	Vector vecEnd = (iSide * gpGlobals->v_right * RANDOM_FLOAT( 0.0, 1.0 ) + vecAim) * 512.0 + vecSrc;
	UTIL_TraceLine( &vecSrc.x, &vecEnd.x, dont_ignore_monsters, ENT( pev ), &tr );

	if( flDist > tr.flFraction )
		flDist = tr.flFraction;

	// Couldn't find anything close enough
	if( flDist == 1.0 )
		return;

	// The beam might already exist if we've created all beams before.
	if( !m_pBeam[ m_iBeams ] )
		m_pBeam[ m_iBeams ] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );

	if( !m_pBeam[ m_iBeams ] )
		return;

	CBaseEntity* pHit = Instance( tr.pHit );

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	if( pHit && pHit->pev->takedamage != DAMAGE_NO )
	{
		//Beam hit something, deal radius damage to it
		m_pBeam[ m_iBeams ]->EntsInit( pHit->entindex(), entindex() );
		m_pBeam[ m_iBeams ]->SetColor( 255, 255, 255 );
		m_pBeam[ m_iBeams ]->SetBrightness( 255 );
		m_pBeam[ m_iBeams ]->SetNoise( 10 );

		RadiusDamage( tr.vecEndPos, pev, pevOwner, 25, 15, CLASS_NONE, DMG_ENERGYBEAM );
	}
	else
	{
		m_pBeam[ m_iBeams ]->PointEntInit( tr.vecEndPos, entindex() );
		m_pBeam[ m_iBeams ]->SetColor( 96, 128, 16 );
		m_pBeam[ m_iBeams ]->SetBrightness( 255 );
		m_pBeam[ m_iBeams ]->SetNoise( 80 );
	}
	m_iBeams++;
}

void CDisplacerBall::Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles )
{
	CDisplacerBall *pSpit = GetClassPtr((CDisplacerBall *)NULL);
	pSpit->Spawn();
	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->angles = vecAngles;
	pSpit->pev->owner = ENT(pevOwner);
}

void CDisplacerBall::SelfCreate(entvars_t *pevOwner,Vector vecStart)
{
	CDisplacerBall *pSelf = GetClassPtr((CDisplacerBall *)NULL);
	UTIL_SetOrigin(pSelf->pev, vecStart);
	pSelf->Spawn();
	pSelf->ClearBeams();

	pSelf->pev->owner = ENT(pevOwner);
	pSelf->Circle();
	pSelf->SetTouch( NULL );
	pSelf->SetThink(&CDisplacerBall::KillThink);
	pSelf->pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2f : 0.5f );
}

void CDisplacerBall::Touch(CBaseEntity *pOther)
{
	// Do not collide with the owner.
	if (ENT(pOther->pev) == pev->owner || (ENT(pOther->pev) == VARS(pev->owner)->owner))
		return;

	TraceResult tr;
	Vector vecSpot;
	Vector vecSrc;
	pev->enemy = pOther->edict();
	CBaseEntity *pTarget = NULL;

	if (!pOther->pev->takedamage)
	{
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/displacer_impact.wav", 1, ATTN_NORM, 0, 100);
		UTIL_MuzzleLight( pOther->pev->origin, 160.0f, 255, 180, 96, 1.0f, 100.0f );
	}

	if( ( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() ) && pOther->IsPlayer() )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)pOther;
		pTarget = GetClassPtr( (CBaseEntity *)VARS( EntSelectSpawnPoint( pPlayer ) ) );

		if( pTarget )
		{
			// UTIL_ScreenFade( pPlayer, Vector( 0, 200, 0 ), 0.5, 0.5, 255, FFADE_IN );
			Vector tmp = pTarget->pev->origin;
			UTIL_CleanSpawnPoint( tmp, 100 );

			EMIT_SOUND( pPlayer->edict(), CHAN_BODY, "weapons/displacer_self.wav", 1, ATTN_NORM );

			// make origin adjustments (origin in center, not at feet)
			tmp.z -= pPlayer->pev->mins.z + 36;
			tmp.z++;

			pPlayer->pev->flags &= ~FL_ONGROUND;

			UTIL_SetOrigin( pPlayer->pev, tmp );

			pPlayer->pev->angles = pTarget->pev->angles;

			pPlayer->pev->v_angle = pTarget->pev->angles;

			pPlayer->pev->fixangle = TRUE;

			pPlayer->pev->velocity = pOther->pev->basevelocity = g_vecZero;
		}
	}

	pev->movetype = MOVETYPE_NONE;

	Circle();

	SetThink(&CDisplacerBall::KillThink);
	pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2f : 0.5f );
}

void CDisplacerBall::Circle( void )
{
	// portal circle
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_BEAMCYLINDER);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z + 800); // reach damage radius over .2 seconds
		WRITE_SHORT(iRingSprite);
		WRITE_BYTE(0); // startframe
		WRITE_BYTE(0); // framerate
		WRITE_BYTE(3); // life
		WRITE_BYTE(16);  // width
		WRITE_BYTE(0);   // noise
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255);   // r, g, b
		WRITE_BYTE(255); // brightness
		WRITE_BYTE(0);		// speed
	MESSAGE_END();
	UTIL_MuzzleLight( pev->origin, 160.0f, 255, 180, 96, 1.0f, 100.0f );
}

void CDisplacerBall::KillThink( void )
{
	if( pRemoveEnt )
		UTIL_Remove( pRemoveEnt );
	SetThink( &CDisplacerBall::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.2f;
}

void CDisplacerBall::ExplodeThink( void )
{
	ClearBeams();

	pev->effects |= EF_NODRAW;

	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/displacer_teleport.wav", VOL_NORM, ATTN_NORM);

	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;
	pev->owner = NULL;

	UTIL_Remove( this );

	::RadiusDamage( pev->origin, pev, pevOwner, gSkillData.plrDmgDisplacer, gSkillData.plrDisplacerRadius, CLASS_NONE, DMG_ENERGYBEAM );
}

void CDisplacerBall::ClearBeams( void )
{
	for( int i = 0;i < 8; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
}


#endif // !defined ( CLIENT_DLL )

#define DISPLACER_SECONDARY_USAGE 60
#define DISPLACER_PRIMARY_USAGE 20

enum displacer_e {
	DISPLACER_IDLE1 = 0,
	DISPLACER_IDLE2,
	DISPLACER_SPINUP,
	DISPLACER_SPIN,
	DISPLACER_FIRE,
	DISPLACER_DRAW,
	DISPLACER_HOLSTER,
};

LINK_ENTITY_TO_CLASS(weapon_displacer, CDisplacer)

//=========================================================
// Purpose:
//=========================================================
int CDisplacer::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iFlags = 0;
	p->iSlot = 5;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_DISPLACER;
	p->iWeight = DISPLACER_WEIGHT;

	return 1;
}

//=========================================================
// Purpose:
//=========================================================
int CDisplacer::AddToPlayer(CBasePlayer *pPlayer)
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

//=========================================================
// Purpose:
//=========================================================
BOOL CDisplacer::PlayEmptySound(void)
{
	if (m_iPlayEmptySound)
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "buttons/button11.wav", 1, ATTN_NORM);
		m_iPlayEmptySound = 0;
		return 0;
	}
	return 0;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Spawn()
{
	Precache();
	m_iId = WEAPON_DISPLACER;
	SET_MODEL(ENT(pev), "models/w_displacer.mdl");

	m_iDefaultAmmo = DISPLACER_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Precache(void)
{
	PRECACHE_MODEL("models/v_displacer.mdl");
	PRECACHE_MODEL("models/w_displacer.mdl");
	PRECACHE_MODEL("models/p_displacer.mdl");

	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("weapons/displacer_fire.wav");
	PRECACHE_SOUND("weapons/displacer_impact.wav");
	PRECACHE_SOUND("weapons/displacer_self.wav");
	PRECACHE_SOUND("weapons/displacer_spin.wav");
	PRECACHE_SOUND("weapons/displacer_spin2.wav");
	PRECACHE_SOUND("weapons/displacer_start.wav");
	PRECACHE_SOUND("weapons/displacer_teleport.wav");
	PRECACHE_SOUND("weapons/displacer_teleport_player.wav");

	PRECACHE_SOUND("buttons/button11.wav");

	PRECACHE_MODEL("sprites/lgtning.spr");

#if !CLIENT_DLL
	iPortalSprite = PRECACHE_MODEL("sprites/exit1.spr");
	iRingSprite = PRECACHE_MODEL("sprites/disp_ring.spr");
#endif

	UTIL_PrecacheOther("displacer_ball");

	m_usDisplacer = PRECACHE_EVENT(1, "events/displacer.sc");
}

//=========================================================
// Purpose:
//=========================================================
BOOL CDisplacer::Deploy()
{
	return DefaultDeploy("models/v_displacer.mdl", "models/p_displacer.mdl", DISPLACER_DRAW, "displacer", UseDecrement());
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Holster(int skiplocal /* = 0 */)
{
	m_fInReload = FALSE;// cancel any reload in progress.

	ClearBeams();
	ClearSpin();

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0f;
	SendWeaponAnim(DISPLACER_HOLSTER);
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::SecondaryAttack(void)
{
	if (m_fFireOnEmpty || !CanFireDisplacer(DISPLACER_SECONDARY_USAGE))
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_iFireMode = FIREMODE_BACKWARD;

	SetThink (&CDisplacer::SpinUp);

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 4.0;
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::PrimaryAttack()
{
	if ( m_fFireOnEmpty || !CanFireDisplacer(DISPLACER_PRIMARY_USAGE))
	{
		PlayEmptySound();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.3f;
		return;
	}
	m_iFireMode = FIREMODE_FORWARD;

	SetThink (&CDisplacer::SpinUp);
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.6;
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_2DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.5)
	{
		iAnim = DISPLACER_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}
	else
	{
		iAnim = DISPLACER_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 3.0f;
	}

	SendWeaponAnim(iAnim, UseDecrement());
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::ClearSpin( void )
{

	switch (m_iFireMode)
	{
	case FIREMODE_FORWARD:
		STOP_SOUND(ENT(pev), CHAN_WEAPON, "weapons/displacer_spin.wav");
		break;
	case FIREMODE_BACKWARD:
		STOP_SOUND(ENT(pev), CHAN_WEAPON, "weapons/displacer_spin2.wav");
		break;
	}
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::SpinUp( void )
{
	SendWeaponAnim( DISPLACER_SPINUP, UseDecrement());

	LightningEffect();

	if( m_iFireMode == FIREMODE_FORWARD )
	{
		EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/displacer_spin.wav", 1, ATTN_NORM );
		SetThink (&CDisplacer::Displace);
	}
	else
	{
		EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/displacer_spin2.wav", 1, ATTN_NORM );
		SetThink (&CDisplacer::Teleport);
	}
	pev->nextthink = gpGlobals->time + 0.9;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.3;
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Displace( void )
{
	ClearBeams();
	ClearSpin();

	SendWeaponAnim( DISPLACER_FIRE, UseDecrement());
	EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/displacer_fire.wav", 1, ATTN_NORM );

	// player "shoot" animation
        m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_pPlayer->pev->punchangle.x -= 2;
#if !CLIENT_DLL
	Vector vecSrc;
	UseAmmo(DISPLACER_PRIMARY_USAGE);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	vecSrc = m_pPlayer->GetGunPosition();
	vecSrc = vecSrc + gpGlobals->v_forward	* 22;
	vecSrc = vecSrc + gpGlobals->v_right	* 8;
	vecSrc = vecSrc + gpGlobals->v_up		* -12;

	CDisplacerBall::Shoot( m_pPlayer->pev, vecSrc, gpGlobals->v_forward * 500, m_pPlayer->pev->v_angle );

	SetThink( NULL );
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::Teleport( void )
{
	const char *pszName;
	ClearBeams();
	ClearSpin();
#if !CLIENT_DLL
	CBaseEntity *pTarget = NULL;
	Vector tmp( 0, 0, 0 );

	if( g_pGameRules->IsMultiplayer() && !g_pGameRules->IsCoOp() )
	{
		pTarget = GetClassPtr( (CBaseEntity *)VARS( EntSelectSpawnPoint( m_pPlayer ) ) );
	}
	else
	{
		if( !m_pPlayer->m_fInXen )
			pszName = "info_displacer_xen_target";
		else
			pszName = "info_displacer_earth_target";
		pTarget = UTIL_FindEntityByClassname( 0, pszName );
	}

	if( pTarget )
		tmp = pTarget->pev->origin;

	if( pTarget && /*HACK*/( tmp != Vector( 0, 0, 0 )/*HACK*/ ) )
	{
		// UTIL_ScreenFade( m_pPlayer, Vector( 0, 200, 0 ), 0.5, 0.5, 255, FFADE_IN );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();

		UseAmmo(DISPLACER_SECONDARY_USAGE);

		UTIL_CleanSpawnPoint( tmp, 50 );

		EMIT_SOUND( m_pPlayer->edict(), CHAN_WEAPON, "weapons/displacer_self.wav", 1, ATTN_NORM );
	 	CDisplacerBall::SelfCreate(m_pPlayer->pev, m_pPlayer->pev->origin);

		// make origin adjustments (origin in center, not at feet)
		//tmp.z -= m_pPlayer->pev->mins.z + 36;
		tmp.z+=37;

		m_pPlayer->pev->flags &= ~FL_ONGROUND;

		UTIL_SetOrigin(m_pPlayer->pev, tmp);

		m_pPlayer->pev->angles = pTarget->pev->angles;

		m_pPlayer->pev->v_angle = pTarget->pev->angles;

		m_pPlayer->pev->fixangle = TRUE;
		m_pPlayer->pev->velocity = m_pPlayer->pev->basevelocity = g_vecZero;

		if( !g_pGameRules->IsMultiplayer())
		{
			m_pPlayer->m_fInXen = !m_pPlayer->m_fInXen;
			if (m_pPlayer->m_fInXen)
				m_pPlayer->pev->gravity = 0.6;
			else
				m_pPlayer->pev->gravity = 1.0;
		}
	}
	else
	{
		EMIT_SOUND( edict(), CHAN_BODY, "buttons/button11.wav", 1, ATTN_NORM );
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 3.0;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.9;
	}

	SetThink( NULL );
#endif
}

void CDisplacer::LightningEffect( void )
{
#if !CLIENT_DLL
	int m_iBeams = 0;

	if( g_pGameRules->IsMultiplayer())
		return;

	for( int i = 2; i < 5; ++i )
	{
		if( !m_pBeam[m_iBeams] )
			m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 16 );
		m_pBeam[m_iBeams]->EntsInit( m_pPlayer->entindex(), m_pPlayer->entindex() );
 		m_pBeam[m_iBeams]->SetStartAttachment( i );
		m_pBeam[m_iBeams]->SetEndAttachment( i == 4 ? i - 2 : i + 1 );
		m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
		m_pBeam[m_iBeams]->SetBrightness( 240 );
		m_pBeam[m_iBeams]->SetNoise( 60 );
		m_pBeam[m_iBeams]->SetScrollRate( 30 );
		m_pBeam[m_iBeams]->pev->scale = 10;
		m_iBeams++;
	}
#endif
}

void CDisplacer::ClearBeams( void )
{
#if !CLIENT_DLL
	if( g_pGameRules->IsMultiplayer())
		return;

	for( int i = 0; i < 3; i++ )
	{
		if( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
#endif
}

//=========================================================
// Purpose:
//=========================================================
void CDisplacer::UseAmmo(int count)
{
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= count;
}

//=========================================================
// Purpose:
//=========================================================
BOOL CDisplacer::CanFireDisplacer( int count ) const
{
	return m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= count;
}
