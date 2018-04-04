// ---------------------------------------------------------------
// BubbleMod
// 
// AUTHOR
//        Tyler Lund <halflife@bubblemod.org>
//
// LICENSE                                                            
//                                                                    
//        Permission is granted to anyone to use this  software  for  
//        any purpose on any computer system, and to redistribute it  
//        in any way, subject to the following restrictions:          
//                                                                    
//        1. The author is not responsible for the  consequences  of  
//           use of this software, no matter how awful, even if they  
//           arise from defects in it.                                
//        2. The origin of this software must not be misrepresented,  
//           either by explicit claim or by omission.                 
//        3. Altered  versions  must  be plainly marked as such, and  
//           must  not  be  misrepresented  (by  explicit  claim  or  
//           omission) as being the original software.                
//        3a. It would be nice if I got  a  copy  of  your  improved  
//            version  sent to halflife@bubblemod.org. 
//        4. This notice must not be removed or altered.              
//                                                                    
// ---------------------------------------------------------------
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include <time.h>
#include "shake.h"
#include "decals.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include <string.h>
#include <time.h>

// bigguy's CastPlayer functions

CBasePlayer* UTIL_CastPlayer ( CBaseEntity *pEnt )
{
	if (pEnt) // check validity
	{
		if (pEnt->IsPlayer()) // is it even a player?
		{
			CBasePlayer *plr = GetClassPtr((CBasePlayer *)pEnt); // cast player
			if (plr) // check validity
				return plr; // return pointer
		}
	}

	return NULL; // otherwise, its not a player
}

CBasePlayer* UTIL_CastPlayer ( entvars_t *pev )
{
	if (pev) // check validity
	{
		if (FClassnameIs(pev, "player")) // is it even a player?
		{
			CBasePlayer *plr = GetClassPtr((CBasePlayer *)pev); // cast player
			if (plr) // check validity
				return plr; // return pointer
		}
	}
	return NULL; // otherwise, its not a player
}
CBasePlayer* UTIL_CastPlayer ( edict_t *pEdict )
{
	if (pEdict) // check validity
	{
//		if (FClassnameIs(pEdict, "player")) // is it even a player?
//		{
			entvars_t *pev = &pEdict->v;
			// CBasePlayer *plr = GetClassPtr((CBasePlayer *)pev); // cast player
			CBasePlayer *plr = (CBasePlayer*) CBasePlayer::Instance( pev ); // cast player
			if (plr) // check validity
				return plr; // return pointer
//		}
	}
	return NULL; // otherwise, its not a player
}
CBasePlayer* UTIL_CastPlayer ( int index )
{
	if (index > 0 && index <= gpGlobals->maxClients) // check validity
	{
		edict_t *pEdict = INDEXENT(index); // cast pEdict
		CBasePlayer *plr = UTIL_CastPlayer(pEdict); // cast player
		if (plr) // check validity
			return plr; // return pointer
	}
	return NULL; // otherwise, its not a player
}

