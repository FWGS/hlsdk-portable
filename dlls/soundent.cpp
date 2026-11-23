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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "soundent.h"

LINK_ENTITY_TO_CLASS( soundent, CSoundEnt );

CSoundEnt* pSoundEnt;

//=========================================================
// CSound - Clear - zeros all fields for a sound
//=========================================================
void CSound::Clear()
{
	m_vecOrigin = g_vecZero;
	m_iType = bits_SOUND_NONE;
	m_iVolume = 0;
	m_flExpireTime = 0;
	m_iNext = SOUNDLIST_EMPTY;
	m_iNextAudible = 0;
}

//=========================================================
// Reset - clears the volume, origin, and type for a sound,
// but doesn't expire or unlink it. 
//=========================================================
void CSound::Reset()
{
	m_vecOrigin = g_vecZero;
	m_iType = bits_SOUND_NONE;
	m_iVolume = 0;
	m_iNext = SOUNDLIST_EMPTY;
}

BOOL CSound::FIsSound()
{
	return ( m_iType & ( bits_SOUND_COMBAT | bits_SOUND_WORLD | bits_SOUND_PLAYER | bits_SOUND_DANGER ) ) != 0;
}

BOOL CSound::FIsScent()
{
	return ( m_iType & ( bits_SOUND_CARCASS | bits_SOUND_MEAT | bits_SOUND_GARBAGE ) ) != 0;
}

//=========================================================
// Spawn 
//=========================================================
void CSoundEnt::Spawn()
{
	pev->solid = SOLID_NOT;
	Initialize();
	pev->nextthink = gpGlobals->time + 1;
}

void CSoundEnt::Precache()
{
}

void CSoundEnt::Think()
{
	pev->nextthink = gpGlobals->time + 0.3; // how often to check the sound list.
	int previous = SOUNDLIST_EMPTY;
	int sound = m_iActiveSound;
	while ( sound != SOUNDLIST_EMPTY )
	{
		if ( m_SoundPool[sound].m_flExpireTime <= gpGlobals->time &&
			m_SoundPool[sound].m_flExpireTime != SOUND_NEVER_EXPIRE )
		{
			int next = m_SoundPool[sound].m_iNext;
			FreeSound( sound, previous );
			sound = next;
		}
		else
		{
			previous = sound;
			sound = m_SoundPool[sound].m_iNext;
		}
	}
	if ( m_fShowReport )
	{
		ALERT( at_aiconsole, "Soundlist: %d / %d  (%d)\n", ISoundsInList( SOUNDLISTTYPE_ACTIVE ),
			ISoundsInList( SOUNDLISTTYPE_FREE ), ISoundsInList( SOUNDLISTTYPE_ACTIVE ) - m_cLastActiveSounds );
		m_cLastActiveSounds = ISoundsInList( SOUNDLISTTYPE_ACTIVE );
	}
}

void CSoundEnt::Initialize()
{
	m_iFreeSound = 0;
	m_iActiveSound = SOUNDLIST_EMPTY;
	for ( int i = 0; i < MAX_WORLD_SOUNDS; i++ )
	{ // clear all sounds, and link them into the free sound list.
		m_SoundPool[i].Clear();
		m_SoundPool[i].m_iNext = i + 1;
	}
	m_SoundPool[MAX_WORLD_SOUNDS - 1].m_iNext = SOUNDLIST_EMPTY;
	for ( int i = 0; i < gpGlobals->maxClients; i++ )
	{
		int sound = pSoundEnt->IAllocSound();
		if ( sound == SOUNDLIST_EMPTY )
		{
			ALERT( at_console, "Could not AllocSound() for Client Reserve! (DLL)\n" );
			return;
		}
		pSoundEnt->m_SoundPool[sound].m_flExpireTime = SOUND_NEVER_EXPIRE;
	}
	m_fShowReport = CVAR_GET_FLOAT( "displaysoundlist" ) == 1;
}

int CSoundEnt::IAllocSound()
{
	int sound = m_iFreeSound;
	if ( sound == SOUNDLIST_EMPTY )
	{
		// no free sound!
		ALERT( at_console, "Free Sound List is full!\n" );
		return SOUNDLIST_EMPTY;
	}
	m_iFreeSound = m_SoundPool[sound].m_iNext;
	m_SoundPool[sound].m_iNext = m_iActiveSound;
	m_iActiveSound = sound;
	return sound;
}

//=========================================================
// InsertSound - Allocates a free sound and fills it with 
// sound info.
//=========================================================
void CSoundEnt::InsertSound( int type, const Vector& origin, int volume, float duration )
{
	if ( !pSoundEnt )
		return;
	int sound = pSoundEnt->IAllocSound();
	if ( sound == SOUNDLIST_EMPTY )
	{
		ALERT( at_console, "Could not AllocSound() for InsertSound() (DLL)\n" );
		return;
	}
	pSoundEnt->m_SoundPool[sound].m_vecOrigin = origin;
	pSoundEnt->m_SoundPool[sound].m_iType = type;
	pSoundEnt->m_SoundPool[sound].m_iVolume = volume;
	pSoundEnt->m_SoundPool[sound].m_flExpireTime = gpGlobals->time + duration;
}

void CSoundEnt::FreeSound( int sound, int previous )
{
	if ( pSoundEnt )
	{
		if ( previous == SOUNDLIST_EMPTY )
			pSoundEnt->m_iActiveSound = pSoundEnt->m_SoundPool[sound].m_iNext;
		else
			pSoundEnt->m_SoundPool[previous].m_iNext = pSoundEnt->m_SoundPool[sound].m_iNext;
		pSoundEnt->m_SoundPool[sound].m_iNext = pSoundEnt->m_iFreeSound;
		pSoundEnt->m_iFreeSound = sound;
	}
}

int CSoundEnt::ActiveList()
{
	return pSoundEnt ? pSoundEnt->m_iActiveSound : SOUNDLIST_EMPTY;
}

int CSoundEnt::FreeList()
{
	return pSoundEnt ? pSoundEnt->m_iFreeSound : SOUNDLIST_EMPTY;
}

//=========================================================
// SoundPointerForIndex - returns a pointer to the instance
// of CSound at index's position in the sound pool.
//=========================================================
CSound* CSoundEnt::SoundPointerForIndex( int index )
{
	if ( !pSoundEnt )
		return NULL;
	if ( index >= MAX_WORLD_SOUNDS )
	{
		ALERT( at_console, "SoundPointerForIndex() - Index too large!\n" );
		return NULL;
	}
	if ( index < 0 )
	{
		ALERT( at_console, "SoundPointerForIndex() - Index < 0!\n" );
		return NULL;
	}
	return &pSoundEnt->m_SoundPool[index];
}

int CSoundEnt::ClientSoundIndex( edict_t* client )
{
	return ENTINDEX( client ) - 1;
}

int CSoundEnt::ISoundsInList( int listType )
{
	int sound;
	if ( listType == SOUNDLISTTYPE_FREE )
		sound = m_iFreeSound;
	else if ( listType == SOUNDLISTTYPE_ACTIVE )
		sound = m_iActiveSound;
	else
	{
		ALERT( at_console, "Unknown Sound List Type!\n" );
		return 0;
	}
	int count = 0;
	while ( sound != SOUNDLIST_EMPTY )
	{
		count++;
		sound = m_SoundPool[sound].m_iNext;
	}
	return count;
}
