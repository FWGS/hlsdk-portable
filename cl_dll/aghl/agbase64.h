//++ BulliT

#ifndef __AG_BASE64_H__
#define __AG_BASE64_H__

void AgBase64Decode(const char* pszString, unsigned long ulInLen, unsigned char* pszBuffer, unsigned short& ulOutLen);
void AgBase64Encode(const unsigned char* pszString, unsigned long ulInLen, char* pszBuffer);

#endif //__AG_BASE64_H__

//-- Martin Webrant
