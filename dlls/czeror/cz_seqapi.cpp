/*
cz_seqapi.cpp - the sequence engine API for engines that only stub it
Copyright (C) 2026 sergiopoverony

GoldSrc parses sequences/<map>.seq and sequences/global.seq itself and gives
the game trigger_sequence entries (pfnSequenceGet) and sentences of the
"!sentences" blocks (pfnSequencePickSentence). Xash3D FWGS only has stubs for
both, they return NULL. Then the entries are built here from the same files,
in the structures of sequence.h, and the sentences give the game the path of
the sound instead of the engine's sentence index.
*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "sequence.h"
#include "cz_sequence.h"
#include "cz_seqapi.h"

#include <string>
#include <deque>
#include <map>
#include <vector>

static czseq::Library g_seqLib;
static std::string g_seqMap;

// built entries live until the map changes, trigger_sequence keeps pointers to them
static std::deque<std::string> g_seqStrings;
static std::deque<sequenceCommandLine_s> g_seqLines;
static std::deque<sequenceEntry_s> g_seqEntries;
static std::map<std::string, sequenceEntry_s *> g_seqBuilt;

// "#KEY" texts come from resource/<game>_english.txt ("lang" { "Tokens" { "KEY" "text" } })
static std::map<std::string, std::string> g_seqLoc;
static bool g_seqLocLoaded = false;

static void SeqApi_LoadFile( const char *name )
{
	if( !name || !name[0] )
		return;

	std::string base = name;
	if( base.size() > 4 && czseq::ToLower( base.substr( base.size() - 4 )) == ".seq" )
		base.erase( base.size() - 4 );

	if( g_seqLib.IsFileLoaded( base ))
		return;

	// the shared macros (TitleStyle, SubtitleStyle, ...) live in global.seq
	if( czseq::ToLower( base ) != "global" )
		SeqApi_LoadFile( "global" );

	char path[256];
	snprintf( path, sizeof( path ), "sequences/%s.seq", base.c_str());

	int length = 0;
	byte *data = LOAD_FILE_FOR_ME( path, &length );
	if( !data )
	{
		ALERT( at_aiconsole, "sequence: can't open %s\n", path );
		return;
	}

	int count = g_seqLib.ParseText( (const char *)data, (size_t)length, base );
	FREE_FILE( data );

	ALERT( at_aiconsole, "sequence: %s: %d entries\n", path, count );
}

static void SeqApi_Clear( void )
{
	g_seqLib.Clear();
	g_seqBuilt.clear();
	g_seqEntries.clear();
	g_seqLines.clear();
	g_seqStrings.clear();
	g_seqMap.clear();
}

void CZ_SequenceReset( void )
{
	// the entities that pointed to the old entries are gone. A restart of the same
	// map is a new server too: the sounds of the entries must be precached again,
	// with the old entries kept they were not and the dialogs stayed silent
	SeqApi_Clear();
}

static void SeqApi_EnsureMap( void )
{
	const char *map = STRING( gpGlobals->mapname );

	if( g_seqMap != map )
	{
		SeqApi_Clear();
		g_seqMap = map;
	}

	SeqApi_LoadFile( map );
}

static void SeqApi_LoadLocalization( void )
{
	static const char *files[] = { "resource/czeror_english.txt", "resource/cstrike_english.txt" };

	g_seqLocLoaded = true;

	for( size_t f = 0; f < ARRAYSIZE( files ); f++ )
	{
		int length = 0;
		byte *data = LOAD_FILE_FOR_ME( files[f], &length );
		if( !data )
			continue;

		// the localization files of the game are UTF-16LE with a byte order mark
		std::string text;
		if( length >= 2 && data[0] == 0xFF && data[1] == 0xFE )
		{
			for( int i = 2; i + 1 < length; i += 2 )
			{
				unsigned int c = data[i] | ( data[i + 1] << 8 );

				if( c < 0x80 )
				{
					text += (char)c;
				}
				else if( c < 0x800 )
				{
					text += (char)( 0xC0 | ( c >> 6 ));
					text += (char)( 0x80 | ( c & 0x3F ));
				}
				else
				{
					text += (char)( 0xE0 | ( c >> 12 ));
					text += (char)( 0x80 | (( c >> 6 ) & 0x3F ));
					text += (char)( 0x80 | ( c & 0x3F ));
				}
			}
		}
		else
		{
			text.assign( (const char *)data, length );
		}

		FREE_FILE( data );

		const char *p = text.c_str();
		const char *end = p + text.size();
		std::vector<std::string> tokens;

		// "quoted strings" (may span lines), { }, // comments
		while( p < end )
		{
			if( *p == '/' && p + 1 < end && p[1] == '/' )
			{
				while( p < end && *p != '\n' )
					p++;
			}
			else if( *p == '"' )
			{
				std::string token;

				for( p++; p < end && *p != '"'; p++ )
				{
					if( *p != '\r' )
						token += *p;
				}

				p++;
				tokens.push_back( token );
			}
			else if( *p == '{' || *p == '}' )
			{
				tokens.push_back( std::string( 1, *p ));
				p++;
			}
			else
			{
				p++;
			}
		}

		bool inTokens = false;
		for( size_t i = 0; i + 1 < tokens.size(); )
		{
			if( !inTokens )
			{
				if( czseq::ToLower( tokens[i] ) == "tokens" )
					inTokens = true;
				i++;
				continue;
			}

			if( tokens[i] == "{" )
			{
				i++;
				continue;
			}

			if( tokens[i] == "}" )
				break;

			std::string key = czseq::ToLower( tokens[i] );
			if( g_seqLoc.find( key ) == g_seqLoc.end())
				g_seqLoc[key] = tokens[i + 1];
			i += 2;
		}
	}

	ALERT( at_aiconsole, "sequence: %d localized strings\n", (int)g_seqLoc.size());
}

static const char *SeqApi_Localize( const std::string &text )
{
	if( text.empty() || text[0] != '#' )
		return text.c_str();

	if( !g_seqLocLoaded )
		SeqApi_LoadLocalization();

	std::map<std::string, std::string>::const_iterator it = g_seqLoc.find( czseq::ToLower( text.substr( 1 )));
	if( it != g_seqLoc.end())
		return it->second.c_str();

	return text.c_str();
}

static char *SeqApi_String( const std::string &s )
{
	g_seqStrings.push_back( s );
	return &g_seqStrings.back()[0];
}

static std::string SeqApi_WithWav( const std::string &name )
{
	size_t dot = name.rfind( '.' );
	size_t slash = name.rfind( '/' );

	if( dot != std::string::npos && ( slash == std::string::npos || dot > slash + 1 ) && name.size() - dot <= 5 )
		return name; // already has an extension

	return name + ".wav";
}

// the engine plays the sentences of .seq files without precaching, here they are
// sounds of the game, they must be precached while the map spawns
static void SeqApi_PrecacheSound( const std::string &name )
{
	const std::vector<std::string> *group = g_seqLib.GetGroup( name );

	if( group )
	{
		for( size_t i = 0; i < group->size(); i++ )
			PRECACHE_SOUND( SeqApi_String( SeqApi_WithWav( ( *group )[i] )));
	}
	else
	{
		PRECACHE_SOUND( SeqApi_String( SeqApi_WithWav( name )));
	}
}

static sequenceEntry_s *SeqApi_Build( const czseq::Entry &entry )
{
	g_seqEntries.push_back( sequenceEntry_s());
	sequenceEntry_s *out = &g_seqEntries.back();
	memset( out, 0, sizeof( *out ));
	out->fileName = SeqApi_String( entry.fileName );
	out->entryName = SeqApi_String( entry.name );
	out->isGlobal = czseq::ToLower( entry.fileName ) == "global";

	sequenceCommandLine_s *prev = NULL;

	for( size_t i = 0; i < entry.commands.size(); i++ )
	{
		const czseq::Command &cmd = entry.commands[i];

		g_seqLines.push_back( sequenceCommandLine_s());
		sequenceCommandLine_s *line = &g_seqLines.back();
		memset( line, 0, sizeof( *line ));

		// trigger_sequence runs a line, then waits its delay before the next one
		line->delay = cmd.delay;
		line->textChannel = cmd.mod.channel;

		switch( cmd.kind )
		{
		case czseq::CMD_FIRE:
			line->commandType = SEQUENCE_COMMAND_FIRETARGETS;
			line->fireTargetNames = SeqApi_String( cmd.value );
			break;
		case czseq::CMD_KILL:
			line->commandType = SEQUENCE_COMMAND_KILLTARGETS;
			line->killTargetNames = SeqApi_String( cmd.value );
			break;
		case czseq::CMD_TEXT:
		{
			client_textmessage_t *msg = &line->clientMessage;

			line->commandType = SEQUENCE_COMMAND_TEXT;
			msg->effect = cmd.mod.effect;
			msg->r1 = cmd.mod.c1[0];
			msg->g1 = cmd.mod.c1[1];
			msg->b1 = cmd.mod.c1[2];
			msg->a1 = cmd.mod.c1[3];
			msg->r2 = cmd.mod.c2[0];
			msg->g2 = cmd.mod.c2[1];
			msg->b2 = cmd.mod.c2[2];
			msg->a2 = cmd.mod.c2[3];
			msg->x = cmd.mod.x;
			msg->y = cmd.mod.y;
			msg->fadein = cmd.mod.fadein;
			msg->fadeout = cmd.mod.fadeout;
			msg->holdtime = cmd.mod.holdtime;
			msg->fxtime = cmd.mod.fxtime;
			msg->pName = out->entryName;
			msg->pMessage = SeqApi_String( SeqApi_Localize( cmd.value ));
			break;
		}
		case czseq::CMD_SOUND:
			line->commandType = SEQUENCE_COMMAND_SOUND;
			line->soundFileName = SeqApi_String( cmd.value );
			if( !cmd.mod.speaker.empty())
				line->speakerName = SeqApi_String( cmd.mod.speaker );
			if( !cmd.mod.listener.empty())
				line->listenerName = SeqApi_String( cmd.mod.listener );
			SeqApi_PrecacheSound( cmd.value );
			break;
		case czseq::CMD_REPEAT:
			line->commandType = SEQUENCE_COMMAND_REPEAT;
			line->repeatCount = cmd.repeat;
			break;
		default:
			line->commandType = SEQUENCE_COMMAND_PAUSE;
			break;
		}

		if( prev )
			prev->nextCommandLine = line;
		else
			out->firstCommand = line;
		prev = line;
	}

	return out;
}

sequenceEntry_s *CZ_SequenceGet( const char *fileName, const char *entryName )
{
	sequenceEntry_s *engineEntry = (sequenceEntry_s *)g_engfuncs.pfnSequenceGet( fileName, entryName );

	if( engineEntry || !entryName || !entryName[0] )
		return engineEntry;

	SeqApi_EnsureMap();
	if( fileName )
		SeqApi_LoadFile( fileName );

	std::string file = fileName ? fileName : "";
	std::string key = czseq::ToLower( file ) + "|" + czseq::ToLower( entryName );

	std::map<std::string, sequenceEntry_s *>::const_iterator it = g_seqBuilt.find( key );
	if( it != g_seqBuilt.end())
		return it->second;

	const czseq::Entry *entry = g_seqLib.Find( file, entryName );
	if( !entry && !file.empty())
		entry = g_seqLib.Find( "", entryName );

	sequenceEntry_s *built = entry ? SeqApi_Build( *entry ) : NULL;
	g_seqBuilt[key] = built;

	if( !built )
		ALERT( at_aiconsole, "sequence: no entry \"%s\"\n", entryName );

	return built;
}

int CZ_SequencePickSentence( const char *groupName, char *found, size_t size )
{
	if( !groupName || !groupName[0] || !size )
		return -1;

	SeqApi_EnsureMap();

	int picked = -1;
	const std::string *sound = g_seqLib.PickSentence( groupName, &picked );
	if( !sound )
		return -1;

	std::string wav = SeqApi_WithWav( *sound );
	snprintf( found, size, "%s", wav.c_str());

	return picked < 0 ? 0 : picked;
}

void CZ_SequencePrecacheGroup( const char *groupName )
{
	if( !groupName || !groupName[0] )
		return;

	SeqApi_EnsureMap();

	if( g_seqLib.GetGroup( groupName ))
		SeqApi_PrecacheSound( groupName );
}
