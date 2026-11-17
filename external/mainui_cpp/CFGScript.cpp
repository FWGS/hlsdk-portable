/*
cfgscript.c - "Valve script" parsing routines
Copyright (C) 2016 mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#include <time.h>
#include "extdll_menu.h"
#include "BaseMenu.h"
#include "enginecallback_menu.h"
#include "CFGScript.h"

#define CVAR_USERINFO BIT(1)


const char *cvartypes[] = { NULL, "BOOL" , "NUMBER", "LIST", "STRING" };

struct parserstate_t
{
	parserstate_t() : buf( NULL ), filename( NULL ) { token[0] = 0;}
	char *buf;
	char token[MAX_STRING];
	const char *filename;
};

/*
===================
CSCR_ExpectString

Return true if next token is pExpext and skip it
===================
*/
bool CSCR_ExpectString( parserstate_t *ps, const char *pExpect, bool skip, bool error )
{
	char *tmp = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));

	if( !stricmp( ps->token, pExpect ) )
	{
		ps->buf = tmp;
		return true;
	}

	if( skip )
		ps->buf = tmp;

	if( error )
		Con_DPrintf( "Syntax error in %s: got \"%s\" instead of \"%s\"\n", ps->filename, ps->token, pExpect );

	return false;
}

/*
===================
CSCR_ParseType

Determine script variable type
===================
*/
cvartype_t CSCR_ParseType( parserstate_t *ps )
{
	int i;

	for ( i = 1; i < T_COUNT; ++i )
	{
		if( CSCR_ExpectString( ps, cvartypes[i], false, false ) )
			return (cvartype_t)i;
	}

	Con_DPrintf( "Cannot parse %s: Bad type %s\n", ps->filename, ps->token );
	return T_NONE;
}



/*
=========================
CSCR_ParseSingleCvar
=========================
*/
bool CSCR_ParseSingleCvar( parserstate_t *ps, scrvardef_t *result )
{
	// clean linked list for list
	result->list.iCount = 0;
	result->list.pEntries = result->list.pLast = NULL;
	result->list.pArray = NULL;

	// read the name
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, result->name, sizeof( result->name ));

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		goto error;

	// read description
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, result->desc, sizeof( result->desc ));

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		goto error;

	result->type = CSCR_ParseType( ps );

	switch( result->type )
	{
	case T_BOOL:
		// bool only has description
		if( !CSCR_ExpectString( ps, "}", false, true ) )
			goto error;
		break;
	case T_NUMBER:
		// min
		ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));
		result->number.fMin = atof( ps->token );

		// max
		ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));
		result->number.fMax = atof( ps->token );

		if( !CSCR_ExpectString( ps, "}", false, true ) )
			goto error;
		break;
	case T_STRING:
		if( !CSCR_ExpectString( ps, "}", false, true ) )
			goto error;
		break;
	case T_LIST:
		while( !CSCR_ExpectString( ps, "}", true, false ) )
		{
			// char szName[128];
			char *szName = ps->token;
			char szValue[64];
			scrvarlistentry_t *entry = 0;

			// Read token for each item here

			// ExpectString already moves buffer pointer, so just read from ps->token
			// ps->buf = EngFuncs::COM_ParseFile( ps->buf, szName, sizeof( szName ));
			if( !szName[0] )
				goto error;

			ps->buf = EngFuncs::COM_ParseFile( ps->buf, szValue, sizeof( szValue ));
			if( !szValue[0] )
				goto error;

			entry = new scrvarlistentry_t;
			entry->next = NULL;
			entry->szName = StringCopy( szName );
			entry->flValue = atof( szValue );

			if( !result->list.pEntries )
				result->list.pEntries = entry;
			else
				result->list.pLast->next = entry;

			result->list.pLast = entry;
			result->list.iCount++;
		}
		break;
	default:
		goto error;
	}

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		goto error;

	// default value
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, result->value, sizeof( result->value ));

	if( !CSCR_ExpectString( ps, "}", false, true ) )
		goto error;

	if( CSCR_ExpectString( ps, "SetInfo", false, false ) )
		result->flags |= CVAR_USERINFO;

	if( !CSCR_ExpectString( ps, "}", false, true ) )
		goto error;

	if( result->type == T_LIST )
	{
		scrvarlistentry_t *entry = result->list.pEntries;

		result->list.pArray = new const char*[result->list.iCount];
		result->list.pModel = new CStringArrayModel( result->list.pArray, result->list.iCount );

		for( int i = 0; entry; entry = entry->next, i++ )
		{
			result->list.pArray[i] = L( entry->szName );
		}
	}

	return true;
