/***
*AR2 Impulse Rifle from HALF-LIFE 2 , rewritten to hlsdk by Solexid.
*Uses custom models and sounds
*
****/

#include "extdll.h"
#include "util.h"
#include "customentity.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "customweapons.h"

#define AR2_BEAM_SPRITE		"sprites/xbeam1.spr"
enum AR2_e
{
	AR2_LONGIDLE = 0,
	AR2_IDLE1,
	AR2_LAUNCH,
	AR2_RELOAD,
	AR2_DEPLOY,
	AR2_FIRE1,
	AR2_FIRE2,
	AR2_FIRE3,
};

class CAR2 : public CBasePlayerWeapon
{
public:
	void Spawn(void);
	void Precache(void);
	int iItemSlot(void) { return 3; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer(CBasePlayer *pPlayer);
	CBeam* m_pBeam1;
	void PrimaryAttack(void);
	void Cleaner(void);
	void SecondaryAttack(void);
	int SecondaryAmmoIndex(void);
	BOOL Deploy(void);
	void Holster(int skiplocal);
	void MyAnim(int iAnim);
	void Reload(void);
	void WeaponIdle(void);
	float m_flNextAnimTime;
	int m_iShell;

	virtual BOOL UseDecrement(void)
	{
		return false;

	}

};

LINK_ENTITY_TO_CLASS(weapon_ar2, CAR2);
class CAR2Ball: public CBaseEntity
{
public:
	static CAR2Ball *AR2Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time);
	void AR2Touch(CBaseEntity *pOther);
	void Precache();
	void Spawn();
	void AR2Think();
	void Detonate();
	void Explode(TraceResult *tr, int);
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);
	virtual float TouchGravGun( CBaseEntity *attacker, int stage )
	{
		pev->owner = attacker->edict();

		if( stage == 3 )
		{
			pev->dmgtime = gpGlobals->time + 6 ;
			// play launch sound
			switch (RANDOM_LONG(0, 2))
			{
				case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2/ar2launch1.wav", 1, ATTN_NORM);	break;
				case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2/ar2launch2.wav", 1, ATTN_NORM);	break;
			}
			m_fGravgunSound = false;
		}

		if( stage == 2 && gpGlobals->time - pev->dmgtime < 15 )
		{
			pev->dmgtime = gpGlobals->time + 20 ;
			if( !m_fGravgunSound )
			{
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "ar2/ar2gravgun.wav", 0.2, ATTN_NORM, 0, 70 + RANDOM_LONG(0, 34));
				m_fGravgunSound = true;
			}
		}

		if( stage == 1 )
			pev->dmgtime = gpGlobals->time + 20 ;

		return 1600;
	}
	float m_flNextAttack;
	bool m_fRegisteredSound;
	bool m_fGravgunSound;
	float m_flLastSound;
	int m_iShockWaveTexture;
};

LINK_ENTITY_TO_CLASS(ar2grenade, CAR2Ball);
//=========================================================
//=========================================================

int CAR2Ball::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	Vector r = (pevInflictor->origin - pev->origin);
	pev->velocity = pev->velocity + r * flDamage / -7;
	pev->avelocity.x = pev->avelocity.x * 0.5 + RANDOM_FLOAT(100, -100);
	return 1;
}

void CAR2Ball::Precache()
{
	m_iShockWaveTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
	PRECACHE_MODEL( "models/ar2grenade.mdl" );
}

void CAR2Ball::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CAR2Ball::AR2Touch);	// Bounce if touched

	SetThink(&CAR2Ball::AR2Think);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmgtime = gpGlobals->time + 99999;
	pev->takedamage = DAMAGE_YES;
	pev->sequence = RANDOM_LONG(3, 6);
	pev->framerate = 1.0;
	pev->effects = EF_LIGHT;
	pev->rendermode = kRenderTransAdd;
	pev->renderfx = kRenderFxDistort;
	pev->renderamt = 256;

	pev->gravity = 0.0005;
	pev->friction = 0;

	SET_MODEL(ENT(pev), "models/ar2grenade.mdl");
	pev->dmg = 60;
	m_fRegisteredSound = FALSE;
	//UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );
	UTIL_SetOrigin(pev, pev->origin);
	pev->avelocity.x = RANDOM_LONG(-1000, 1000);
	pev->avelocity.y = RANDOM_LONG(-1000, 1000);

}

