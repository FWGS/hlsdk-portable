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
//=========================================================
// sound.cpp 
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "talkmonster.h"
#include "gamerules.h"
#include "sequence.h"
#include "czeror/cz_seqapi.h"
#include "pm_materials.h"

#include <ctype.h>
#include <stdint.h>

// ==================== SENTENCE GROUPS, UTILITY FUNCTIONS  ======================================

#define CSENTENCE_LRU_MAX	32		// max number of elements per sentence group
#define CSENTENCEG_MAX		500
#define CTEXTURESMAX			512			// max number of textures loaded
#define CBTEXTURENAMEMAX		13

// group of related sentences

typedef struct sentenceg
{
	char			szgroupname[32];
	int				count;
	unsigned char	rgblru[CSENTENCE_LRU_MAX];
} SENTENCEG;
// globals

SENTENCEG rgsentenceg[CSENTENCEG_MAX];
int fSentencesInit;
char gszallsentencenames[CVOXFILESENTENCEMAX][CBSENTENCENAME_MAX];
int gcallsentences;
int fTextureTypeInit;
int gcTextures;
char grgszTextureName[CTEXTURESMAX][CBTEXTURENAMEMAX];	// texture names
char grgchTextureType[CTEXTURESMAX];						// parallel array of texture types

static char* memfgets( byte* file, int size, int& position, char* buffer, int bufferSize )
{
	if ( !file || !buffer || position >= size )
		return NULL;
	int start = position;
	int end = position + bufferSize - 1;
	if ( size - position < bufferSize )
		end = size;
	while ( position < end )
	{
		if ( file[position++] == '\n' )
			break;
	}
	if ( start == position )
		return NULL;
	memcpy( buffer, file + start, position - start );
	buffer[position - start] = 0;
	return buffer;
}

void USENTENCEG_InitLRU( unsigned char* lru, int count )
{
	if ( !fSentencesInit )
		return;
	count = min( count, CSENTENCE_LRU_MAX );
	for ( int i = 0; i < count; i++ )
		lru[i] = (unsigned char)i;
	for ( int i = 0; i < count * 4; i++ )
	{
		int first = RANDOM_LONG( 0, count - 1 );
		int second = RANDOM_LONG( 0, count - 1 );
		unsigned char value = lru[first];
		lru[first] = lru[second];
		lru[second] = value;
	}
}

int USENTENCEG_Pick( int group, char* found )
{
	if ( !fSentencesInit || group < 0 )
		return -1;
	char* groupName = rgsentenceg[group].szgroupname;
	unsigned char count = (unsigned char)rgsentenceg[group].count;
	if ( count > CSENTENCE_LRU_MAX )
		count = CSENTENCE_LRU_MAX;
	unsigned char* lru = rgsentenceg[group].rgblru;
	for ( ;; )
	{
		for ( unsigned char i = 0; i < count; i++ )
		{
			if ( lru[i] != 0xFF )
			{
				unsigned char pick = lru[i];
				lru[i] = 0xFF;
				strcpy( found, "!" );
				strcat( found, groupName );
				char number[8];
				sprintf( number, "%d", pick );
				strcat( found, number );
				return pick;
			}
		}
		USENTENCEG_InitLRU( lru, count );
	}
}

int USENTENCEG_PickSequential( int group, char* found, int pick, int )
{
	if ( !fSentencesInit || group < 0 )
		return -1;
	unsigned char count = (unsigned char)rgsentenceg[group].count;
	if ( !count )
		return -1;
	if ( pick >= count )
		pick = count - 1;
	found[0] = '!';
	strcpy( found + 1, rgsentenceg[group].szgroupname );
	char number[8];
	sprintf( number, "%d", pick );
	strcat( found, number );
	return pick + 1;
}

int USEQUENCE_SENTENCEG_Pick( const char* group, char* found )
{
	int pick = -1;
	sentenceEntry_s* sentence = (sentenceEntry_s *)g_engfuncs.pfnSequencePickSentence( group, 0, &pick );
	if ( !sentence )
	{
		// engines that only stub the sequence API: the sound path of the .seq sentence
		found[0] = 0;
		return CZ_SequencePickSentence( group, found, 64 );
	}
	sprintf( found, "#%d", sentence->index );
	return pick;
}

int SENTENCEG_GetIndex( const char* group )
{
	if ( !fSentencesInit || !group )
		return -1;
	for ( int i = 0; i < CSENTENCEG_MAX && rgsentenceg[i].count; i++ )
	{
		if ( !strcmp( group, rgsentenceg[i].szgroupname ) )
			return i;
	}
	return -1;
}

static void SENTENCEG_CopyName( char* dest, const char* tableEnd, const char* source )
{
	size_t room = (size_t)( tableEnd - dest );
	size_t length = strlen( source );
	if ( length > room - 1 )
		length = room - 1;
	memcpy( dest, source, length );
	dest[length] = 0;
}

int SENTENCEG_PlayRndI( edict_t* entity, int group, float volume, float attenuation, int flags, int pitch )
{
	if ( !fSentencesInit )
		return -1;
	char found[64];
	found[0] = 0;
	int pick = USENTENCEG_Pick( group, found );
	if ( pick > 0 )
		EMIT_SOUND_DYN( entity, CHAN_VOICE, found, volume, attenuation, flags, pitch );
	return pick;
}

int SENTENCEG_PlayRndSz( edict_t* entity, const char* group, float volume, float attenuation, int flags, int pitch )
{
	if ( !fSentencesInit )
		return -1;
	char found[64];
	found[0] = 0;
	int pick = USEQUENCE_SENTENCEG_Pick( group, found );
	if ( pick >= 0 && found[0] )
	{
		EMIT_SOUND_DYN( entity, CHAN_VOICE, found, volume, attenuation, flags, pitch );
		return pick;
	}
	int index = SENTENCEG_GetIndex( group );
	if ( index < 0 )
	{
		ALERT( at_console, "No such sentence group %s\n", group );
		return -1;
	}
	pick = USENTENCEG_Pick( index, found );
	if ( pick >= 0 && found[0] )
		EMIT_SOUND_DYN( entity, CHAN_VOICE, found, volume, attenuation, flags, pitch );
	return pick;
}

int SENTENCEG_PlaySequentialSz( edict_t* entity, const char* group, float volume, float attenuation,
	int flags, int pitch, int pick, int reset )
{
	if ( !fSentencesInit )
		return -1;
	char found[64];
	found[0] = 0;
	int index = SENTENCEG_GetIndex( group );
	if ( index < 0 )
		return -1;
	int next = USENTENCEG_PickSequential( index, found, pick, reset );
	if ( next >= 0 && found[0] )
		EMIT_SOUND_DYN( entity, CHAN_VOICE, found, volume, attenuation, flags, pitch );
	return next;
}

