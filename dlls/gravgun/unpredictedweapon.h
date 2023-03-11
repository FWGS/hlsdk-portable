#ifndef UNPREDICTEDWEAPON_H
#define UNPREDICTEDWEAPON_H

#include "weapons.h"

class CBasePlayerWeaponU: public CBasePlayerWeapon
{
public:
	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];
	BOOL UseDecrement( void );
	float UTIL_WeaponTimeBase();
};

#endif // UNPREDICTEDWEAPON_H

