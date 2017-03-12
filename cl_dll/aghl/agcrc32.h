//++ BulliT

#ifndef __AG_CRC32_H__
#define __AG_CRC32_H__

#ifdef WIN32
typedef unsigned __int8  WORD8;   // unsigned 8bit integer, prefix "b"
typedef unsigned __int32 WORD32;  // unsigned 8bit integer, prefix "l"
#else
typedef unsigned char      WORD8;
typedef unsigned int       WORD32;
#endif

WORD32 AgCRC32(const void* pData, WORD32 lNumOfBytes);

#endif //__AG_CRC_H__

//-- Martin Webrant