CAR2Ball * CAR2Ball::AR2Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time)
{
	CAR2Ball *pGrenade = (CAR2Ball *)CBaseEntity::Create( "ar2grenade", vecStart, vecVelocity, ENT(pevOwner) );
	pGrenade->Spawn();
	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;

	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);

	pGrenade->pev->dmgtime = gpGlobals->time + time;

	if (time < 0.1)
	{
		pGrenade->pev->nextthink = gpGlobals->time;
		pGrenade->pev->velocity = Vector(0, 0, 0);
	}
	// play launch sound
	switch (RANDOM_LONG(0, 2))
	{
		case 0:	EMIT_SOUND(ENT(pGrenade->pev), CHAN_VOICE, "ar2/ar2launch1.wav", 1, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pGrenade->pev), CHAN_VOICE, "ar2/ar2launch2.wav", 1, ATTN_NORM);	break;
	}

	return pGrenade;
}

void CAR2Ball::AR2Touch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this sphere
	if (pOther->edict() == pev->owner)
		return;

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time )
	{
		entvars_t *pevOwner = VARS(pev->owner);
		if( !pevOwner )
			pevOwner = pev;
		TraceResult tr = UTIL_GetGlobalTrace();
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, 2500, gpGlobals->v_forward, &tr, DMG_CLUB | DMG_DISINTEGRATE);
		if( pOther->IsPlayer() || pOther->IsMoving() )
		{
			pev->velocity = gpGlobals->v_forward.Normalize() * 1600;
			// play strike sound
			switch (RANDOM_LONG(0, 2))
			{
				case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2/ar2strike1.wav", 1, ATTN_NORM);	break;
				case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2/ar2strike2.wav", 1, ATTN_NORM);	break;
			}
			m_fGravgunSound = false;
			m_flLastSound = gpGlobals->time;
		}
		ApplyMultiDamage(pev, pevOwner);
		m_flNextAttack = gpGlobals->time + 0.03; // debounce
	}

	if( !pev->velocity.Length() )
		pev->velocity.z += 100;

	Vector vecTestVelocity;
	vecTestVelocity = pev->velocity;

	if (!m_fRegisteredSound && vecTestVelocity.Length() <= 60)
	{
		CSoundEnt::InsertSound(bits_SOUND_DANGER, pev->origin, pev->dmg / 0.4, 0.3);
		m_fRegisteredSound = TRUE;
	}

	if( gpGlobals->time - m_flLastSound > 0.12 )
	{
		// play bounce sound
		switch (RANDOM_LONG(0, 2))
		{
			case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2/ar2bounce1.wav", 0.5, ATTN_NORM);	break;
			case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2/ar2bounce2.wav", 0.5, ATTN_NORM);	break;
		}
		m_fGravgunSound = false;
		m_flLastSound = gpGlobals->time;
	}

	pev->framerate = pev->velocity.Length() / 200.0;
	if( pev->framerate > 0.8 )
		pev->framerate = 0.8;
	if( pev->framerate < 0.1 )
		pev->framerate = 0.1;
	pev->effects |= EF_BRIGHTLIGHT;
	pev->velocity = pev->velocity + pOther->pev->velocity;
}

void CAR2Ball::AR2Think()
{
	if (!IsInWorld())
	{
		UTIL_Remove( this );
		return;
	}

	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->dmgtime - 1 < gpGlobals->time)
	{
		CSoundEnt::InsertSound ( bits_SOUND_DANGER, pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400, 0.1 );
	}

	if (pev->dmgtime <= gpGlobals->time)
	{
		pev->renderfx = kRenderFxExplode;
		SetThink( &CAR2Ball::Detonate );
	}
	if (pev->waterlevel != 0)
	{
		SetThink( &CAR2Ball::Detonate );
	}
	pev->effects &= ~EF_BRIGHTLIGHT;
}
void CAR2Ball::Detonate( void )
{
	TraceResult tr;
	Vector		vecSpot;// trace starts here!

	vecSpot = pev->origin + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -40 ),  ignore_monsters, ENT(pev), & tr);

	Explode( &tr, DMG_SHOCK );
}

