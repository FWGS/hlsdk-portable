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
//
// text_message.cpp
//
// implementation of CHudTextMessage class
//
// this class routes messages through titles.txt for localisation
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

#if USE_VGUI
#include "vgui_TeamFortressViewport.h"
#endif

DECLARE_MESSAGE( m_TextMessage, TextMsg )

int CHudTextMessage::Init( void )
{
	HOOK_MESSAGE( TextMsg );

	gHUD.AddHudElem( this );

	Reset();

	return 1;
}

// Searches through the string for any msg names (indicated by a '#')
// any found are looked up in titles.txt and the new message substituted
// the new value is pushed into dst_buffer
char *CHudTextMessage::LocaliseTextString( const char *msg, char *dst_buffer, int buffer_size )
{
	char *dst = dst_buffer;
	for( char *src = (char*)msg; *src != 0 && ( buffer_size - 1 ) > 0; buffer_size-- )
	{
		if( *src == '#' )
		{
			// cut msg name out of string
			static char word_buf[255];
			char *wdst = word_buf, *word_start = src;
			int wordbuf_size = (int)sizeof(word_buf);
			for( ++src; ( ( *src >= 'A' && *src <= 'z' ) || ( *src >= '0' && *src <= '9' ) ) && ( wordbuf_size - 1 ) > 0; wdst++, src++, wordbuf_size-- )
			{
				*wdst = *src;
			}
			*wdst = 0;

			// lookup msg name in titles.txt
			client_textmessage_t *clmsg = TextMessageGet( word_buf );
			if( !clmsg || !( clmsg->pMessage ) )
			{
				src = word_start;
				*dst = *src;
				dst++, src++;
				continue;
			}

			// copy string into message over the msg name
			for( char *wsrc = (char*)clmsg->pMessage; *wsrc != 0 && ( buffer_size - 1 ) > 0; wsrc++, dst++, buffer_size-- )
			{
				*dst = *wsrc;
			}
			buffer_size++;
		}
		else
		{
			*dst = *src;
			dst++, src++;
		}
	}

	*dst = 0; // ensure null termination
	return dst_buffer;
}

// As above, but with a local static buffer
char *CHudTextMessage::BufferedLocaliseTextString( const char *msg )
{
	static char dst_buffer[1024];
	LocaliseTextString( msg, dst_buffer, sizeof(dst_buffer) );
	return dst_buffer;
}

// Simplified version of LocaliseTextString;  assumes string is only one word
const char *CHudTextMessage::LookupString( const char *msg, int *msg_dest )
{
	if( !msg )
		return "";

	// '#' character indicates this is a reference to a string in titles.txt, and not the string itself
	if( msg[0] == '#' ) 
	{
		// this is a message name, so look up the real message
		client_textmessage_t *clmsg = TextMessageGet( msg + 1 );

		if( !clmsg || !(clmsg->pMessage) )
			return msg; // lookup failed, so return the original string

		if( msg_dest )
		{
			// check to see if titles.txt info overrides msg destination
			// if clmsg->effect is less than 0, then clmsg->effect holds -1 * message_destination
			if( clmsg->effect < 0 )  // 
				*msg_dest = -clmsg->effect;
		}

		return clmsg->pMessage;
	}
	else
	{
		// nothing special about this message, so just return the same string
		return msg;
	}
}

void StripEndNewlineFromString( char *str )
{
	int s = strlen( str ) - 1;
	if( str[s] == '\n' || str[s] == '\r' )
		str[s] = 0;
}

// converts all '\r' characters to '\n', so that the engine can deal with the properly
// returns a pointer to str
char* ConvertCRtoNL( char *str )
{
	for( char *ch = str; *ch != 0; ch++ )
		if( *ch == '\r' )
			*ch = '\n';
	return str;
}

// Message handler for text messages
// displays a string, looking them up from the titles.txt file, which can be localised
// parameters:
//   byte:   message direction  ( HUD_PRINTCONSOLE, HUD_PRINTNOTIFY, HUD_PRINTCENTER, HUD_PRINTTALK )
//   string: message
// optional parameters:
//   string: message parameter 1
//   string: message parameter 2
//   string: message parameter 3
//   string: message parameter 4
// any string that starts with the character '#' is a message name, and is used to look up the real message in titles.txt
// the next (optional) one to four strings are parameters for that string (which can also be message names if they begin with '#')
int CHudTextMessage::MsgFunc_TextMsg( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int msg_dest = READ_BYTE();

#define MSG_BUF_SIZE 128
	char szBuf[6][MSG_BUF_SIZE];

	strlcpy( szBuf[0], LookupString( READ_STRING(), &msg_dest ), MSG_BUF_SIZE );

	for( int i = 1; i <= 4; i++ )
	{
		// keep reading strings and using C format strings for subsituting the strings into the localised text string
		strlcpy( szBuf[i], LookupString( READ_STRING() ), MSG_BUF_SIZE );
		StripEndNewlineFromString( szBuf[i] ); // these strings are meant for subsitution into the main strings, so cull the automatic end newlines
	}

	char *psz = szBuf[5];

#if USE_VGUI
	if( gViewPort && gViewPort->AllowedToPrintText() == FALSE )
		return 1;
#endif

	switch( msg_dest )
	{
	case HUD_PRINTCENTER:
		safe_snprintf( psz, MSG_BUF_SIZE, szBuf[0], szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		CenterPrint( ConvertCRtoNL( psz ) );
		break;
	case HUD_PRINTNOTIFY:
		psz[0] = 1;  // mark this message to go into the notify buffer
		safe_snprintf( psz + 1, MSG_BUF_SIZE - 1, szBuf[0], szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		ConsolePrint( ConvertCRtoNL( psz ) );
		break;
	case HUD_PRINTTALK:
		safe_snprintf( psz, MSG_BUF_SIZE, szBuf[0], szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		gHUD.m_SayText.SayTextPrint( ConvertCRtoNL( psz ), MSG_BUF_SIZE );
		break;
	case HUD_PRINTCONSOLE:
		safe_snprintf( psz, MSG_BUF_SIZE, szBuf[0], szBuf[1], szBuf[2], szBuf[3], szBuf[4] );
		ConsolePrint( ConvertCRtoNL( psz ) );
		break;
	}

	return 1;
}
