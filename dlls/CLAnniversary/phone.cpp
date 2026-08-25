#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


#define BOLT_AIR_VELOCITY	900
#define BOLT_WATER_VELOCITY	600
#define TIME_PHONE_COOLDOWN 5

LINK_ENTITY_TO_CLASS(weapon_phone, CPhone);


static const char *DialogSounds[] =
{
	"newsounds/bog_phone1.wav",
	"newsounds/bog_phone2.wav",
	"newsounds/bog_phone3.wav",
	"newsounds/bog_phone4.wav",
	"newsounds/bog_phone5.wav",
	"newsounds/bog_phone6.wav",
	"newsounds/bog_phone7.wav",
};

static const char *DialogTitles[] =
{
	"BOGPHONE1",
	"BOGPHONE2",
	"BOGPHONE3",
	"BOGPHONE4",
	"BOGPHONE5",
	"BOGPHONE6",
	"BOGPHONE7",
};

static const float DialogTimes[] =
{
	7.0,
	5.0,
	5.0,
	6.0,
	7.0,
	5.0,
	6.0
};


class CPhoneMark : public CGrenade
{
	void Spawn(void);
	void Precache(void);
	void EXPORT AmmoStrike(void);
	void EXPORT HealthStrike(void);
	void EXPORT AirStrike(void);
	int  Classify(void);
	void EXPORT ActionThink(void);

	int MarkType;
	int Timer;
	int MarkStep;
	int m_spriteTexture;

public:
	static CPhoneMark *MarkCreate(int type);
};
LINK_ENTITY_TO_CLASS(phonemark, CPhoneMark);

CPhoneMark *CPhoneMark::MarkCreate(int type)
{
	// Create a new entity with CCrossbowBolt private data
	CPhoneMark *pMark = GetClassPtr((CPhoneMark *)NULL);
	pMark->pev->classname = MAKE_STRING("mark");
	pMark->Spawn();
	pMark->MarkType = type;
	pMark->MarkStep = 0;
	pMark->SetThink(&CPhoneMark::ActionThink);
	pMark->pev->nextthink = gpGlobals->time + 12;
	return pMark;
}

void CPhoneMark::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;
	pev->renderamt = 255;
	pev->rendermode = kRenderTransAdd;

	SET_MODEL(ENT(pev), "sprites/laserdot.spr");

	UTIL_SetOrigin(pev, pev->origin);
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
}

void CPhoneMark::AmmoStrike()
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 1024);
	WRITE_SHORT(m_spriteTexture);
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(10); // life
	WRITE_BYTE(40);  // width
	WRITE_BYTE(15);   // noise
	WRITE_BYTE(0);   // r, g, b
	WRITE_BYTE(255);   // r, g, b
	WRITE_BYTE(0);   // r, g, b
	WRITE_BYTE(128);	// brightness
	WRITE_BYTE(15);		// speed
	MESSAGE_END();
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
	CBaseEntity::Create("ammo_buckshot", pev->origin, pev->angles);
	CBaseEntity::Create("ammo_crossbow", pev->origin, pev->angles);
	CBaseEntity::Create("ammo_357", pev->origin, pev->angles);
	CBaseEntity::Create("ammo_9mmAR", pev->origin, pev->angles);
	UTIL_Remove(this);
}

void CPhoneMark::HealthStrike()
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 1024);
	WRITE_SHORT(m_spriteTexture);
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(10); // life
	WRITE_BYTE(100);  // width
	WRITE_BYTE(15);   // noise
	WRITE_BYTE(255);   // r, g, b
	WRITE_BYTE(0);   // r, g, b
	WRITE_BYTE(0);   // r, g, b
	WRITE_BYTE(128);	// brightness
	WRITE_BYTE(15);		// speed
	MESSAGE_END();
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150);
	for (int i = 0; i < 4;i++)
		CBaseEntity::Create("item_healthkit", pev->origin, pev->angles);
	UTIL_Remove(this);
}

void CPhoneMark::AirStrike()
{
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMPOINTS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 1024);
	WRITE_SHORT(m_spriteTexture);
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(0); // framerate
	WRITE_BYTE(1); // life
	WRITE_BYTE(100);  // width
	WRITE_BYTE(0);   // noise
	WRITE_BYTE(255);   // r, g, b
	WRITE_BYTE(160);   // r, g, b
	WRITE_BYTE(100);   // r, g, b
	WRITE_BYTE(128);	// brightness
	WRITE_BYTE(0);		// speed
	MESSAGE_END();
	TraceResult tr;
	UTIL_TraceLine(pev->origin + Vector(0, 0, 1024), pev->origin - Vector(0, 0, 1024), dont_ignore_monsters, ENT(pev), &tr);

	Explode(&tr, DMG_BLAST | DMG_MORTAR);
