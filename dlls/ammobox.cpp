#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"

// Ammobox entity by XaeroX

class CAmmoBox : public CBasePlayerAmmo
{
	typedef CBasePlayerAmmo BaseClass;
	typedef struct {
		const char* ammoName;
		int			defaultGive;
		int			maxCarry;
	} LocalAmmoType;
public:
	virtual void Spawn();
	virtual void Precache();
	virtual void KeyValue(KeyValueData* pkvd);
	virtual BOOL AddAmmo(CBaseEntity* pOther);
private:
	static const LocalAmmoType AmmoTypes[];
	static const size_t AmmoTypeCount;
};

LINK_ENTITY_TO_CLASS(ammobox, CAmmoBox);

const CAmmoBox::LocalAmmoType CAmmoBox::AmmoTypes[] = {
	/* ammo name	default give count		max. carry */
	{ "9mm",		AMMO_GLOCKCLIP_GIVE,	_9MM_MAX_CARRY },			// FGD type 0
	{ "357",		AMMO_357BOX_GIVE,		_357_MAX_CARRY },			// FGD type 1
	{ "buckshot",	AMMO_BUCKSHOTBOX_GIVE,	BUCKSHOT_MAX_CARRY },		// FGD type 2
	{ "bolts",		AMMO_CROSSBOWCLIP_GIVE, BOLT_MAX_CARRY },			// FGD type 3
	{ "rockets",	AMMO_RPGCLIP_GIVE,		ROCKET_MAX_CARRY },			// FGD type 4
	{ "uranium",	AMMO_URANIUMBOX_GIVE,	URANIUM_MAX_CARRY },		// FGD type 5
	{ "ARgrenades",	AMMO_M203BOX_GIVE,		M203_GRENADE_MAX_CARRY },	// FGD type 6
	{ "44",			AMMO_DEAGLECLIP_GIVE,	_44_MAX_CARRY },			// FGD type 7
	{ "45ACP",		AMMO_45ACPCLIP_GIVE,	_45ACP_MAX_CARRY },			// FGD type 8
	{ "556mm",		AMMO_MP5CLIP_GIVE,		_556MM_MAX_CARRY },			// FGD type 9
	{ "Hand Grenade",		HANDGRENADE_DEFAULT_GIVE,		HANDGRENADE_MAX_CARRY },	// FGD type 10
	{ "14mm",		SNIPERRIFLE_DEFAULT_GIVE,		_14MM_MAX_CARRY },			// FGD type 11
	// add more ammo types here...
};
const size_t CAmmoBox::AmmoTypeCount = sizeof(CAmmoBox::AmmoTypes) / sizeof(CAmmoBox::LocalAmmoType);

void CAmmoBox::Spawn()
{
	// make sure model is assigned
	if (FStringNull(pev->model))
		pev->model = MAKE_STRING("models/w_9mmclip.mdl");

	Precache();

	// set the ammo model
	SET_MODEL(ENT(pev), STRING(pev->model));

	// check ammo type
	if (pev->weapons >= AmmoTypeCount)
		pev->weapons = AmmoTypeCount - 1;
	if (pev->weapons < 0)
		pev->weapons = 0;

	// check ammo count
	// negative or zero means default give value
	if (pev->impulse <= 0)
		pev->impulse = AmmoTypes[pev->weapons].defaultGive;

	// spawn base player ammo
	BaseClass::Spawn();
}

void CAmmoBox::Precache()
{
	// before precaching, make sure model name is not empty
	if (!FStringNull(pev->model))
		PRECACHE_MODEL(const_cast<char*>(STRING(pev->model)));

	// before precaching, make sure sound name is not empty
	if (!FStringNull(pev->noise))
		PRECACHE_SOUND(const_cast<char*>(STRING(pev->noise)));
}

void CAmmoBox::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "ammotype")) {
		// parse ammo type (integer)
		pev->weapons = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "ammocount")) {
		// parse ammo count (integer)
		pev->impulse = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else {
		BaseClass::KeyValue(pkvd);
	}
}

BOOL CAmmoBox::AddAmmo(CBaseEntity* pOther)
{
	if (pOther->GiveAmmo(pev->impulse, const_cast<char*>(AmmoTypes[pev->weapons].ammoName), AmmoTypes[pev->weapons].maxCarry) != -1) {
		// play custom pickup sound
		if (!FStringNull(pev->noise))
			EMIT_SOUND(ENT(pev), CHAN_ITEM, STRING(pev->noise), 1, ATTN_NORM);
		// we've got the ammo
		return TRUE;
	}
	// we've ignored the ammo
	return FALSE;
}
