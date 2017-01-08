// halflife common
#include        "extdll.h"
#include        "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "client.h"
// regamedll
#define NOXREF
#include "bot/GameEvent.h"
#include "bot/botsengcallback.h"
typedef long long int64;
typedef int int32;
typedef unsigned int uint32;
#define Q_stricmp strcasecmp

#define Q_max( a, b ) (((a) > (b)) ? (a) : (b))
#define Q_min( a, b ) (((a) < (b)) ? (a) : (b))
#define Q_atoi atoi
#define Q_snprintf snprintf
#define Q_strncpy strncpy
#define CloneString strdup
#define Q_atof atof
#define Q_strchr strchr
#define Q_strcmp strcmp
#define Q_strcpy strcpy
#define Q_sprintf sprintf
#define Q_strlen strlen
#define Q_write write
#define Q_close close
#define Q_strcat strcat
#define Q_stricmp strcasecmp
#define Q_strnicmp strncasecmp
#define Q_vsnprintf vsnprintf
template <typename T>
T clamp(T a, T min, T max) { return (a > max) ? max : (a < min) ? min : a; }

#include "bot/pm_math.h"
static char mp_com_token[64];

static inline char *MP_COM_GetToken()
{
	return mp_com_token;
}

static inline char *MP_COM_Parse(char *data)
{
	int c;
	int len;

	len = 0;
	mp_com_token[0] = '\0';

	if (!data)
	{
		return NULL;
	}

skipwhite:
	// skip whitespace
	while (*data <= ' ')
	{
		if (!data[0])
			return NULL;

		++data;
	}

	c = *data;

	// skip // comments till the next line
	if (c == '/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			++data;

		goto skipwhite;	// start over new line
	}

	// handle quoted strings specially: copy till the end or another quote
	if (c == '\"')
	{
		++data;	// skip starting quote

		while (true)
		{
			// get char and advance
			c = *data++;

			if (c == '\"' || !c)
			{
				mp_com_token[ len ] = '\0';
				return data;
			}

			mp_com_token[ len++ ] = c;
		}
	}

	// parse single characters
	if (c == '{' || c == '}'|| c == ')'|| c == '(' || c == '\'' || c == ',')
	{
		mp_com_token[ len++ ] = c;
		mp_com_token[ len ] = '\0';

		return data + 1;
	}

	// parse a regular word
	do
	{
		mp_com_token[ len++ ] = c;
		++data;
		c = *data;

		if (c == '{' || c == '}'|| c == ')'|| c == '(' || c == '\'' || c == ',')
			break;
	}
	while (c > 32);

	mp_com_token[ len ] = '\0';
	return data;
}

static inline int MP_COM_TokenWaiting(char *buffer)
{
	char *p;

	p = buffer;
	while (*p && *p != '\n')
	{
		if (!isspace(*p) || isalnum(*p))
			return 1;

		++p;
	}

	return 0;
}
#include "bot/shared_util.h"
#include "bot/utllinkedlist.h"
#include "bot/utlvector.h"
#define MAX_CLIENTS 32

// manager
class CNavNode;
#include "bot/steam_util.h"
#include "bot/manager/nav.h"
#include "bot/manager/nav_area.h"
#include "bot/manager/nav_node.h"
#include "bot/manager/nav_file.h"
#include "bot/manager/improv.h"
#include "bot/manager/bot_util.h"

#include "bot/manager/nav_path.h"



#include "bot/manager/bot.h"

#include "bot/manager/bot_manager.h"

#include "bot/manager/bot_constants.h"
#include "bot/manager/bot_profile.h"

// zbot
#include "bot/cs_bot.h"
