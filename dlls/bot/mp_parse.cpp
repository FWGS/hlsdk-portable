#include "bot_common.h"

static char mp_com_token[64];

char *MP_COM_GetToken()
{
	return mp_com_token;
}

char *MP_COM_Parse(char *data)
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

int MP_COM_TokenWaiting(char *buffer)
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