#ifndef CLIENT_DLL
	UTIL_ScreenShake(tr.vecEndPos, 25.0, 150.0, 1.0, 1500);
#endif

	int pitch = RANDOM_LONG(95, 124);
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/mortarhit.wav", 1.0, 0.55, 0, pitch);

	// ForceSound( SNDRADIUS_MP5, bits_SOUND_COMBAT );

	// ExplodeModel( pev->origin, 400, g_sModelIndexShrapnel, 30 );
	RadiusDamage(pev, VARS(pev->owner), 200, CLASS_NONE, DMG_BLAST);
	UTIL_Remove(this);
}

void CPhoneMark::ActionThink(void)
{

	// play mortar incoming sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/mortar.wav", 1.0, ATTN_NORM, 0, PITCH_NORM);

	switch (MarkType)
	{
	case 0://Ammo
	{
			   SetThink(&CPhoneMark::AmmoStrike);
			   pev->nextthink = gpGlobals->time + 3;
			   break;
	}
	case 1://Health kit
	{
			   SetThink(&CPhoneMark::HealthStrike);
			   pev->nextthink = gpGlobals->time + 3;
			   break;
	}
	case 2://Airstrike
	{
			   SetThink(&CPhoneMark::AirStrike);
			   pev->nextthink = gpGlobals->time + 3;
			   break;
	}
	}
}


void CPhoneMark::Precache()
{
	PRECACHE_MODEL("models/arrow.mdl");
	m_spriteTexture = PRECACHE_MODEL("sprites/lgtning.spr");
}


int	CPhoneMark::Classify(void)
{
	return	CLASS_NONE;
}


enum phone_e {
	PHONE_IDLE = 0,
	PHONE_TALK,
	PHONE_DRAW,
	PHONE_CALL,
	PHONE_HOLSTER,
	PHONE_STOPTALK
};


void CPhone::Spawn()
{
	Precache();
	m_iId = WEAPON_PHONE;
	SET_MODEL(ENT(pev), "models/w_phone.mdl");
	m_iClip = -1;
	IsCalling = false;
	IsTalking = false;
	Type = 0;

	FallInit();// get ready to fall down.
}


void CPhone::Precache(void)
{
	PRECACHE_MODEL("models/v_phone.mdl");
	PRECACHE_MODEL("models/w_phone.mdl");
	PRECACHE_MODEL("models/p_satchel_radio.mdl");
	PRECACHE_SOUND("weapons/mortar.wav");
	PRECACHE_SOUND("items/smallmedkit1.wav");
	PRECACHE_SOUND("items/suitchargeok1.wav");
	PRECACHE_SOUND("weapons/phone_airstrike1.wav");
	PRECACHE_SOUND("weapons/phone_airstrike2.wav");
	PRECACHE_SOUND("weapons/phone_airstrike3.wav");
	PRECACHE_SOUND("weapons/phone_ammo1.wav");
	PRECACHE_SOUND("weapons/phone_ammo2.wav");
	PRECACHE_SOUND("weapons/phone_ammo3.wav");
	PRECACHE_SOUND("weapons/phone_health1.wav");
	PRECACHE_SOUND("weapons/phone_health2.wav");
	PRECACHE_SOUND("weapons/phone_health3.wav");
	PRECACHE_SOUND("weapons/phone_bog_airstrike1.wav");
	PRECACHE_SOUND("weapons/phone_bog_airstrike2.wav");
	PRECACHE_SOUND("weapons/phone_bog_airstrike3.wav");
	PRECACHE_SOUND("weapons/phone_bog_ammo1.wav");
	PRECACHE_SOUND("weapons/phone_bog_ammo2.wav");
	PRECACHE_SOUND("weapons/phone_bog_ammo3.wav");
	PRECACHE_SOUND("weapons/phone_bog_health1.wav");
	PRECACHE_SOUND("weapons/phone_bog_health2.wav");
	PRECACHE_SOUND("weapons/phone_bog_health3.wav");
	PRECACHE_SOUND("weapons/phonecall.wav");
	PRECACHE_SOUND("weapons/mortarhit.wav");
	PRECACHE_MODEL("models/arrow.mdl");
	PRECACHE_MODEL("models/w_weaponbox.mdl");
	PRECACHE_MODEL("models/w_medkit.mdl");

	PRECACHE_SOUND_ARRAY(DialogSounds);

	m_usPhone = PRECACHE_EVENT(1, "events/phone.sc");
}

