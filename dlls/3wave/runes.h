/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
#pragma once
#ifndef RUNES_H
#define RUNES_H

static const char *g_RuneEntityName[] =
{
	"",
	"item_rune1",
	"item_rune2",
	"item_rune3",
	"item_rune4"
};

static const char *g_RuneName[] =
{
	"Unknown",
	"ResistRune",
	"StrengthRune",
	"HasteRune",
	"RegenRune"
};

class CItemRune : public CBaseEntity
{
private:
	void EXPORT RuneRespawn();
public:
	void EXPORT RuneTouch( CBaseEntity *pOther );
	void Spawn();
	int Classify() { return CLASS_RUNE; };
	void EXPORT MakeTouchable();
	virtual void PrintTouchMessage( CBasePlayer *pPlayer ){}
	virtual void PrintRespawnMessage(){}

	int m_iRuneFlag;
	bool m_bTouchable;
	bool dropped;
};

class CResistRune : public CItemRune
{
public:
	void Spawn();
	void PrintTouchMessage( CBasePlayer *pPlayer );
	void PrintRespawnMessage();
};

class CStrengthRune : public CItemRune
{
public:
	void Spawn();
	void PrintTouchMessage( CBasePlayer *pPlayer ); 
	void PrintRespawnMessage();
}; 

class CHasteRune : public CItemRune
{
public:
	void Spawn();
	void PrintTouchMessage( CBasePlayer *pPlayer ); 
	void PrintRespawnMessage();
};

class CRegenRune : public CItemRune
{
public:
	void Spawn();
	void PrintTouchMessage( CBasePlayer *pPlayer ); 
	void PrintRespawnMessage();
};
#endif // RUNES_H