// UNDONE: temporary scorching for PreAlpha - find a less sleazy permenant solution.
void CAR2Ball::Explode( TraceResult *pTrace, int bitsDamageType )
{
	float		flRndSound;// sound randomizer

	pev->model = iStringNull;//invisible
	pev->solid = SOLID_NOT;// intangible

	pev->takedamage = DAMAGE_NO;

	// Pull out of the wall a bit
	if ( pTrace->flFraction != 1.0 )
	{
		pev->origin = pTrace->vecEndPos + (pTrace->vecPlaneNormal * (pev->dmg - 24) * 0.6);
	}

	int iContents = UTIL_PointContents ( pev->origin );


	// blast circles
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + 384 / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iShockWaveTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_BEAMCYLINDER );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16);
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 16 + ( 384/ 2 ) / .2); // reach damage radius over .3 seconds
		WRITE_SHORT( m_iShockWaveTexture );
		WRITE_BYTE( 0 ); // startframe
		WRITE_BYTE( 0 ); // framerate
		WRITE_BYTE( 2 ); // life
		WRITE_BYTE( 16 );  // width
		WRITE_BYTE( 0 );   // noise

		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );

		WRITE_BYTE( 255 ); //brightness
		WRITE_BYTE( 0 );		// speed
	MESSAGE_END();


	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, NORMAL_EXPLOSION_VOLUME, 3.0 );
	entvars_t *pevOwner;
	if ( pev->owner )
		pevOwner = VARS( pev->owner );
	else
		pevOwner = NULL;

	pev->owner = NULL; // can't traceline attack owner if this is set

	RadiusDamage ( pev->origin, pev, pevOwner, 30, 200, CLASS_NONE, bitsDamageType );
	RadiusDamage ( pev->origin, pev, pevOwner, 200, 30, CLASS_NONE, bitsDamageType );


	//flRndSound = RANDOM_FLOAT( 0 , 1 );


	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2/ar2explosion.wav", 1, ATTN_NORM);

	pev->effects |= EF_NODRAW;
	SetThink( &CAR2Ball::SUB_Remove );
	SetTouch( NULL );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;
/*
	if (iContents != CONTENTS_WATER)
	{
		int sparkCount = RANDOM_LONG(0,3);
		for ( int i = 0; i < sparkCount; i++ )
			Create( "spark_shower", pev->origin, pTrace->vecPlaneNormal, NULL );
	}*/
}

int CAR2::SecondaryAmmoIndex(void)
{
	return m_iSecondaryAmmoType;
}

void CAR2::Spawn()
{
	pev->classname = MAKE_STRING("weapon_ar2");
	Precache();
	SET_MODEL(ENT(pev), "models/w_ar2.mdl");
	m_iId = WEAPON_AR2;

	m_iDefaultAmmo = 30;

	FallInit();// get ready to fall down.
}


void CAR2::Precache(void)
{
	PRECACHE_MODEL("models/v_ar2.mdl");
	PRECACHE_MODEL("models/w_ar2.mdl");
	PRECACHE_MODEL("models/p_ar2.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/ar2grenade.mdl");	// grenade

	PRECACHE_MODEL("models/w_9mmARclip.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");

	PRECACHE_SOUND("items/clipinsert1.wav");
	PRECACHE_SOUND("items/cliprelease1.wav");

	PRECACHE_SOUND("ar2s1.wav");// H to the K

	PRECACHE_SOUND("ar2launch.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

}

int CAR2::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	if( !cvar_ar2_mp5.value )
	{
		p->pszAmmo1 = "AR2";
		p->iMaxAmmo1 = 120;
		p->pszAmmo2 = "AR2grenades";
		p->iMaxAmmo2 = 3;
	}
	else
	{
		p->pszAmmo1 = "9mm";
		p->iMaxAmmo1 = _9MM_MAX_CARRY;
		p->pszAmmo2 = "ARgrenades";
		p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
	}
	p->iMaxClip = 30;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AR2;
	p->iWeight = 25;

	return 1;
}

int CAR2::AddToPlayer(CBasePlayer *pPlayer)
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