int CPhone::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 4;
	p->iId = WEAPON_PHONE;
	p->iWeight = CROWBAR_WEIGHT;
	return 1;
}



BOOL CPhone::Deploy()
{
	return DefaultDeploy("models/v_phone.mdl", "models/p_satchel_radio.mdl", PHONE_DRAW, "phone");
}

void CPhone::Holster(int skiplocal /* = 0 */)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim(PHONE_HOLSTER);
#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
	}
#endif
}

#ifndef CLIENT_DLL

LINK_ENTITY_TO_CLASS(phonelaser, CLaserSpot);

void CPhone::UpdateSpot(void)
{
	if (!m_pSpot)
	{
		m_pSpot = CLaserSpot::CreateSpot();
	}

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecSrc + vecAiming * 8192, ignore_monsters, ENT(m_pPlayer->pev), &tr);

	UTIL_SetOrigin(m_pSpot->pev, tr.vecEndPos);
}
#endif

void CPhone::WeaponIdle(void)
{
#ifndef CLIENT_DLL
	UpdateSpot();
#endif
}

void CPhone::PrimaryAttack()
{
#ifndef CLIENT_DLL
	UpdateSpot();
	if (m_pPlayer->phonecalling)
	{
		CBaseEntity *pEnt = NULL;
		const char *PhoneTarget = (char *)STRING(m_pPlayer->phonetarget);
		if ((pEnt = UTIL_FindEntityByTargetname(pEnt, PhoneTarget)) != NULL)
			if (!FStringNull(pEnt->pev->targetname))
				FireTargets(STRING(pEnt->pev->targetname), pEnt, this, USE_TOGGLE, 0);

		IsTalking = true;
		IsCalling = true;
		SendWeaponAnim(PHONE_CALL);
		CurrentDialog = m_pPlayer->phonetalktime;
		m_pPlayer->phonecalling = false;
		SetThink(&CPhone::BogDialog);
		pev->nextthink = gpGlobals->time + 2.0;
		//m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 10.5;
		//m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 10.5;
		return;
	}
	if (IsCalling == false)
	{
		if (UTIL_PointContents(m_pSpot->pev->origin) != CONTENTS_WATER)
		{
			UTIL_Sparks(m_pSpot->pev->origin);
			SetThink(&CPhone::Contact);
			pev->nextthink = gpGlobals->time + 3.5;
			//m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 10.5;
			//m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 10.5;
			SendWeaponAnim(PHONE_CALL);
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/phonecall.wav", 1, ATTN_NORM);
			IsCalling = true;
			IsTalking = true;
			CPhoneMark *PPmark = CPhoneMark::MarkCreate(Type);
			PPmark->pev->origin = m_pSpot->pev->origin;
			Vector lightorigin = m_pSpot->pev->origin + gpGlobals->v_up * 1;

			MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
			WRITE_BYTE(TE_DLIGHT);
			WRITE_COORD(lightorigin.x);	// X
			WRITE_COORD(lightorigin.y);	// Y
			WRITE_COORD(lightorigin.z);	// Z
			WRITE_BYTE(15);		// radius * 0.1
			WRITE_BYTE(255);		// r
			WRITE_BYTE(34);		// g
			WRITE_BYTE(34);		// b
			WRITE_BYTE(150);		// time * 10
			WRITE_BYTE(0);		// decay * 0.1
			MESSAGE_END();
		}
		else
		{
			UTIL_CenterPrintAll("Coordinates cannot be marked underwater.");
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.5;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.5;
		}
	}
#endif
}

void CPhone::ResetCall()
{
	IsCalling = false;

	SendWeaponAnim(PHONE_STOPTALK);
}

void CPhone::Contact()
{
	SendWeaponAnim(PHONE_TALK);
	switch (Type)
	{
	case 0:
	{
			  switch (RANDOM_LONG(0, 2))
			  {
			  case 0:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_ammo1.wav", 1, ATTN_NORM);
				  TalkTime = 2.5;
				  break;
			  case 1:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_ammo2.wav", 1, ATTN_NORM);
				  TalkTime = 2.5;
				  break;
			  case 2:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_ammo3.wav", 1, ATTN_NORM);
				  TalkTime = 2.5;
				  break;
			  }
			  break;
	}
	case 1:
	{
			  switch (RANDOM_LONG(0, 2))
			  {
			  case 0:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_health1.wav", 1, ATTN_NORM);
				  TalkTime = 2.5;
				  break;
			  case 1:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_health2.wav", 1, ATTN_NORM);
				  TalkTime = 3;
				  break;
			  case 2:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_health3.wav", 1, ATTN_NORM);
				  TalkTime = 3;
				  break;
			  }
			  break;
	}
	case 2:
	{
			  switch (RANDOM_LONG(0, 2))
			  {
			  case 0:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_airstrike1.wav", 1, ATTN_NORM);
				  TalkTime = 2.5;
				  break;
			  case 1:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_airstrike2.wav", 1, ATTN_NORM);
				  TalkTime = 2;
				  break;
			  case 2:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_airstrike3.wav", 1, ATTN_NORM);
				  TalkTime = 2.5;
				  break;
			  }
			  break;
	}
	}
	SetThink(&CPhone::BogAnswer);

	pev->nextthink = gpGlobals->time + TalkTime;
}

