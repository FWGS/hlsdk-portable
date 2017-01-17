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
#ifndef GEARBOX_PLAYER_H
#define GEARBOX_PLAYER_H

enum Player_Menu {
	Team_Menu,
	Team_Menu_IG,
};

#define		PFLAG_ONROPE		( 1 << 6 )

class CGearboxPlayer : public CBasePlayer
{
public:
	virtual int		Save(CSave &save);
	virtual int		Restore(CRestore &restore);

	BOOL	FlashlightIsOn(void);
	void	FlashlightTurnOn(void);
	void	FlashlightTurnOff(void);

	static	TYPEDESCRIPTION m_playerSaveData[];

	virtual void Spawn(void);

private:
	BOOL	m_fInXen;
	BOOL	m_fIsFrozen;

public:
	int		m_bHasFlag;
	void ShowMenu(int bitsValidSlots, int nDisplayTime, BOOL fNeedMore, char *pszText);
	int     m_iMenu;

	float	m_flNextTeamChange;

	CBasePlayer *pFlagCarrierKiller;
	CBasePlayer *pFlagReturner;
	CBasePlayer *pCarrierHurter;

	float	m_flCarrierHurtTime;
	float	m_flCarrierPickupTime;
	float	m_flFlagCarrierKillTime;
	float	m_flFlagReturnTime;
	float	m_flFlagStatusTime;

	float	m_flRegenTime;

	int		m_iRuneStatus;

	void	W_FireHook(void);
	void	Throw_Grapple(void);

	bool	m_bHook_Out;
	bool    m_bOn_Hook;
	CBaseEntity *m_ppHook;

	void Service_Grapple(void);
private:

	friend class CDisplacer;
	friend class CTriggerXenReturn;
	friend class CPlayerFreeze;
};

#endif // GEARBOX_PLAYER_H
