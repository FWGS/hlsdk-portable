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

#ifndef XEN_H
#define XEN_H

class CActAnimating : public CBaseAnimating
{
public:
	void			SetActivity(Activity act);
	inline Activity	GetActivity(void) { return m_Activity; }

	virtual int	ObjectCaps(void) { return CBaseAnimating::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	virtual int	Save(CSave &save);
	virtual int	Restore(CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

private:
	Activity	m_Activity;
};

#endif // XEN_H