void SENTENCEG_Stop( edict_t* entity, int group, int pick )
{
	if ( !fSentencesInit || group < 0 || pick < 0 )
		return;
	char found[64];
	found[0] = '!';
	strcpy( found + 1, rgsentenceg[group].szgroupname );
	char number[8];
	sprintf( number, "%d", pick );
	strcat( found, number );
	STOP_SOUND( entity, CHAN_VOICE, found );
}

void SENTENCEG_Init()
{
	if ( fSentencesInit )
		return;
	memset( gszallsentencenames, 0, sizeof( gszallsentencenames ) );
	gcallsentences = 0;
	memset( rgsentenceg, 0, sizeof( rgsentenceg ) );
	char buffer[512] = {};
	char previous[64] = {};
	int group = -1;
	int position = 0;
	int size;
	byte* file = g_engfuncs.pfnLoadFileForMe( "sound/sentences.txt", &size );
	if ( !file )
		return;
	while ( memfgets( file, size, position, buffer, 511 ) )
	{
		int start = 0;
		while ( buffer[start] == ' ' || buffer[start] == '\t' )
			start++;
		if ( !buffer[start] || buffer[start] == '/' || !isalpha( buffer[start] ) )
			continue;
		int end = start;
		while ( buffer[end] && buffer[end] != ' ' && buffer[end] != '\t' )
			end++;
		if ( !buffer[end] )
			continue;
		if ( gcallsentences >= CVOXFILESENTENCEMAX )
		{
			ALERT( at_error, "Too many sentences in sentences.txt!\n" );
			break;
		}
		buffer[end] = 0;
		if ( strlen( buffer + start ) >= CBSENTENCENAME_MAX )
			ALERT( at_warning, "Sentence %s longer than %d letters\n", buffer + start, CBSENTENCENAME_MAX - 1 );
		SENTENCEG_CopyName( gszallsentencenames[gcallsentences++],
			(const char*)gszallsentencenames + sizeof( gszallsentencenames ), buffer + start );
		end--;
		if ( end <= start || !isdigit( buffer[end] ) )
			continue;
		while ( end > start && isdigit( buffer[end] ) )
			end--;
		if ( end <= start )
			continue;
		buffer[end + 1] = 0;
		if ( strcmp( previous, buffer + start ) )
		{
			group++;
			if ( group >= CSENTENCEG_MAX )
			{
				ALERT( at_error, "Too many sentence groups in sentences.txt!\n" );
				break;
			}
			SENTENCEG_CopyName( rgsentenceg[group].szgroupname, (const char*)rgsentenceg + sizeof( rgsentenceg ),
				buffer + start );
			rgsentenceg[group].count = 1;
			strncpy( previous, buffer + start, sizeof( previous ) - 1 );
			previous[sizeof( previous ) - 1] = 0;
		}
		else if ( group >= 0 )
			rgsentenceg[group].count++;
	}
	g_engfuncs.pfnFreeFile( file );
	fSentencesInit = TRUE;
	for ( int i = 0; i < CSENTENCEG_MAX && rgsentenceg[i].count; i++ )
		USENTENCEG_InitLRU( rgsentenceg[i].rgblru, rgsentenceg[i].count );
}

// convert sentence (sample) name to !sentencenum, return !sentencenum

int SENTENCEG_Lookup( const char* sample, char* number )
{
	for ( int i = 0; i < gcallsentences; i++ )
	{
		if ( !stricmp( gszallsentencenames[i], sample + 1 ) )
		{
			if ( number )
			{
				strcpy( number, "!" );
				char digits[8];
				sprintf( digits, "%d", i );
				strcat( number, digits );
			}
			return i;
		}
	}
	// sentence name not found!
	return -1;
}

void EMIT_SOUND_DYN( edict_t* entity, int channel, const char* sample, float volume, float attenuation,
	int flags, int pitch )
{
	if ( sample && sample[0] == '!' )
	{
		char number[32];
		if ( SENTENCEG_Lookup( sample, number ) >= 0 )
			EMIT_SOUND_DYN2( entity, channel, number, volume, attenuation, flags, pitch );
		else
			ALERT( at_aiconsole, "Unable to find %s in sentences.txt\n", sample );
	}
	else
		EMIT_SOUND_DYN2( entity, channel, sample, volume, attenuation, flags, pitch );
}

// play a specific sentence over the HEV suit speaker - just pass player entity, and !sentencename

void EMIT_SOUND_SUIT( edict_t* entity, const char* sample )
{
	float volume = CVAR_GET_FLOAT( "suitvolume" );
	int pitch = PITCH_NORM;
	if ( RANDOM_LONG( 0, 1 ) )
		pitch = RANDOM_LONG( 0, 6 ) + 98;
	if ( volume > 0.05 )
		EMIT_SOUND_DYN( entity, CHAN_STATIC, sample, volume, ATTN_NORM, 0, pitch );
}

// play a sentence, randomly selected from the passed in group id, over the HEV suit speaker

void EMIT_GROUPID_SUIT( edict_t* entity, int group )
{
	float volume = CVAR_GET_FLOAT( "suitvolume" );
	int pitch = PITCH_NORM;
	if ( RANDOM_LONG( 0, 1 ) )
		pitch = RANDOM_LONG( 0, 6 ) + 98;
	if ( volume > 0.05 )
		SENTENCEG_PlayRndI( entity, group, volume, ATTN_NORM, 0, pitch );
}

// play a sentence, randomly selected from the passed in groupname

void EMIT_GROUPNAME_SUIT( edict_t* entity, const char* group )
{
	float volume = CVAR_GET_FLOAT( "suitvolume" );
	int pitch = PITCH_NORM;
	if ( RANDOM_LONG( 0, 1 ) )
		pitch = RANDOM_LONG( 0, 6 ) + 98;
	if ( volume > 0.05 )
		SENTENCEG_PlayRndSz( entity, group, volume, ATTN_NORM, 0, pitch );
}

void TEXTURETYPE_Init()
{
	if ( fTextureTypeInit )
		return;
	memset( grgszTextureName, 0, sizeof( grgszTextureName ) );
	memset( grgchTextureType, 0, sizeof( grgchTextureType ) );
	gcTextures = 0;
	char buffer[512] = {};
	int size = 0;
	int position = 0;
	byte* file = g_engfuncs.pfnLoadFileForMe( "sound/materials.txt", &size );
	if ( !file )
		return;
	while ( memfgets( file, size, position, buffer, 511 ) && gcTextures < CTEXTURESMAX )
	{
		int start = 0;
		while ( buffer[start] && isspace( buffer[start] ) )
			start++;
		if ( !buffer[start] || buffer[start] == '/' || !isalpha( buffer[start] ) )
			continue;
		grgchTextureType[gcTextures] = (char)toupper( buffer[start++] );
		while ( buffer[start] && isspace( buffer[start] ) )
			start++;
		if ( !buffer[start] )
			continue;
		int end = start;
		while ( buffer[end] && !isspace( buffer[end] ) )
			end++;
		if ( !buffer[end] )
			continue;
		end = min( end, start + CBTEXTURENAMEMAX - 1 );
		buffer[end] = 0;
		strcpy( grgszTextureName[gcTextures++], buffer + start );
	}
	g_engfuncs.pfnFreeFile( file );
	fTextureTypeInit = TRUE;
}

