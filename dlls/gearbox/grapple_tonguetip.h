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

#ifndef GRAPPLE_TONGUETIP_H
#define GRAPPLE_TONGUETIP_H

class CGrapple;

//
//
//
class CGrappleTonguetip : public CBaseEntity
{
public:

#ifndef CLIENT_DLL
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];
#endif

	void Spawn(void);
	void FlyThink(void);
	void HitThink(void);
	void TipTouch(CBaseEntity* pOther);
	void PreRemoval(void);

	CGrapple* m_pMyGrappler;

private:
	static CGrappleTonguetip* CreateTip(entvars_t *pevOwner, Vector vecStart, Vector vecVelocity);
	friend class CGrapple;
};

#endif // GRAPPLE_TONGUETIP_H