// Oriented Bounding Box to line segment intersection test
// Adapted from Miguel Gomez article on Gamasutra Oct. '99
//
// Arguments are line start point, line end point, BBox center, and BBox extents
// 
// Returns TRUE if the line and BBox intersect, FALSE if they do not.
BOOL UTIL_OBB_LineTest(Vector vecSrc, Vector vecDst, Vector boxP, Vector boxE)
{
	// Unit vector in direction of line.
	Vector l = vecDst - vecSrc;

	// Half of line segment length
	float hl = fabs(l.Length() / 2);
	
	l = l.Normalize();

	// midpoint of line segemnt.
	Vector mid = (vecDst + vecSrc) / 2;
	
	// The separating axis.
	Vector T = boxP - mid;
/*
	char sPlayers[256] = "";
	sprintf(sPlayers, "Start: %f, %f, %f\nEnd: %f, %f, %f\nhl: %f\nunit: %f, %f, %f\n\n", vecSrc.x, vecSrc.y, vecSrc.z,
			vecDst.x, vecDst.y, vecDst.z, hl, l.x, l.y, l.z);
	g_engfuncs.pfnServerPrint( sPlayers );


	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_BEAMPOINTS );
                WRITE_COORD(mid.x);
                WRITE_COORD(mid.y);
                WRITE_COORD(mid.z);
                WRITE_COORD(mid.x + (hl * l.x));
                WRITE_COORD(mid.y + (hl * l.y));
                WRITE_COORD(mid.z + (hl * l.z));
                WRITE_SHORT( g_sModelIndexLaser );
                WRITE_BYTE( 0 ); // Starting frame
                WRITE_BYTE( 0  ); // framerate * 0.1
                WRITE_BYTE( 255 ); // life * 0.1
                WRITE_BYTE( 10 ); // width
                WRITE_BYTE( 0 ); // noise
                WRITE_BYTE( 0 ); // color r,g,b
                WRITE_BYTE( 0 ); // color r,g,b
                WRITE_BYTE( 255 ); // color r,g,b
                WRITE_BYTE( 255 ); // brightness
                WRITE_BYTE( 0 ); // scroll speed
    MESSAGE_END();
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_BEAMPOINTS );
                WRITE_COORD(mid.x);
                WRITE_COORD(mid.y);
                WRITE_COORD(mid.z);
                WRITE_COORD(mid.x - (hl * l.x));
                WRITE_COORD(mid.y - (hl * l.y));
                WRITE_COORD(mid.z - (hl * l.z));
                WRITE_SHORT( g_sModelIndexLaser );
                WRITE_BYTE( 0 ); // Starting frame
                WRITE_BYTE( 0  ); // framerate * 0.1
                WRITE_BYTE( 255 ); // life * 0.1
                WRITE_BYTE( 10 ); // width
                WRITE_BYTE( 0 ); // noise
                WRITE_BYTE( 255 ); // color r,g,b
                WRITE_BYTE( 0 ); // color r,g,b
                WRITE_BYTE( 0 ); // color r,g,b
                WRITE_BYTE( 255 ); // brightness
                WRITE_BYTE( 0 ); // scroll speed
    MESSAGE_END();
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
                WRITE_BYTE( TE_BEAMPOINTS );
                WRITE_COORD(mid.x);
                WRITE_COORD(mid.y);
                WRITE_COORD(mid.z);
                WRITE_COORD(boxP.x);
                WRITE_COORD(boxP.y);
                WRITE_COORD(boxP.z);
                WRITE_SHORT( g_sModelIndexLaser );
                WRITE_BYTE( 0 ); // Starting frame
                WRITE_BYTE( 0  ); // framerate * 0.1
                WRITE_BYTE( 255 ); // life * 0.1
                WRITE_BYTE( 10 ); // width
                WRITE_BYTE( 0 ); // noise
                WRITE_BYTE( 255 ); // color r,g,b
                WRITE_BYTE( 255 ); // color r,g,b
                WRITE_BYTE( 255 ); // color r,g,b
                WRITE_BYTE( 255 ); // brightness
                WRITE_BYTE( 0 ); // scroll speed
    MESSAGE_END();
*/
	// The box is assumed to be axis aligned to the world, so we start
	// with those trivial cases.
	if( ( fabs(T.x) > boxE.x + hl * fabs(l.x) ) ||
		( fabs(T.y) > boxE.y + hl * fabs(l.y) ) ||
		( fabs(T.z) > boxE.z + hl * fabs(l.z) ) )
		return FALSE;

	// Now we check the box transformed into the line space.
	if ( ( fabs(T.y*l.z - T.z*l.y) > boxE.y*fabs(l.z) + boxE.z*fabs(l.y) ) ||
		 ( fabs(T.z*l.x - T.x*l.z) > boxE.z*fabs(l.x) + boxE.x*fabs(l.z) ) ||
		 ( fabs(T.x*l.y - T.y*l.x) > boxE.x*fabs(l.y) + boxE.y*fabs(l.x) ) )
		 return FALSE;

	return TRUE;
}

// Oriented Bounding Box to point intersection test
//
// Arguments are line point, BBox center, and BBox extents
// 
// Returns TRUE if the point and BBox intersect, FALSE if they do not.
BOOL UTIL_OBB_PointTest(Vector vecSrc, Vector boxP, Vector boxE)
{
	if ((vecSrc.x < (boxP.x - boxE.x) )||
		(vecSrc.x > (boxP.x + boxE.x) )||
		(vecSrc.y < (boxP.y - boxE.y) )||
		(vecSrc.y > (boxP.y + boxE.y) )||
		(vecSrc.z < (boxP.z - boxE.z) )||
		(vecSrc.z > (boxP.z + boxE.z) ))
		return FALSE;

		return TRUE;
}

void UTIL_SpeakAll( const char *message )
{
	char text[256];
	strcpy(text, "speak \"");
	strcat(text, message);
	strcat(text, "\"\n");

	CBasePlayer *client = NULL;

	while ( ((client = (CBasePlayer*)UTIL_FindEntityByClassname( client, "player" )) != NULL) && (!FNullEnt(client->edict())) ) 
	{
		CLIENT_COMMAND(client->edict(), text);
	}
}

void UTIL_SpeakBadWeapon( void )
{
	switch (RANDOM_LONG(0, 3))
	{
	case 0:
		UTIL_SpeakAll("deeoo, warning, unauthorized weapon use detected");
		break;
	case 1:
		UTIL_SpeakAll("deeoo, warning, weapon violation detected");
		break;
	case 2:
		UTIL_SpeakAll("deeoo, warning, forbidden use of detonation device");
		break;
	case 3:
		UTIL_SpeakAll("deeoo, warning, illegal explosion in sector fourty two");
		break;
	}
}