BOOL CAR2::Deploy()
{
	Cleaner();
	return DefaultDeploy("models/v_ar2.mdl", "models/p_ar2.mdl", AR2_DEPLOY, "MP5");
}
void CAR2::Holster(int skiplocal /* = 0 */)
{
	Cleaner();
	MyAnim(AR2_DEPLOY);
}
void CAR2::MyAnim(int iAnim)
{

	m_pPlayer->pev->weaponanim = iAnim;



	MESSAGE_BEGIN(MSG_ONE, SVC_WEAPONANIM, NULL, m_pPlayer->pev);
	WRITE_BYTE(iAnim); // sequence number
	WRITE_BYTE(pev->body); // weaponmodel bodygroup.
	MESSAGE_END();
}
void CAR2::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack =gpGlobals->time+0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);
	Vector vecDir;

	vecDir = m_pPlayer->FireBulletsPlayer(5, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 0, 3, m_pPlayer->pev, m_pPlayer->random_seed);

	int iAnim = RANDOM_LONG( AR2_FIRE1, AR2_FIRE3 );

	MyAnim(iAnim);
	if( !m_pBeam1 )
	{

		m_pBeam1 = CBeam::BeamCreate(AR2_BEAM_SPRITE, 40);
		m_pBeam1->PointEntInit(pev->origin, m_pPlayer->entindex());
		m_pBeam1->SetFlags(BEAM_FSINE);
		m_pBeam1->pev->spawnflags |= SF_BEAM_TEMPORARY;
		m_pBeam1->pev->owner = m_pPlayer->edict();
		m_pBeam1->SetEndAttachment(1);
		m_pBeam1->SetStartPos( gpGlobals->trace_endpos );
		//meam1->SetEndPos(this->pev->origin + pev->view_ofs +gpGlobals->v_up*20+gpGlobals->v_right*5+gpGlobals->v_forward*30);
		m_pBeam1->SetWidth(15);
		m_pBeam1->SetBrightness(255);

		SetThink(&CAR2::Cleaner);
		pev->nextthink = gpGlobals->time + 0.05;
	}
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "ar2s1.wav", 1, ATTN_NORM);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	SetThink(&CAR2::Cleaner);
	
	m_flNextPrimaryAttack = gpGlobals->time + 0.1;

	m_flTimeWeaponIdle = gpGlobals->time  + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void CAR2::Cleaner(void) {

	if( m_pBeam1 )
		UTIL_Remove(m_pBeam1);
	m_pBeam1 = NULL;
	SetThink( NULL );
}

void CAR2::SecondaryAttack(void)
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = gpGlobals->time + 0.15;
		return;
	}

	if (m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0)
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = gpGlobals->time + 0.7;

	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs;
	TraceResult tr;
	Vector forward = gpGlobals->v_forward;
	UTIL_TraceLine( vecSrc, vecSrc + gpGlobals->v_forward * 16, ignore_monsters, ENT( m_pPlayer->pev ), &tr );
	if( tr.flFraction != 1.0 )
	{
		vecSrc = tr.vecEndPos + ( tr.vecPlaneNormal * 15 );
	}
	CAR2Ball::AR2Shoot(m_pPlayer->pev,vecSrc,
		forward * 1600,5);

	// reload sound
	if( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] )
		EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2launch.wav", 0.75, ATTN_NORM);
	MyAnim(AR2_LAUNCH);

	m_flNextPrimaryAttack = gpGlobals->time + 1;
	m_flNextSecondaryAttack = gpGlobals->time + 2;
	m_flTimeWeaponIdle = gpGlobals->time + + 5;// idle pretty soon after shooting.

	if (!m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType])
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
}

void CAR2::Reload(void)
{
	if( cvar_ar2_mp5.value && m_pPlayer->ammo_9mm <= 0 )
		return;

	DefaultReload(30, AR2_RELOAD, 1.0);

}


void CAR2::WeaponIdle(void)
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle >gpGlobals->time)
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = AR2_LONGIDLE;
		break;

	default:
	case 1:
		iAnim = AR2_IDLE1;
		break;
	}

	MyAnim(iAnim);

	m_flTimeWeaponIdle = gpGlobals->time+UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.

}



class CAR2Ammo : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/combine_rifle_cartridge01.mdl");
		pev->angles = Vector( -90, 0, 0 );
		CBasePlayerAmmo::Spawn( );
		UTIL_SetSize( pev, Vector( -16, -16, -3 ), Vector( 16, 16, 13 ) );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/combine_rifle_cartridge01.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = (pOther->GiveAmmo( 30, "AR2", 120) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS( ammo_ar2, CAR2Ammo );


class CAR2AmmoGrenade : public CBasePlayerAmmo
{
	void Spawn( void )
	{
		Precache( );
		SET_MODEL(ENT(pev), "models/combine_rifle_ammo01.mdl");
		CBasePlayerAmmo::Spawn( );


	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/combine_rifle_ammo01.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther )
	{
		int bResult = (pOther->GiveAmmo( 1, "AR2grenades", 3 ) != -1);

		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};

LINK_ENTITY_TO_CLASS( ammo_ar2_altfire, CAR2AmmoGrenade );