error:
	if( result->type == T_LIST )
	{
		if( result->list.pArray )
			delete[] result->list.pArray;
		if( result->list.pModel )
			delete result->list.pModel;

		while( result->list.pEntries )
		{
			scrvarlistentry_t *next = result->list.pEntries->next;
			delete[] result->list.pEntries->szName;
			delete result->list.pEntries;

			result->list.pEntries = next;
		}
	}
	return false;
}

/*
======================
CSCR_ParseHeader

Check version and seek to first cvar name
======================
*/
bool CSCR_ParseHeader( parserstate_t *ps )
{
	if( !CSCR_ExpectString( ps, "VERSION", false, true ) )
		return false;

	// Parse in the version #
	// Get the first token.
	ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));

	if( atof( ps->token ) != 1 )
	{
		Con_DPrintf( "File %s has wrong version %s!\n", ps->filename, ps->token );
		return false;
	}

	if( !CSCR_ExpectString( ps, "DESCRIPTION", false, true ) )
		return false;

	ps->buf = EngFuncs::COM_ParseFile( ps->buf, ps->token, sizeof( ps->token ));

	if( stricmp( ps->token, "INFO_OPTIONS") && stricmp( ps->token, "SERVER_OPTIONS" ) )
	{
		Con_DPrintf( "DESCRIPTION must be INFO_OPTIONS or SERVER_OPTIONS\n");
		return false;
	}

	if( !CSCR_ExpectString( ps, "{", false, true ) )
		return false;

	return true;
}

/*
======================
CSCR_LoadDefaultCVars

Register all cvars declared in config file and set default values
======================
*/
scrvardef_t *CSCR_LoadDefaultCVars( const char *scriptfilename, int *count )
{
	int length = 0;
	char *start;
	parserstate_t state;
	bool success = false;
	scrvardef_t *list = 0, *last = 0;

	*count = 0;

	state.filename = scriptfilename;

	state.buf = (char*)EngFuncs::COM_LoadFile( scriptfilename, &length );

	start = state.buf;

	if( state.buf == 0 || length == 0)
	{
		if( start )
			EngFuncs::COM_FreeFile( start );
		return 0;
	}

	Con_DPrintf( "Reading config script file %s\n", scriptfilename );

	if( !CSCR_ParseHeader( &state ) )
	{
		Con_DPrintf( "Failed to	parse header!\n" );
		goto finish;
	}

	while( !CSCR_ExpectString( &state, "}", false, false ) )
	{
		scrvardef_t var;

		// Create a new object
		if( CSCR_ParseSingleCvar( &state, &var ) )
		{
			scrvardef_t *entry = new scrvardef_t;
			*entry = var;

			if( !list )
			{
				list = last = entry;
			}
			else
			{
				last = last->next = entry;
			}
			(*count)++;
		}
		else
			break;

		if( *count > 1024 )
			break;
	}

	if( EngFuncs::COM_ParseFile( state.buf, state.token, sizeof( state.token )))
		Con_DPrintf( "Got extra tokens!\n" );
	else
		success = true;

finish:
	if( !success )
	{
		state.token[ sizeof( state.token ) - 1 ] = 0;
		if( start && state.buf )
			Con_DPrintf( "Parse error in %s, byte %d, token %s\n", scriptfilename, (int)( state.buf - start ), state.token );
		else
			Con_DPrintf( "Parse error in %s, token %s\n", scriptfilename, state.token );
	}
	if( start )
		EngFuncs::COM_FreeFile( start );

	return list;
}