char* UTIL_CountVotes()
{
	static char string[128];
	char maps[32][81];
	int votes[32];
	int i, p = 0;
	int q = 0;

	for( i = 0; i < 32; i++ )
	{
		strcpy(maps[i], "");
		votes[i] = 0;
	}

	CBasePlayer *client = NULL;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		client = (CBasePlayer*)UTIL_PlayerByIndex( i );

		if ( client && (client->m_bIsConnected) && (IS_MAP_VALID(client->m_sMapVote) || !strcmp(client->m_sMapVote, "extend"))) 
		{
			p = 0;
			while(strcmp(client->m_sMapVote, maps[p]) && (p < 32))
			{
				p++;
			}

			if (p < 32)
			{
				votes[p]++;
			}
			else
			{
				strcpy(maps[q], client->m_sMapVote);
				votes[q]++;
				q++;
			}
		}
	}

	q = 0;
	for (i = 0; i < 32; i++)
	{
		if (votes[i] > votes[q])
			q = i;
	}

	string[0] = (char)votes[q];
	string[1] = 0;

	strcat(&(string[1]), maps[q]);

	return string;
}

typedef struct
{
	char WONid[81];
//	unsigned int WONid;
	int	frags;
	int deaths;
	int SpawnKills;
	int TypeKills;
	BOOL IsLlama;
} SavedPlayer;

void UTIL_SaveRestorePlayer( CBasePlayer *pPlayer, BOOL save, BOOL resetall ) 
{
	// return;

	static SavedPlayer Player[32];
	static BOOL	init = FALSE;

	// Initialize the array
	if (!init || resetall) 
	{
		for (int i = 0; i < 32; i++) 
		{
			Player[i].WONid[0] = 0;
//			Player[i].WONid = 0;
			Player[i].frags = 0;
			Player[i].deaths = 0;
			Player[i].SpawnKills = 0;
			Player[i].TypeKills = 0;
			Player[i].IsLlama = 0;
		}
		init = TRUE;
	}

	if (resetall)
		return;

	// Save a player's data
	if (save)
	{
		for (int i = 0; i < 32; i++) 
		{
			if (Player[i].WONid[0] == 0)
//			if (Player[i].WONid == 0)
			{
				strcpy(Player[i].WONid, GETPLAYERAUTHID(pPlayer->edict()));
//				Player[i].WONid = GETPLAYERAUTHID(pPlayer->edict());
				//Player[i].frags = pPlayer->pev->frags;
				//Player[i].deaths = pPlayer->m_iDeaths;
				Player[i].SpawnKills = pPlayer->m_iSpawnKills;
				Player[i].TypeKills = pPlayer->m_iTypeKills;
				Player[i].IsLlama = pPlayer->m_IsLlama;
				break;
			}
		}
	}

	// Otherwise load a player's data
	else
	{
		for (int i = 0; i < 32; i++) 
		{
			if (!strcmp(Player[i].WONid, GETPLAYERAUTHID(pPlayer->edict())))
//			if (Player[i].WONid == GETPLAYERAUTHID(pPlayer->edict()))
			{
				//pPlayer->pev->frags = Player[i].frags;
				//pPlayer->m_iDeaths = Player[i].deaths;
				pPlayer->m_IsLlama = Player[i].IsLlama;
				pPlayer->m_iSpawnKills = Player[i].SpawnKills;
				pPlayer->m_iTypeKills = Player[i].TypeKills;
				Player[i].WONid[0] = 0;
//				Player[i].WONid = 0;
				Player[i].frags = 0;
				Player[i].deaths = 0;
				Player[i].SpawnKills = 0;
				Player[i].TypeKills = 0;
				break;
			}
		}
	}
}