// given texture name, find texture type
// if not found, return type 'concrete'

// NOTE: this routine should ONLY be called if the 
// current texture under the player changes!

char TEXTURETYPE_Find( char* name )
{
	// CONSIDER: pre-sort texture names and perform faster binary search here

	for ( int i = 0; i < gcTextures; i++ )
	{
		if ( !strnicmp( name, grgszTextureName[i], CBTEXTURENAMEMAX - 1 ) )
			return grgchTextureType[i];
	}
	return CHAR_TEX_CONCRETE;
}

// play a strike sound based on the texture that was hit by the attack traceline.  VecSrc/VecEnd are the
// original traceline endpoints used by the attacker, iBulletType is the type of bullet that hit the texture.
// returns volume of strike instrument (crowbar) to play

float TEXTURETYPE_PlaySound( TraceResult* trace, Vector source, Vector end, int bulletType )
{
	if ( !g_pGameRules->PlayTextureSounds() )
		return 0;
	CBaseEntity* entity = CBaseEntity::Instance( trace->pHit );
	char material = 0;
	if ( entity && entity->Classify() != CLASS_NONE && entity->Classify() != CLASS_MACHINE )
		material = CHAR_TEX_FLESH;
	else
	{
		const char* texture = TRACE_TEXTURE( entity ? entity->edict() : ENT( 0 ), &source.x, &end.x );
		if ( texture )
		{
			if ( texture[0] == '-' || texture[0] == '+' )
				texture += 2;
			if ( texture[0] == '{' || texture[0] == '!' || texture[0] == '~' || texture[0] == ' ' )
				texture++;
			char name[64];
			strcpy( name, texture );
			name[CBTEXTURENAMEMAX - 1] = 0;
			material = TEXTURETYPE_Find( name );
		}
	}
	const char* sounds[5];
	float volume;
	float weaponVolume;
	float attenuation = ATTN_NORM;
	switch ( material )
	{
	case 'A':
		volume = 0.5f;
		weaponVolume = 0.1f;
		sounds[0] = "player/pl_grass1.wav";
		sounds[1] = "player/pl_grass3.wav";
		sounds[2] = "player/pl_grass2.wav";
		sounds[3] = "player/pl_grass4.wav";
		break;
	case CHAR_TEX_DIRT:
		volume = 0.9f;
		weaponVolume = 0.1f;
		sounds[0] = "player/pl_dirt1.wav";
		sounds[1] = "player/pl_dirt2.wav";
		sounds[2] = "player/pl_dirt3.wav";
		break;
	case 'E':
		volume = 0.5f;
		weaponVolume = 0.1f;
		sounds[0] = "player/pl_carpet1.wav";
		sounds[1] = "player/pl_carpet3.wav";
		sounds[2] = "player/pl_carpet2.wav";
		sounds[3] = "player/pl_carpet4.wav";
		break;
	case CHAR_TEX_GRATE:
		volume = 0.9f;
		weaponVolume = 0.5f;
		sounds[0] = "player/pl_grate1.wav";
		sounds[1] = "player/pl_grate4.wav";
		break;
	case 'H':
		volume = 0.8f;
		weaponVolume = 0;
		sounds[0] = "player/pl_slosh1.wav";
		sounds[1] = "player/pl_slosh3.wav";
		sounds[2] = "player/pl_slosh2.wav";
		sounds[3] = "player/pl_slosh4.wav";
		break;
	case 'I':
		volume = 0.5f;
		weaponVolume = 0.1f;
		sounds[0] = "player/pl_tin1.wav";
		sounds[1] = "player/pl_tin3.wav";
		sounds[2] = "player/pl_tin2.wav";
		sounds[3] = "player/pl_tin4.wav";
		break;
	case 'L':
	case CHAR_TEX_SNOW:
		volume = material == 'L' ? 0.6f : 0.7f;
		weaponVolume = 0.4f;
		sounds[0] = "player/pl_snow_light1.wav";
		sounds[1] = "player/pl_snow_light3.wav";
		sounds[2] = "player/pl_snow_light2.wav";
		sounds[3] = "player/pl_snow_light4.wav";
		break;
	case CHAR_TEX_METAL:
		volume = 0.9f;
		weaponVolume = 0.3f;
		sounds[0] = "player/pl_metal1.wav";
		sounds[1] = "player/pl_metal2.wav";
		sounds[2] = "player/pl_metal3.wav";
		sounds[3] = "player/pl_metal4.wav";
		break;
	case CHAR_TEX_GLASS:
	case CHAR_TEX_COMPUTER:
		volume = 0.8f;
		weaponVolume = 0.2f;
		sounds[0] = "debris/glass1.wav";
		sounds[1] = "debris/glass2.wav";
		sounds[2] = "debris/glass3.wav";
		break;
	case 'R':
		volume = 0.9f;
		weaponVolume = 0.1f;
		sounds[0] = "player/pl_gravel1.wav";
		sounds[1] = "player/pl_gravel3.wav";
		sounds[2] = "player/pl_gravel2.wav";
		sounds[3] = "player/pl_gravel4.wav";
		break;
	case CHAR_TEX_SLOSH:
		volume = 0.9f;
		weaponVolume = 0;
		sounds[0] = "player/pl_wade1.wav";
		sounds[1] = "player/pl_wade3.wav";
		sounds[2] = "player/pl_wade2.wav";
		sounds[3] = "player/pl_wade4.wav";
		break;
	case CHAR_TEX_TILE:
		volume = 0.8f;
		weaponVolume = 0.2f;
		sounds[0] = "player/pl_tile1.wav";
		sounds[1] = "player/pl_tile3.wav";
		sounds[2] = "player/pl_tile2.wav";
		sounds[3] = "player/pl_tile4.wav";
		sounds[4] = "player/pl_tile5.wav";
		break;
	case 'U':
		volume = 0.5f;
		weaponVolume = 0.1f;
		sounds[0] = "player/pl_duct1.wav";
		sounds[1] = "player/pl_duct3.wav";
		sounds[2] = "player/pl_duct2.wav";
		sounds[3] = "player/pl_duct4.wav";
		break;
	case CHAR_TEX_VENT:
		volume = 0.5f;
		weaponVolume = 0.3f;
		sounds[0] = sounds[1] = "player/pl_duct1.wav";
		break;
	case CHAR_TEX_WOOD:
		volume = 0.9f;
		weaponVolume = 0.2f;
		sounds[0] = "player/pl_wood1.wav";
		sounds[1] = "player/pl_wood3.wav";
		sounds[2] = "player/pl_wood2.wav";
		sounds[3] = "player/pl_wood4.wav";
		break;
	case 'Z':
		volume = 0.5f;
		weaponVolume = 0.1f;
		sounds[0] = "player/pl_mud1.wav";
		sounds[1] = "player/pl_mud3.wav";
		sounds[2] = "player/pl_mud2.wav";
		sounds[3] = "player/pl_mud4.wav";
		break;
	case CHAR_TEX_FLESH:
		if ( bulletType == BULLET_PLAYER_CROWBAR )
			return 0;
		volume = 1;
		weaponVolume = 0.2f;
		attenuation = 1;
		sounds[0] = "weapons/bullet_hit1.wav";
		sounds[1] = "weapons/bullet_hit2.wav";
		break;
	default:
		volume = 0.9f;
		weaponVolume = 0.6f;
		sounds[0] = "player/pl_crete1.wav";
		sounds[1] = "player/pl_crete3.wav";
		sounds[2] = "player/pl_crete2.wav";
		sounds[3] = "player/pl_crete4.wav";
		break;
	}

	// did we hit a breakable?

	if ( entity && FClassnameIs( entity->pev, "func_breakable" ) )
	{
		volume /= 1.5;
		weaponVolume /= 2.0;
	}
	else if ( material == CHAR_TEX_COMPUTER && trace->flFraction != 1 && RANDOM_LONG( 0, 1 ) )
	{
		UTIL_Sparks( trace->vecEndPos );
		float sparkVolume = RANDOM_FLOAT( 0.7f, 1 );
		switch ( RANDOM_LONG( 0, 1 ) )
		{
		case 0:
			UTIL_EmitAmbientSound( ENT( 0 ), trace->vecEndPos, "buttons/spark5.wav", sparkVolume, ATTN_NORM, 0, 100 );
			break;
		case 1:
			UTIL_EmitAmbientSound( ENT( 0 ), trace->vecEndPos, "buttons/spark6.wav", sparkVolume, ATTN_NORM, 0, 100 );
			break;
		}
	}
	int pick = 0;
	if ( RANDOM_LONG( 0, 0 ) <= 0 && RANDOM_LONG( 0, 0 ) >= 0 )
		pick = RANDOM_LONG( 0, 0 );
	if ( (uintptr_t)sounds[pick] > 2 )
		UTIL_EmitAmbientSound( ENT( 0 ), trace->vecEndPos, sounds[pick], volume, attenuation, 0, 100 );
	return weaponVolume;
}