void CPhone::BogAnswer()
{
	double BogTalkTime = 5;
	switch (Type)
	{
	case 0:
	{
			  switch (RANDOM_LONG(0, 2))
			  {
			  case 0:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_ammo1.wav", 1, ATTN_NORM);
				  BogTalkTime = 3.5;
				  break;
			  case 1:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_ammo2.wav", 1, ATTN_NORM);
				  BogTalkTime = 3.5;
				  break;
			  case 2:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_ammo3.wav", 1, ATTN_NORM);
				  BogTalkTime = 3;
				  break;
			  }
			  break;
	}
	case 1:
	{
			  switch (RANDOM_LONG(0, 2))
			  {
			  case 0:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_health1.wav", 1, ATTN_NORM);
				  BogTalkTime = 3.5;
				  break;
			  case 1:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_health2.wav", 1, ATTN_NORM);
				  BogTalkTime = 3.5;
				  break;
			  case 2:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_health3.wav", 1, ATTN_NORM);
				  BogTalkTime = 3.5;
				  break;
			  }
			  break;
	}
	case 2:
	{
			  switch (RANDOM_LONG(0, 2))
			  {
			  case 0:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_airstrike1.wav", 1, ATTN_NORM);
				  BogTalkTime = 4.5;
				  break;
			  case 1:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_airstrike2.wav", 1, ATTN_NORM);
				  BogTalkTime = 4;
				  break;
			  case 2:
				  EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, "weapons/phone_bog_airstrike3.wav", 1, ATTN_NORM);
				  BogTalkTime = 4.5;
				  break;
			  }
			  break;
	}
	}
	SetThink(&CPhone::BogAction);
	pev->nextthink = gpGlobals->time + BogTalkTime;
}

void CPhone::BogAction()
{
	IsTalking = false;
	SetThink(&CPhone::ResetCall);
	SendWeaponAnim(PHONE_STOPTALK);
	pev->nextthink = gpGlobals->time + 5;
}

void CPhone::BogDialog()
{
	ALERT(at_console, "Current dialog: %d\nPlayer talk dialog: %d\nPhone call dialog time: %d\n", CurrentDialog, m_pPlayer->phonetalkdialog, m_pPlayer->phonetalktime);
	if (CurrentDialog <= 0)
	{
		IsTalking = false;
		SetThink(&CPhone::ResetCall);
		SendWeaponAnim(PHONE_STOPTALK);
		pev->nextthink = gpGlobals->time + 2.0;
	}
	else
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_VOICE, DialogSounds[m_pPlayer->phonetalkdialog], 1, ATTN_NORM);
		#ifndef CLIENT_DLL
		UTIL_ShowMessage(DialogTitles[m_pPlayer->phonetalkdialog], m_pPlayer);
		#endif
		pev->nextthink = gpGlobals->time + DialogTimes[m_pPlayer->phonetalkdialog];
		m_pPlayer->phonetalkdialog++;
		CurrentDialog--;
	}
}


void CPhone::SecondaryAttack()
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;
#ifndef CLIENT_DLL
	UpdateSpot();
#endif
	if (IsTalking == true) return;
	switch (Type)
	{
	case 0:
	{
			  Type = 1;
			  UTIL_CenterPrintAll("CALL REQUEST: Health Kit");
			  break;
	}
	case 1:
	{
			  Type = 2;
			  UTIL_CenterPrintAll("CALL REQUEST: Airstike");
			  break;
	}
	case 2:
	{
			  Type = 0;
			  UTIL_CenterPrintAll("CALL REQUEST: Ammo");
			  break;
	}
	}
}

BOOL CPhone::CanHolster(void)
{
	return !IsTalking;
#ifndef CLIENT_DLL
	if (m_pSpot)
	{
		m_pSpot->Killed(NULL, GIB_NEVER);
		m_pSpot = NULL;
	}
#endif
}