const int NUMSTRINGS = 43;
const char *XLate [NUMSTRINGS*5] ={ "YOU", "J00", "JO0", "j00", "Jo0",
									"?", "?!", "!?", "?!?", "!?!",
									"!", "!!", "!", "!!", "!",
									"IES", "YS", "yZ", "Y$", "y5",
									"HACKER", "H4X0r" ,"H4><0r", "h4X0r", "h4XoR",
									"SUCK", "SuX0rZ" ,"Su><0rZ", "sUX0rz", "suXoRZ",
									"DUDE", "D00D" ,"d00d", "dUUD", "|)0o|)",
									"QU", "|<u", "Ku", "kU", "]<U",
									"ENOUGH", "3nUfF", "EnUF" ,"3|\\|UF", "eNuFF",
									"DOES", "dUz", "D0Z", "Du$", "duuZZ",
									"TOO", "2", "2", "2", "2",
									"TO", "2", "2", "2", "2",
									"FOR", "4", "4", "Fo|2", "f0R",
									"WHAT", "wUt", "Wu7", "Wu+", "wU++",
									"ROCK", "|20X0rZ" ,"ro><orZ", "r()x()|2$", "r0x0rZ",
									"COOL", "|<3w3l" ,"k3vv3l", "KeW3|_", "k3vv3L",
									"STUPID", "$t00piD" ,"Z+00p!d", "S700P!D", "z7OOp!d",
									"A", "/\\", "A", "4", "@",
                                    "B", "B", "b", "8", "|3",
									"C", "C", "c", "[", "(",
									"D", "D", "d", "|)", "|]",
									"E", "E", "e", "3", "3",
									"F", "F", "f", "F", "f",
									"G", "G", "g", "G", "6",
									"H", "H", "h", "H", "h",
									"I", "I", "i", "!", "!",
									"J", "J", "j", "J", "j",
									"K", "K", "k" ,"|<", "]<",
									"L", "L", "l", "1", "|_",
									"M", "M", "m", "M", "m",
									"N", "N", "n", "|\\|", "N",
									"O", "<>", "o", "0", "()",
									"P", "P", "p", "P", "p",
									"Q", "Q", "q", "Q", "q",
									"R", "R", "r", "|2", "|2",
									"S", "z", "$", "5", "Z",
									"T", "T", "t", "7", "+",
									"U", "U", "u", "U", "u",
									"V", "V", "v", "\\/", "V",
									"W", "W", "w", "vv", "W",
									"X", "X", "x", "><", "%",
									"Y", "Y", "y", "Y", "y",
									"Z", "Z", "z", "Z", "z"
								};

void UTIL_Speak_2_l33t( char *szTarget, char *szOString )
{
	const int MAX_L33T = 512;

	int i = 0;
	char szNewString[MAX_L33T] = "";
	char szString[MAX_L33T] = "";
	strcpy(szString, szOString);
	int j;
	BOOL match;

	for (i=0; i<(int)strlen(szString); i++) 
	{
		szString[i] = toupper(szString[i]);
	};

	i = 0;
	for (i=0; i<(int)strlen(szString); i++) 
	{
		match = false;
		for(j=0; j<NUMSTRINGS; j++) 
		{
			if (!strncmp(szString+i, XLate[j*5], strlen(XLate[j*5])) )
			{
				strcat(szNewString, XLate[j*5+RANDOM_LONG(1,4)]);
				i += strlen(XLate[j*5])-1;
				match= true;
				break;
			}
		}
		if (!match)
		{
			strncat(szNewString, szString+i, 1);
		}
	}
	strcat(szTarget, szNewString);
}

void UTIL_SayTime( void ) {
	//	 tm_sec:	seconds after the minute (0-59)
	//	 tm_min:	minutes after hour (0-59)
	//	 tm_hour:	hours after midnight (0-23)
	//	 tm_mday:	day of month (1-31)
	//	 tm_mon:	month of year (0-11; January == 0)
	//	 tm_year:	year (value of current year minus 1900)

	struct tm *adjustedTime;
	time_t sysTime;

	time( &sysTime );			// Get system time encoded as time_t
	adjustedTime = localtime( &sysTime );	// Adjust for local time

	char szTime[80];

	int hour = adjustedTime->tm_hour;
	hour = hour > 12 ? hour - 12 : hour;
	hour = hour == 0 ? 12 : hour;
	int min = adjustedTime->tm_min;
	const char *ampm = "AM";
	if (adjustedTime->tm_hour > 11)
		ampm = "PM";

	sprintf(szTime, "<SERVER> Local server time is: %d:%02d %s\n", hour, min, ampm);
	UTIL_ClientPrintAll( HUD_PRINTTALK, szTime);
}

// Convert a four digit hex string to an int. 
int UTIL_axtoi(char *hexStg) {
  int n = 0;         // position in string
  int m = 0;         // position in digit[] to shift
  int count;         // loop index
  int intValue = 0;  // integer value of hex string
  int digit[5];      // hold values to convert
  while (n < 4) {
     if (hexStg[n]=='\0')
        break;
     if (hexStg[n] > 0x29 && hexStg[n] < 0x40 ) //if 0 to 9
        digit[n] = hexStg[n] & 0x0f;            //convert to int
     else if (hexStg[n] >='a' && hexStg[n] <= 'f') //if a to f
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else if (hexStg[n] >='A' && hexStg[n] <= 'F') //if A to F
        digit[n] = (hexStg[n] & 0x0f) + 9;      //convert to int
     else break;
    n++;
  }
  count = n;
  m = n - 1;
  n = 0;
  while(n < count) {
     // digit[n] is value of hex digit at position n
     // (m << 2) is the number of positions to shift
     // OR the bits into return value
     intValue = intValue | (digit[n] << (m << 2));
     m--;   // adjust the position to set
     n++;   // next digit to process
  }
  return (intValue);
}