/*
======================
CSCR_SaveToFile

Save cvars to script file with specific formatting
======================
*/
void CSCR_SaveToFile( const char *filename, const char *description, scrvardef_t *list )
{
	if( !filename || !list ) return;

	CUtlString buffer;

	// Write current time in header
	char timeBuf[64];
	time_t now = time( NULL );
	strftime( timeBuf, sizeof(timeBuf), "%a %b %d %I:%M:%S %p", localtime( &now ) );

	buffer.AppendFormat( "// NOTE:  THIS FILE IS AUTOMATICALLY REGENERATED, \n" );
	buffer.AppendFormat( "//DO NOT EDIT THIS HEADER, YOUR COMMENTS WILL BE LOST IF YOU DO\n" );
	
	if( !stricmp( description, "INFO_OPTIONS" ) )
		buffer.AppendFormat( "// User options script\n" );
	else
		buffer.AppendFormat( "// Multiplayer options script\n" );

	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "// Format:\n" );
	buffer.AppendFormat( "//  Version [float]\n" );
	buffer.AppendFormat( "//  Options description followed by \n" );
	buffer.AppendFormat( "//  Options defaults\n" );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "// Option description syntax:\n" );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "//  \"cvar\" { \"Prompt\" { type [ type info ] } { default } }\n" );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "//  type = \n" );
	buffer.AppendFormat( "//   BOOL   (a yes/no toggle)\n" );
	buffer.AppendFormat( "//   STRING\n" );
	buffer.AppendFormat( "//   NUMBER\n" );
	buffer.AppendFormat( "//   LIST\n" );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "// type info:\n" );
	buffer.AppendFormat( "// BOOL                 no type info\n" );
	buffer.AppendFormat( "// NUMBER       min max range, use -1 -1 for no limits\n" );
	buffer.AppendFormat( "// STRING       no type info\n" );
	buffer.AppendFormat( "// LIST          delimited list of options value pairs\n" );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "// default depends on type\n" );
	buffer.AppendFormat( "// BOOL is \"0\" or \"1\"\n" );
	buffer.AppendFormat( "// NUMBER is \"value\"\n" );
	buffer.AppendFormat( "// STRING is \"value\"\n" );
	buffer.AppendFormat( "// LIST is \"index\", where index \"0\" is the first element of the list\n" );
	buffer.AppendFormat( "\n\n" );

	if( !stricmp( description, "INFO_OPTIONS" ) )
		buffer.AppendFormat( "// Half-Life User Info Configuration Layout Script (stores last settings chosen, too)\n" );
	else
		buffer.AppendFormat( "// Half-Life Server Configuration Layout Script (stores last settings chosen, too)\n" );
	
	buffer.AppendFormat( "// File generated:  %s\n", timeBuf );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "//\n" );
	buffer.AppendFormat( "// Cvar	-	Setting\n" );
	buffer.AppendFormat( "\n" );

	buffer.AppendFormat( "VERSION 1.0\n" );
	buffer.AppendFormat( "\n" );
	buffer.AppendFormat( "DESCRIPTION %s\n", description );
	buffer.AppendFormat( "{\n" );

	for( scrvardef_t *var = list; var; var = var->next )
	{
		const char *currentValue = var->value;

		buffer.AppendFormat( "\t\"%s\"\n", var->name );
		buffer.AppendFormat( "\t{\n" );
		buffer.AppendFormat( "\t\t\"%s\"\n", var->desc );
		
		// Type info
		buffer.AppendFormat( "\t\t{ " );
		switch( var->type )
		{
		case T_BOOL:
			buffer.AppendFormat( "BOOL" );
			break;
		case T_STRING:
			buffer.AppendFormat( "STRING" );
			break;
		case T_NUMBER:
			buffer.AppendFormat( "NUMBER %g %g", var->number.fMin, var->number.fMax );
			break;
		case T_LIST:
			buffer.AppendFormat( "\n\t\t\tLIST" );
			for( scrvarlistentry_t *entry = var->list.pEntries; entry; entry = entry->next )
			{
				buffer.AppendFormat( "\n\t\t\t\"%s\" \"%g\"", entry->szName, entry->flValue );
			}
			break;
		default:
			break;
		}
		
		if( var->type != T_LIST )
			buffer.AppendFormat( " }" );
		else
			buffer.AppendFormat( "\n\t\t}" );

		buffer.AppendFormat( "\n" );

		// Value
		buffer.AppendFormat( "\t\t{ \"%s\" }\n", currentValue );

		if( var->flags & CVAR_USERINFO )
		{
			buffer.AppendFormat( "\t\tSetInfo\n" );
		}
		
		buffer.AppendFormat( "\t}\n\n" );
	}

	buffer.AppendFormat( "}\n" );
	
	EngFuncs::COM_SaveFile( filename, buffer.Get(), buffer.Length() );
}

void CSCR_FreeList( scrvardef_t *list )
{
	scrvardef_t *i = list;
	while( i )
	{
		scrvardef_t *next = i->next;

		if( i->type == T_LIST )
		{
			if( i->list.pModel )
				delete i->list.pModel;
			if( i->list.pArray )
				delete[] i->list.pArray;

			while( i->list.pEntries )
			{
				scrvarlistentry_t *next = i->list.pEntries->next;
				delete[] i->list.pEntries->szName;
				delete i->list.pEntries;

				i->list.pEntries = next;
			}
		}

		delete i;
		i = next;
	}
}
