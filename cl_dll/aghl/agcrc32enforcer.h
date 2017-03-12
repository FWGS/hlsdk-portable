//++ BulliT

#ifndef __AG_CRC32ENFORCER_H__
#define __AG_CRC32ENFORCER_H__

#include "agcrc32.h"

bool AgCRC32EnforceFile(char* pszFile, WORD32 w32CheckSum);

bool AgCRC32EnforceFiles();

#endif //__AG_CRC32ENFORCER_H__

//-- Martin Webrant
