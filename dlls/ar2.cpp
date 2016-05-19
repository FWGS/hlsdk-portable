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

#define AR2_BEAM_SPRITE		"sprites/xbeam1.spr"
#define WEAPON_AR2			19
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
class CAR2Ball: public CGrenade
{
public:
	static CAR2Ball *AR2Shoot(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity, float time);
	void AR2Touch(CBaseEntity *pOther);
	void Spawn();
	virtual float TouchGravGun( CBaseEntity *attacker, int stage )
	{
		pev->owner = attacker->edict();

		if( stage == 3 )
			pev->dmgtime = gpGlobals->time + 5 ;

		if( stage == 2 && gpGlobals->time - pev->dmgtime > 15 )
			pev->dmgtime = gpGlobals->time + 15 ;

		if( stage == 1 )
			pev->dmgtime = gpGlobals->time + 15 ;

		return 1600;
	}
};

LINK_ENTITY_TO_CLASS(ar2grenade, CAR2Ball);
//=========================================================
//=========================================================
int CAR2::SecondaryAmmoIndex(void)
{
	return m_iSecondaryAmmoType;
}

void CAR2::Spawn()
{
	pev->classname = MAKE_STRING("weapon_ar2"); // hack to allow for old names
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
	p->pszAmmo1 = "9mm";
	p->iMaxAmmo1 = _9MM_MAX_CARRY;
	p->pszAmmo2 = "ARgrenades";
	p->iMaxAmmo2 = M203_GRENADE_MAX_CARRY;
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

	vecDir = m_pPlayer->FireBulletsPlayer(5, vecSrc, vecAiming, VECTOR_CONE_3DEGREES, 8192, BULLET_PLAYER_MP5, 0, 15, m_pPlayer->pev, m_pPlayer->random_seed);

	int iAnim;
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		iAnim = AR2_FIRE1;
		break;

	default:
	case 1:
		iAnim = AR2_FIRE2;
		break;
	case 2:
		iAnim = AR2_FIRE3;
		break;
	}

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
	EMIT_SOUND(ENT(pev), CHAN_VOICE, "ar2s1.wav", 1, ATTN_NORM);

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

	CAR2Ball::AR2Shoot(m_pPlayer->pev,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 1600,5);


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
	;

	if (m_pPlayer->ammo_9mm <= 0)
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

void CAR2Ball::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_SLIDEBOX;
	UTIL_SetOrigin(pev, pev->origin);
	SetTouch(&CAR2Ball::AR2Touch);	// Bounce if touched

	SetThink(&CAR2Ball::TumbleThink);
	pev->nextthink = gpGlobals->time + 0.1;
	pev->dmgtime = gpGlobals->time + 99999;
	pev->sequence = RANDOM_LONG(3, 6);
	pev->framerate = 1.0;
	pev->effects = EF_LIGHT;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 256;

	pev->gravity = 0.0005;
	pev->friction = 0;

	SET_MODEL(ENT(pev), "models/ar2grenade.mdl");
	pev->dmg = 30;
	m_fRegisteredSound = FALSE;
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16));
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

	return pGrenade;
}

void CAR2Ball::AR2Touch(CBaseEntity *pOther)
{
	// don't hit the guy that launched this sphere
	if (pOther->edict() == pev->owner)
		return;

	if( ( pev->velocity.Length() >= 500 ) && (pev->dmgtime - gpGlobals->time > 5 ) )
	{
		ALERT( at_console, "Slow detonate\n");
		pev->dmgtime = gpGlobals->time + 5 ;
	}

	if( pev->velocity.Length() >= 100 )
	{
		ALERT( at_console, "Decreasing dmgtime %f\n", pev->dmg - gpGlobals->time );
		pev->dmgtime -= pev->velocity.Length() / 5000;
	}

	// only do damage if we're moving fairly fast
	if (m_flNextAttack < gpGlobals->time )
	{
		entvars_t *pevOwner = VARS(pev->owner);
		if( !pevOwner )
			pevOwner = pev;
		TraceResult tr = UTIL_GetGlobalTrace();
		ClearMultiDamage();
		pOther->TraceAttack(pevOwner, 250, gpGlobals->v_forward, &tr, DMG_CLUB);
		if( pOther->IsPlayer() || pOther->IsMoving() )
			pev->velocity = gpGlobals->v_forward.Normalize() * 1600;
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

	{
		// play bounce sound
		switch (RANDOM_LONG(0, 2))
		{
		case 0:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit1.wav", 0.25, ATTN_NORM);	break;
		case 1:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit2.wav", 0.25, ATTN_NORM);	break;
		case 2:	EMIT_SOUND(ENT(pev), CHAN_VOICE, "weapons/grenade_hit3.wav", 0.25, ATTN_NORM);	break;
		}
	}
	pev->framerate = pev->velocity.Length() / 200.0;

}