// =================== ROOM SOUND FX ==========================================

class CEnvSound : public CPointEntity
{
public:
	void KeyValue( KeyValueData* data ) override;
	void Spawn() override;
	void Think() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	float m_flRadius;
	float m_flRoomtype;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( env_sound, CEnvSound );

TYPEDESCRIPTION CEnvSound::m_SaveData[] =
{
	DEFINE_FIELD( CEnvSound, m_flRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvSound, m_flRoomtype, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CEnvSound, CBaseEntity );

void CEnvSound::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "radius" ) )
	{
		m_flRadius = atof( data->szValue );
		data->fHandled = TRUE;
	}
	if ( FStrEq( data->szKeyName, "roomtype" ) )
	{
		m_flRoomtype = atof( data->szValue );
		data->fHandled = TRUE;
	}
}

// returns TRUE if the given sound entity (pev) is in range 
// and can see the given player entity (pevTarget)

BOOL FEnvSoundInRange( entvars_t* pev, entvars_t* target, float* range )
{
	CEnvSound* sound = GetClassPtr( (CEnvSound*)pev );
	Vector start = pev->origin + pev->view_ofs;
	Vector end = target->origin + target->view_ofs;
	TraceResult trace;
	UTIL_TraceLine( start, end, ignore_monsters, ENT( pev ), &trace );
	if ( ( trace.fInOpen && trace.fInWater ) || trace.flFraction != 1 )
		return FALSE;
	float distance = ( trace.vecEndPos - start ).Length();
	if ( distance > sound->m_flRadius )
		return FALSE;
	if ( range )
		*range = distance;
	return TRUE;
}

void CEnvSound::Think()
{
	edict_t* client = FIND_CLIENT_IN_PVS( edict() );
	if ( FNullEnt( client ) )
	{
		pev->nextthink = gpGlobals->time + 0.75;
		return;
	}
	CBasePlayer* player = GetClassPtr( (CBasePlayer*)VARS( client ) );
	float range;
	if ( !FNullEnt( player->m_pentSndLast ) && player->m_pentSndLast == edict() )
	{
		if ( player->m_flSndRoomtype != 0 && player->m_flSndRange != 0 )
		{
			if ( FEnvSoundInRange( pev, VARS( client ), &range ) )
			{
				player->m_flSndRange = range;
				pev->nextthink = gpGlobals->time + 0.25;
				return;
			}
			player->m_flSndRange = 0;
			player->m_flSndRoomtype = 0;
		}
		pev->nextthink = gpGlobals->time + 0.75;
		return;
	}
	if ( FEnvSoundInRange( pev, VARS( client ), &range ) &&
		( range < player->m_flSndRange || player->m_flSndRange == 0 ) )
	{
		player->m_pentSndLast = edict();
		player->m_flSndRoomtype = m_flRoomtype;
		player->m_flSndRange = range;
		MESSAGE_BEGIN( MSG_ONE, SVC_ROOMTYPE, NULL, client );
			WRITE_SHORT( (short)(int)m_flRoomtype );
		MESSAGE_END();
	}
	pev->nextthink = gpGlobals->time + 0.25;
}

void CEnvSound::Spawn()
{
	float time = gpGlobals->time;
	pev->nextthink = time + RANDOM_FLOAT( 0, 0.5f );
}

// ===================================================================================
//
// Speaker class. Used for announcements per level, for door lock/unlock spoken voice. 
//

