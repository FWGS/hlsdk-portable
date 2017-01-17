/***
*
*	Copyright (c) 2001, Valve LLC. All rights reserved.
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
#ifndef CTF_ITEMS_H
#define CTF_ITEMS_H

class CItemCTFBase : public CBaseEntity
{
public:
	virtual void KeyValue(KeyValueData* pkvd);

	int		m_goalNo;
	Vector  m_goalMin, 
		    m_goalMax;
};

//=========================================================
// Flag stand
//=========================================================
class CFlagStand : public CItemCTFBase
{
public:
	void Spawn(void);
	void Precache(void);

	void EXPORT StandThink(void);
	void StandTouch(CBaseEntity *pToucher);

	BOOL HasFlag;
};

//=========================================================
// Civil flag stand
//=========================================================
class CFlagStandTeam1 : public CFlagStand
{
public:
	void Spawn(void);
};

//=========================================================
// Military flag stand
//=========================================================
class CFlagStandTeam2 : public CFlagStand
{
public:
	void Spawn(void);
};




//=========================================================
// Flags (on ground)
//=========================================================
class CItemFlag : public CItemCTFBase
{
public:
	virtual void Spawn(void);

	BOOL Dropped;
	float m_flDroppedTime;

	void EXPORT FlagThink(void);

private:
	void Precache(void);
	void Capture(CBasePlayer *pPlayer, int iTeam);
	void ResetFlag(int iTeam);
	void Materialize(void);
	void EXPORT FlagTouch(CBaseEntity *pOther);
	// BOOL MyTouch( CBasePlayer *pPlayer );

};

class CItemFlagTeam1 : public CItemFlag
{
	void Spawn(void);
};

class CItemFlagTeam2 : public CItemFlag
{
	void Spawn(void);
};


//=========================================================
// Carried Flags
//=========================================================
class CCarriedFlag : public CBaseEntity
{
public:
	virtual void Spawn(void);

	CBasePlayer *Owner;

	int m_iOwnerOldVel;

private:
	void Precache(void);
	void EXPORT FlagThink(void);
};

class CCarriedFlagTeam1 : public CCarriedFlag
{
	void Spawn(void);
};

class CCarriedFlagTeam2 : public CCarriedFlag
{
	void Spawn(void);
};
#endif // CTF_ITEMS_H
