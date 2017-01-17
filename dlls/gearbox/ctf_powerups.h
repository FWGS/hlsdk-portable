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
#ifndef CTF_POWERUPS_H
#define CTF_POWERUPS_H

#define CTF_POWERUP_SPAWN_ENTITY_CLASSNAME		"info_ctfspawn_powerup"

#define IT_CTF_ACCELERATOR_CLASSNAME			"item_ctfaccelerator"
#define IT_CTF_BACKPACK_CLASSNAME				"item_ctfbackpack"
#define IT_CTF_LONGJUMP_CLASSNAME				"item_ctflongjump"
#define IT_CTF_PORTABLEHEV_CLASSNAME			"item_ctfportablehev"
#define IT_CTF_REGEN_CLASSNAME					"item_ctfregeneration"

#define IT_CTF_ACCELERATOR_FLAG					(1 << 0)
#define IT_CTF_BACKPACK_FLAG					(1 << 1)
#define IT_CTF_LONGJUMP_FLAG					(1 << 2)
#define IT_CTF_PORTABLEHEV_FLAG					(1 << 3)
#define IT_CTF_REGEN_FLAG						(1 << 4)

class CPowerupCTFBase : public CBaseEntity
{
	void EXPORT  RuneRespawn(void);
public:

	virtual const char*		GetRuneModel() const	{ return NULL; }
	virtual int				GetRuneFlag()  const	{ return -1; }

	virtual void KeyValue(KeyValueData* pkvd);
	virtual void Spawn(void);

	void EXPORT  MakeTouchable(void);
	void EXPORT  RuneTouch(CBaseEntity *pOther);


	// Keyvalues
	int m_teamNo;

	// Attributes
	int  m_iRuneFlag;
	bool m_bTouchable;
	bool dropped;
	string_t m_iszPrintName;
};

class CPowerupAccelerator : public CPowerupCTFBase
{
public:
	const char* GetRuneModel(void) const	{ return "models/w_accelerator.mdl"; }
	int			GetRuneFlag() const			{ return IT_CTF_ACCELERATOR_FLAG; }

	void Spawn(void) 
	{
		CPowerupCTFBase::Spawn();
		m_iszPrintName = MAKE_STRING("Accelerator");
	}
};

class CPowerupBackpack : public CPowerupCTFBase
{
public:
	const char* GetRuneModel(void) const	{ return "models/w_backpack.mdl"; }
	int			GetRuneFlag() const			{ return IT_CTF_BACKPACK_FLAG; }

	void Spawn(void)
	{
		CPowerupCTFBase::Spawn();
		m_iszPrintName = MAKE_STRING("Backpack");
	}
};

class CPowerupRegen : public CPowerupCTFBase
{
public:
	const char* GetRuneModel(void) const	{ return "models/w_health.mdl"; }
	int			GetRuneFlag() const			{ return IT_CTF_REGEN_FLAG; }

	void Spawn(void)
	{
		CPowerupCTFBase::Spawn();
		m_iszPrintName = MAKE_STRING("Regeneration");
	}
};

class CPowerupJumppack : public CPowerupCTFBase
{
public:
	const char* GetRuneModel(void) const	{ return "models/w_jumppack.mdl"; }
	int			GetRuneFlag() const			{ return IT_CTF_LONGJUMP_FLAG; }

	void Spawn(void)
	{
		CPowerupCTFBase::Spawn();
		m_iszPrintName = MAKE_STRING("Long jump");
	}
};

class CPowerupPorthev : public CPowerupCTFBase
{
public:
	const char* GetRuneModel(void) const	{ return "models/w_porthev.mdl"; }
	int			GetRuneFlag() const			{ return IT_CTF_PORTABLEHEV_FLAG; }

	void Spawn(void)
	{
		CPowerupCTFBase::Spawn();
		m_iszPrintName = MAKE_STRING("Portable HEV");
	}
};
#endif // CTF_POWERUPS_H