class CSpeaker : public CBaseEntity
{
public:
	void KeyValue( KeyValueData* data ) override;
	void Spawn() override;
	void Precache() override;
	void EXPORT ToggleUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT SpeakerThink();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override { return 0; }
	static TYPEDESCRIPTION m_SaveData[];
	int m_preset;			// preset number
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( speaker, CSpeaker );

TYPEDESCRIPTION CSpeaker::m_SaveData[] =
{
	DEFINE_FIELD( CSpeaker, m_preset, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CSpeaker, CBaseEntity );

//
// ambient_generic - general-purpose user-defined static sound
//
void CSpeaker::Spawn()
{
	if ( !m_preset && ( !pev->message || !STRING( pev->message )[0] ) )
	{
		ALERT( at_error, "SPEAKER with no Level/Sentence! at: %f, %f, %f\n",
			pev->origin.x, pev->origin.y, pev->origin.z );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink( &CSpeaker::SUB_Remove );
		return;
	}
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SetThink( &CSpeaker::SpeakerThink );
	pev->nextthink = 0;

	// allow on/off switching via 'use' function.

	SetUse( &CSpeaker::ToggleUse );
	Precache();
}

void CSpeaker::Precache()
{
	if ( !( pev->spawnflags & SPEAKER_START_SILENT ) )
	{
		float time = gpGlobals->time;
		pev->nextthink = time + RANDOM_FLOAT( 5, 15 );
	}
}

void CSpeaker::SpeakerThink()
{
	float volume = 0.1 * pev->health;


	// Wait for the talkmonster to finish first.
	if ( gpGlobals->time <= CTalkMonster::g_talkWaitTime )
	{
		pev->nextthink = CTalkMonster::g_talkWaitTime + RANDOM_FLOAT( 5, 10 );
		return;
	}
	const char* sample;
	// go lookup preset text, assign szSoundFile
	switch ( m_preset )
	{
	case 0: sample = STRING( pev->message ); break;
	case 1: sample = "C1A0_"; break;
	case 2: sample = "C1A1_"; break;
	case 3: sample = "C1A2_"; break;
	case 4: sample = "C1A3_"; break;
	case 5: sample = "C1A4_"; break;
	case 6: sample = "C2A1_"; break;
	case 7: sample = "C2A2_"; break;
	case 8: sample = "C2A3_"; break;
	case 9: sample = "C2A4_"; break;
	case 10: sample = "C2A5_"; break;
	case 11: sample = "C3A1_"; break;
	case 12: sample = "C3A2_"; break;
	default:
		pev->nextthink = 0;
		return;
	}
	if ( sample[0] == '!' )
	{
		UTIL_EmitAmbientSound( edict(), pev->origin, sample, volume, 0.3f, 0, PITCH_NORM );
		pev->nextthink = 0;
	}
	else
	{
		if ( SENTENCEG_PlayRndSz( edict(), sample, volume, 0.3f, 0, PITCH_NORM ) < 0 )
			ALERT( at_console, "Level Design Error!\nSPEAKER has bad sentence group name: %s\n", sample );
		float time = gpGlobals->time;
		pev->nextthink = time + RANDOM_FLOAT( 15, 135 );
		CTalkMonster::g_talkWaitTime = gpGlobals->time + 5;		// time delay until it's ok to speak: used so that two NPCs don't talk at once
	}
}

void CSpeaker::ToggleUse( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	BOOL active = pev->nextthink > 0;
	if ( useType != USE_TOGGLE &&
		( ( active && useType == USE_ON ) || ( !active && useType == USE_OFF ) ) )
		return;
	if ( useType == USE_ON )
		pev->nextthink = gpGlobals->time + 0.1;
	else if ( useType == USE_OFF )
		pev->nextthink = 0;
	else if ( active )
		pev->nextthink = 0;
	else
		pev->nextthink = gpGlobals->time + 0.1;
}

// KeyValue - load keyvalue pairs into member data
// NOTE: called BEFORE spawn!

void CSpeaker::KeyValue( KeyValueData* data )
{

	// preset
	if ( FStrEq( data->szKeyName, "preset" ) )
	{
		m_preset = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		data->fHandled = FALSE;
}


// ==================== GENERIC AMBIENT SOUND ======================================

// runtime pitch shift and volume fadein/out structure

// NOTE: IF YOU CHANGE THIS STRUCT YOU MUST CHANGE THE SAVE/RESTORE VERSION NUMBER
// SEE BELOW (in the typedescription for the class)
typedef struct dynpitchvol
{
	// NOTE: do not change the order of these parameters 
	// NOTE: unless you also change order of rgdpvpreset array elements!
	int preset;
	int pitchrun;		// pitch shift % when sound is running 0 - 255
	int pitchstart;		// pitch shift % when sound stops or starts 0 - 255
	int spinup;			// spinup time 0 - 100
	int spindown;		// spindown time 0 - 100
	int volrun;			// volume change % when sound is running 0 - 10
	int volstart;		// volume change % when sound stops or starts 0 - 10
	int fadein;			// volume fade in time 0 - 100
	int fadeout;		// volume fade out time 0 - 100

						// Low Frequency Oscillator
	int lfotype;		// 0) off 1) square 2) triangle 3) random
	int lforate;		// 0 - 1000, how fast lfo osciallates
	int lfomodpitch;	// 0-100 mod of current pitch. 0 is off.
	int lfomodvol;		// 0-100 mod of current volume. 0 is off.
	int cspinup;		// each trigger hit increments counter and spinup pitch
	int cspincount;
	int pitch;
	int spinupsav;
	int spindownsav;
	int pitchfrac;
	int vol;
	int fadeinsav;
	int fadeoutsav;
	int volfrac;
	int lfofrac;
	int lfomult;
} dynpitchvol_t;

#define CDPVPRESETMAX 27

// presets for runtime pitch and vol modulation of ambient sounds

dynpitchvol_t rgdpvpreset[CDPVPRESETMAX] =
{
	{ 1, 255, 75, 95, 95, 10, 1, 50, 95, 0, 0, 0, 0, 0 },
	{ 2, 255, 85, 70, 88, 10, 1, 20, 88, 0, 0, 0, 0, 0 },
	{ 3, 255, 100, 50, 75, 10, 1, 10, 75, 0, 0, 0, 0, 0 },
	{ 4, 100, 100, 0, 0, 10, 1, 90, 90, 0, 0, 0, 0, 0 },
	{ 5, 100, 100, 0, 0, 10, 1, 80, 80, 0, 0, 0, 0, 0 },
	{ 6, 100, 100, 0, 0, 10, 1, 50, 70, 0, 0, 0, 0, 0 },
	{ 7, 100, 100, 0, 0, 5, 1, 40, 50, 1, 50, 0, 10, 0 },
	{ 8, 100, 100, 0, 0, 5, 1, 40, 50, 1, 150, 0, 10, 0 },
	{ 9, 100, 100, 0, 0, 5, 1, 40, 50, 1, 750, 0, 10, 0 },
	{ 10, 128, 100, 50, 75, 10, 1, 30, 40, 2, 8, 20, 0, 0 },
	{ 11, 128, 100, 50, 75, 10, 1, 30, 40, 2, 25, 20, 0, 0 },
	{ 12, 128, 100, 50, 75, 10, 1, 30, 40, 2, 70, 20, 0, 0 },
	{ 13, 50, 50, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0 },
	{ 14, 70, 70, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0 },
	{ 15, 90, 90, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0 },
	{ 16, 120, 120, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0 },
	{ 17, 180, 180, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0 },
	{ 18, 255, 255, 0, 0, 10, 1, 20, 50, 0, 0, 0, 0, 0 },
	{ 19, 200, 75, 90, 90, 10, 1, 50, 90, 2, 100, 20, 0, 0 },
	{ 20, 255, 75, 97, 90, 10, 1, 50, 90, 1, 40, 50, 0, 0 },
	{ 21, 100, 100, 0, 0, 10, 1, 30, 50, 3, 15, 20, 0, 0 },
	{ 22, 160, 160, 0, 0, 10, 1, 50, 50, 3, 500, 25, 0, 0 },
	{ 23, 255, 75, 88, 0, 10, 1, 40, 0, 0, 0, 0, 0, 5 },
	{ 24, 200, 20, 95, 70, 10, 1, 70, 70, 3, 20, 50, 0, 0 },
	{ 25, 180, 100, 50, 60, 10, 1, 40, 60, 2, 90, 100, 100, 0 },
	{ 26, 60, 60, 0, 0, 10, 1, 40, 70, 3, 80, 20, 50, 0 },
	{ 27, 128, 90, 10, 10, 10, 1, 20, 40, 1, 5, 10, 20, 0 }
};

class CAmbientGeneric : public CBaseEntity
{
public:
	void KeyValue( KeyValueData* data ) override;
	void Spawn() override;
	void Precache() override;
	void EXPORT ToggleUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT RampThink();
	void InitModulationParms();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	int ObjectCaps() override { return 0; }
	static TYPEDESCRIPTION m_SaveData[];
	float			m_flAttenuation;		// attenuation value
	dynpitchvol_t	m_dpv;
	BOOL			m_fActive;	// only TRUE when the entity is playing a looping sound
	BOOL			m_fLooping;	// TRUE when the sound played will loop
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( ambient_generic, CAmbientGeneric );

TYPEDESCRIPTION CAmbientGeneric::m_SaveData[] =
{
	DEFINE_FIELD( CAmbientGeneric, m_flAttenuation, FIELD_FLOAT ),
	DEFINE_FIELD( CAmbientGeneric, m_fActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAmbientGeneric, m_fLooping, FIELD_BOOLEAN ),
	DEFINE_ARRAY( CAmbientGeneric, m_dpv, FIELD_CHARACTER, sizeof( dynpitchvol_t ) )
};

IMPLEMENT_SAVERESTORE( CAmbientGeneric, CBaseEntity );

void CAmbientGeneric::Spawn()
{
	if ( pev->spawnflags & AMBIENT_SOUND_EVERYWHERE )
		m_flAttenuation = ATTN_NONE;
	else if ( pev->spawnflags & AMBIENT_SOUND_SMALLRADIUS )
		m_flAttenuation = ATTN_IDLE;
	else if ( pev->spawnflags & AMBIENT_SOUND_MEDIUMRADIUS )
		m_flAttenuation = ATTN_STATIC;
	else if ( pev->spawnflags & AMBIENT_SOUND_LARGERADIUS )
		m_flAttenuation = ATTN_NORM;
	else
		m_flAttenuation = ATTN_STATIC;
	if ( !pev->message || !STRING( pev->message )[0] )
	{
		ALERT( at_error, "EMPTY AMBIENT AT: %f, %f, %f\n", pev->origin.x, pev->origin.y, pev->origin.z );
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink( &CAmbientGeneric::SUB_Remove );
		return;
	}
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	// Set up think function for dynamic modification 
	// of ambient sound's pitch or volume. Don't
	// start thinking yet.

	SetThink( &CAmbientGeneric::RampThink );
	if ( pev->spawnflags & AMBIENT_SOUND_START_SILENT )
	{
		pev->nextthink = 0;
		m_fActive = FALSE;
	}
	else
	{
		pev->nextthink = gpGlobals->time + 0.1;
		m_fActive = TRUE;
	}

	// allow on/off switching via 'use' function.

	SetUse( &CAmbientGeneric::ToggleUse );
	m_fLooping = !( pev->spawnflags & AMBIENT_SOUND_NOT_LOOPING );
	Precache();
}

void CAmbientGeneric::Precache()
{
	char* sample = (char*)STRING( pev->message );
	if ( pev->message && strlen( sample ) > 1 && sample[0] != '!' )
		PRECACHE_SOUND( sample );
	InitModulationParms();
	if ( !( pev->spawnflags & AMBIENT_SOUND_START_SILENT ) && m_fLooping )
		m_fActive = TRUE;
	if ( m_fActive )
	{
		int flags = SND_SPAWNING;
		if ( m_fLooping )
			flags |= 1 << 9;
		UTIL_EmitAmbientSound( edict(), pev->origin, sample, m_dpv.vol * 0.01, m_flAttenuation, flags, m_dpv.pitch );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CAmbientGeneric::InitModulationParms()
{
	float volume = 10.0f * pev->health;
	m_dpv.volrun = (int)volume;
	if ( m_dpv.volrun > 100 )
		m_dpv.volrun = 100;
	if ( m_dpv.volrun < 0 )
		m_dpv.volrun = 0;
	if ( m_dpv.preset && m_dpv.preset <= CDPVPRESETMAX )
	{
		// load preset values
		m_dpv = rgdpvpreset[m_dpv.preset - 1];

		// fixup preset values, just like
		// fixups in KeyValue routine.
		if ( m_dpv.spindown > 0 )
			m_dpv.spindown = ( 101 - m_dpv.spindown ) * 64;
		if ( m_dpv.spinup > 0 )
			m_dpv.spinup = ( 101 - m_dpv.spinup ) * 64;
		m_dpv.volstart *= 10;
		m_dpv.volrun *= 10;
		if ( m_dpv.fadein > 0 )
			m_dpv.fadein = ( 101 - m_dpv.fadein ) * 64;
		if ( m_dpv.fadeout > 0 )
			m_dpv.fadeout = ( 101 - m_dpv.fadeout ) * 64;
		m_dpv.lforate *= 256;
		m_dpv.fadeinsav = m_dpv.fadein;
		m_dpv.fadeoutsav = m_dpv.fadeout;
		m_dpv.spinupsav = m_dpv.spinup;
		m_dpv.spindownsav = m_dpv.spindown;
	}
	m_dpv.fadein = m_dpv.fadeinsav;
	m_dpv.fadeout = 0;
	m_dpv.vol = m_dpv.fadein ? m_dpv.volstart : m_dpv.volrun;
	m_dpv.spinup = m_dpv.spinupsav;
	m_dpv.spindown = 0;
	m_dpv.pitch = m_dpv.spinup ? m_dpv.pitchstart : m_dpv.pitchrun;
	if ( !m_dpv.pitch )
		m_dpv.pitch = PITCH_NORM;
	m_dpv.pitchfrac = m_dpv.pitch << 8;
	m_dpv.volfrac = m_dpv.vol << 8;
	m_dpv.lfofrac = 0;
	m_dpv.lforate = abs( m_dpv.lforate );
	m_dpv.cspincount = 1;
	if ( m_dpv.cspinup )
	{
		m_dpv.pitchrun = m_dpv.pitchstart + ( 255 - m_dpv.pitchstart ) / m_dpv.cspinup;
		if ( m_dpv.pitchrun > 255 )
			m_dpv.pitchrun = 255;
	}
	if ( ( m_dpv.spinupsav || m_dpv.spindownsav || ( m_dpv.lfotype && m_dpv.lfomodpitch ) ) &&
		m_dpv.pitch == PITCH_NORM )
		m_dpv.pitch = PITCH_NORM + 1; // must never send 'no pitch' as first pitch
}

//
// ToggleUse - turns an ambient sound on or off.  If the 
// ambient is a looping sound, mark sound as active (m_fActive)
// if it's playing, innactive if not.  If the sound is not
// a looping sound, never mark it as active.
//
void CAmbientGeneric::ToggleUse( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float value )
{
	const char* sample = STRING( pev->message );
	if ( useType != USE_TOGGLE &&
		( ( m_fActive && useType == USE_ON ) || ( !m_fActive && useType == USE_OFF ) ) )
		return;
	// Directly change pitch if arg passed. Only works if sound is already playing.

	if ( useType == USE_SET && m_fActive )		// Momentary buttons will pass down a float in here
	{
		float fraction = value;
		if ( fraction > 1 )
			fraction = 1;
		if ( fraction < 0 )
			fraction = 0.01f;
		m_dpv.pitch = (int)( fraction * 255 );
		UTIL_EmitAmbientSound( edict(), pev->origin, sample, 0, 0, SND_CHANGE_PITCH, m_dpv.pitch );
		return;
	}

	// Toggle

	// m_fActive is TRUE only if a looping sound is playing.

	if ( m_fActive )
	{ // turn sound off
		if ( m_dpv.cspinup )
		{
			// Don't actually shut off. Each toggle causes
			// incremental spinup to max pitch

			if ( m_dpv.cspincount <= m_dpv.cspinup )
			{

				// start a new spinup
				m_dpv.cspincount++;
				int increment = ( 255 - m_dpv.pitchstart ) / m_dpv.cspinup;
				m_dpv.spinup = m_dpv.spinupsav;
				m_dpv.spindown = 0;
				m_dpv.pitchrun = m_dpv.pitchstart + increment * m_dpv.cspincount;
				if ( m_dpv.pitchrun > 255 )
					m_dpv.pitchrun = 255;
				pev->nextthink = gpGlobals->time + 0.1;
			}
		}
		else
		{
			m_fActive = FALSE;

			// HACKHACK - this makes the code in Precache() work properly after a save/restore
			pev->spawnflags |= AMBIENT_SOUND_START_SILENT;
			if ( m_dpv.spindownsav || m_dpv.fadeoutsav )
			{
				// spin it down (or fade it) before shutoff if spindown is set
				m_dpv.spindown = m_dpv.spindownsav;
				m_dpv.spinup = 0;
				m_dpv.fadeout = m_dpv.fadeoutsav;
				m_dpv.fadein = 0;
				pev->nextthink = gpGlobals->time + 0.1;
			}
			else
				UTIL_EmitAmbientSound( edict(), pev->origin, sample, 0, 0, SND_STOP, 0 );
		}
	}
	else
	{ // turn sound on

		// only toggle if this is a looping sound.  If not looping, each
		// trigger will cause the sound to play.  If the sound is still
		// playing from a previous trigger press, it will be shut off
		// and then restarted.

		if ( m_fLooping )
			m_fActive = TRUE;
		else
			UTIL_EmitAmbientSound( edict(), pev->origin, sample, 0, 0, SND_STOP, 0 );
		InitModulationParms();
		UTIL_EmitAmbientSound( edict(), pev->origin, sample, m_dpv.vol * 0.01, m_flAttenuation, 0, m_dpv.pitch );
		pev->nextthink = gpGlobals->time + 0.1;
	}
}

void CAmbientGeneric::RampThink()
{
	const char* sample = STRING( pev->message );
	int pitch = m_dpv.pitch;
	int volume = m_dpv.vol;
	int flags = 0;
	BOOL changed = FALSE;
	if ( !m_dpv.spinup && !m_dpv.spindown && !m_dpv.fadein && !m_dpv.fadeout && !m_dpv.lfotype )
		return;						// no ramps or lfo, stop thinking

	// ==============
	// pitch envelope
	// ==============
	if ( m_dpv.spinup || m_dpv.spindown )
	{
		int previous = m_dpv.pitchfrac >> 8;
		if ( m_dpv.spinup > 0 )
			m_dpv.pitchfrac += m_dpv.spinup;
		else if ( m_dpv.spindown > 0 )
			m_dpv.pitchfrac -= m_dpv.spindown;
		pitch = m_dpv.pitchfrac >> 8;
		if ( pitch > m_dpv.pitchrun )
		{
			pitch = m_dpv.pitchrun;
			m_dpv.spinup = 0;				// done with ramp up
		}
		if ( pitch < m_dpv.pitchstart )
		{
			m_dpv.spindown = 0;
			UTIL_EmitAmbientSound( edict(), pev->origin, sample, 0, 0, SND_STOP, 0 );
			return;
		}
		if ( pitch > 255 )
			pitch = 255;
		if ( pitch < 1 )
			pitch = 1;
		m_dpv.pitch = pitch;
		changed |= previous != pitch;
		flags |= SND_CHANGE_PITCH;
	}

	// ==================
	// amplitude envelope
	// ==================
	if ( m_dpv.fadein || m_dpv.fadeout )
	{
		int previous = m_dpv.volfrac >> 8;
		if ( m_dpv.fadein > 0 )
			m_dpv.volfrac += m_dpv.fadein;
		else if ( m_dpv.fadeout > 0 )
			m_dpv.volfrac -= m_dpv.fadeout;
		volume = m_dpv.volfrac >> 8;
		if ( volume > m_dpv.volrun )
		{
			volume = m_dpv.volrun;
			m_dpv.fadein = 0;
		}
		if ( volume < m_dpv.volstart )
		{
			m_dpv.fadeout = 0;
			UTIL_EmitAmbientSound( edict(), pev->origin, sample, 0, 0, SND_STOP, 0 );
			return;
		}
		if ( volume > 100 )
			volume = 100;
		if ( volume < 1 )
			volume = 1;
		m_dpv.vol = volume;
		changed |= previous != volume;
		flags |= SND_CHANGE_VOL;
	}

	// ===================
	// pitch/amplitude LFO
	// ===================
	if ( m_dpv.lfotype )
	{
		if ( m_dpv.lfofrac > 0x6FFFFFFF )
			m_dpv.lfofrac = 0;

		// update lfo, lfofrac/255 makes a triangle wave 0-255
		m_dpv.lfofrac += m_dpv.lforate;
		int position = m_dpv.lfofrac >> 8;
		if ( m_dpv.lfofrac < 0 )
		{
			m_dpv.lfofrac = 0;
			m_dpv.lforate = abs( m_dpv.lforate );
			position = 0;
		}
		else if ( position > 255 )
		{
			position = 255;
			m_dpv.lfofrac = 255 << 8;
			m_dpv.lforate = -abs( m_dpv.lforate );
		}
		switch ( m_dpv.lfotype )
		{
		case LFO_SQUARE:
			m_dpv.lfomult = position < 128 ? 255 : 0;
			break;
		case LFO_RANDOM:
			if ( position == 255 )
				m_dpv.lfomult = RANDOM_LONG( 0, 255 );
			break;
		case LFO_TRIANGLE:
		default:
			m_dpv.lfomult = position;
			break;
		}
		if ( m_dpv.lfomodpitch )
		{
			int previous = pitch;
			pitch += ( m_dpv.lfomult - 128 ) * m_dpv.lfomodpitch / 100;
			if ( pitch > 255 )
				pitch = 255;
			if ( pitch < 1 )
				pitch = 1;
			changed |= previous != pitch;
			flags |= SND_CHANGE_PITCH;
		}
		if ( m_dpv.lfomodvol )
		{
			int previous = volume;
			volume += ( m_dpv.lfomult - 128 ) * m_dpv.lfomodvol / 100;
			if ( volume > 100 )
				volume = 100;
			if ( volume < 0 )
				volume = 0;
			changed |= previous != volume;
			flags |= SND_CHANGE_VOL;
		}
	}

	// Send update to playing sound only if we actually changed
	// pitch or volume in this routine.

	if ( flags && changed )
	{
		if ( m_fLooping )
			flags |= 1 << 9;
		if ( pitch == PITCH_NORM )
			pitch = PITCH_NORM + 1; // don't send 'no pitch' !
		UTIL_EmitAmbientSound( edict(), pev->origin, sample, volume * 0.01, m_flAttenuation, flags, pitch );
	}
	pev->nextthink = gpGlobals->time + 0.2;
}

void CAmbientGeneric::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "preset" ) )
		m_dpv.preset = atoi( data->szValue );
	else if ( FStrEq( data->szKeyName, "pitch" ) )
	{
		m_dpv.pitchrun = atoi( data->szValue );
		m_dpv.pitchrun = max( 0, min( 255, m_dpv.pitchrun ) );
	}
	else if ( FStrEq( data->szKeyName, "pitchstart" ) )
	{
		m_dpv.pitchstart = atoi( data->szValue );
		m_dpv.pitchstart = max( 0, min( 255, m_dpv.pitchstart ) );
	}

	// spinup
	else if ( FStrEq( data->szKeyName, "spinup" ) )
	{
		m_dpv.spinup = atoi( data->szValue );
		m_dpv.spinup = max( 0, min( 100, m_dpv.spinup ) );
		if ( m_dpv.spinup > 0 )
			m_dpv.spinup = ( 101 - m_dpv.spinup ) * 64;
		m_dpv.spinupsav = m_dpv.spinup;
	}

	// spindown
	else if ( FStrEq( data->szKeyName, "spindown" ) )
	{
		m_dpv.spindown = atoi( data->szValue );
		m_dpv.spindown = max( 0, min( 100, m_dpv.spindown ) );
		if ( m_dpv.spindown > 0 )
			m_dpv.spindown = ( 101 - m_dpv.spindown ) * 64;
		m_dpv.spindownsav = m_dpv.spindown;
	}
	else if ( FStrEq( data->szKeyName, "volstart" ) )
	{
		m_dpv.volstart = atoi( data->szValue );
		m_dpv.volstart = max( 0, min( 10, m_dpv.volstart ) ) * 10;
	}

	// fadein
	else if ( FStrEq( data->szKeyName, "fadein" ) )
	{
		m_dpv.fadein = atoi( data->szValue );
		m_dpv.fadein = max( 0, min( 100, m_dpv.fadein ) );
		if ( m_dpv.fadein > 0 )
			m_dpv.fadein = ( 101 - m_dpv.fadein ) * 64;
		m_dpv.fadeinsav = m_dpv.fadein;
	}

	// fadeout
	else if ( FStrEq( data->szKeyName, "fadeout" ) )
	{
		m_dpv.fadeout = atoi( data->szValue );
		m_dpv.fadeout = max( 0, min( 100, m_dpv.fadeout ) );
		if ( m_dpv.fadeout > 0 )
			m_dpv.fadeout = ( 101 - m_dpv.fadeout ) * 64;
		m_dpv.fadeoutsav = m_dpv.fadeout;
	}
	else if ( FStrEq( data->szKeyName, "lfotype" ) )
	{
		m_dpv.lfotype = atoi( data->szValue );
		if ( m_dpv.lfotype > 4 )
			m_dpv.lfotype = LFO_TRIANGLE;
	}
	else if ( FStrEq( data->szKeyName, "lforate" ) )
	{
		m_dpv.lforate = atoi( data->szValue );
		m_dpv.lforate = max( 0, min( 1000, m_dpv.lforate ) ) * 256;
	}
	else if ( FStrEq( data->szKeyName, "lfomodpitch" ) )
	{
		m_dpv.lfomodpitch = atoi( data->szValue );
		m_dpv.lfomodpitch = max( 0, min( 100, m_dpv.lfomodpitch ) );
	}
	else if ( FStrEq( data->szKeyName, "lfomodvol" ) )
	{
		m_dpv.lfomodvol = atoi( data->szValue );
		m_dpv.lfomodvol = max( 0, min( 100, m_dpv.lfomodvol ) );
	}
	else if ( FStrEq( data->szKeyName, "cspinup" ) )
	{
		m_dpv.cspinup = atoi( data->szValue );
		m_dpv.cspinup = max( 0, min( 100, m_dpv.cspinup ) );
	}
	else
	{
		data->fHandled = FALSE;
		return;
	}
	data->fHandled = TRUE;
}
