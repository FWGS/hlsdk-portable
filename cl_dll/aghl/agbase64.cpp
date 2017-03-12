//++ BulliT

#include "agbase64.h"
#include <string.h>
#include <assert.h>

// define the US-ASCII chars
static char s_szBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/\0";

// how many bits per byte?
#define NUM_BITS    8  
#define BYTES_TO_READ 60   // only for my test method Encode-file

// define masks for Base64-encode
static int i_aMsbMask[] = { 0xfc, 0xf0, 0xc0, 0x00 };
static int i_aLsbMask[] = { 0x00, 0x03, 0x0f, 0x3f };

// Define array for conversion between chars in base64 message and their weight according to array s_szBase64
// This could be done by counting offset in s_szBase64. But sorry, too slow!
// start with asccii 2B (ie '+')
static int i_a64CharWeight [] =
{
  62, -1, -1, -1, 63,     // I have defined '+' and '/'
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61,   // I have defined 0 through 9
  -1, -1, -1, -1, -1, -1, -1,     // have undefined ascii 3A to 40
  // lets define 'A' to 'Z'
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,   // A - Z
  -1, -1, -1, -1, -1, -1,     // have undefined ascii 5B to 40
  // lets define 'a' to 'z'
  26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,
  49,50,51    // defined 'a' to 'z'
};

static int i_aDecodeMsbMask[] = { 0x3f, 0x0f, 0x03 };
static int i_aDecodeLsbMask[] = { 0x30, 0x3c, 0x3f };


//Decodes base64-encoded string.
//Assumptions: Incoming buffer (pszBuffer) is large enough.
void AgBase64Decode(const char* pszString,unsigned long ulInLen,unsigned char* pszBuffer, unsigned short& usOutLen)
{
  char* p = (char*)pszBuffer;
  const char* pszSource = (const char*)pszString;
  usOutLen = 0;
  assert(0 == (ulInLen % 4));  // base-64 coded text should always be done in parts of 4 bytes

  while(ulInLen > 0)
  {
    int iAsciiO = i_a64CharWeight[*pszSource++ - 0x2B];
    for (int i = 1; i <= 3; i++)
    {
      int iAsciiChar = (iAsciiO & i_aDecodeMsbMask[i-1]) << (2*i);
      int iChar = i_a64CharWeight[*pszSource++ - 0x2B];
      iAsciiChar += (iChar & i_aDecodeLsbMask[i-1]) >> (6-2*i);
      iAsciiO = iChar;
      *p++ = (char)iAsciiChar;
      --ulInLen;
      if (!('\0' == *(p-1) && '\0' == iAsciiChar)) //BAD BAD BAD! need to check what chars is padded better.
        ++usOutLen; 
    }
    --ulInLen;
  }

}


//Encodes incoming string to Base64.
//Assumptions: Incoming buffer (pszBuffer) is large enough.
//Haven't bothered to optimize this one since we only do it when we create file.
void AgBase64Encode(const unsigned char* pszString,unsigned long ulInLen,char* pszBuffer)
{
  assert(pszString);
  assert(pszBuffer);

  char* p = (char*)pszString;

  while (ulInLen > 0)
  {
    int iAsciiO = 0;
    // encode 3 characters (will become 4 chars when Base64 encoded)
    for (int i = 0; i < 4; i++)
    {
      int iAsciiN = 0;

      if (ulInLen)
        iAsciiN = (int)*p;

      int iMsbRightShift = NUM_BITS - (6-2*i);
      int iLsbLeftShift = NUM_BITS - iMsbRightShift;

      int iChar64 = ((iAsciiN & i_aMsbMask[i]) >> iMsbRightShift);
      iChar64 += ((iAsciiO & i_aLsbMask[i]) << iLsbLeftShift);

      iAsciiO = iAsciiN;
      
      assert(iChar64 >= 0 && iChar64 <= 63);
      
      if (i < 3)
      {
        p++;
        if (ulInLen)
          ulInLen--;
      }

      *pszBuffer++ = s_szBase64[iChar64];
    }

  }
  *pszBuffer = 0;
}

//-- Martin Webrant

/*

static char* s_szBadCodes[] =
{
  "glhack",   
  "opengl.ini",  
  "TWCheat",   
  "B.teraphy", 
  "Flautz",
  "sw!zz3r",
  "ANAKiN",
  "hooker.dll",
  "UPX!", //whb31
  "c:\\opengl32.dll",
  "hlh.dll",
  "GRiM-F_H",
  "ChromaxS",
  "ogc.dll",
  "Unhooker", // eller hlh.dll
  "eZ!$7v", //Swizz hack
  "coders.dll", //wh_beta4, wh_beta5
  "ogc.cfg",
  "xqz2", //xqz2_b71
  "xqb6", //xqz2_b80
  "p@gram", //XQZ2Beta85
};

  int ix = 0;
  for (ix = 0; ix < sizeof(s_szBadCodes)/sizeof(s_szBadCodes[0]); ix++)
  {
      char szBuff[256];
      AgBase64Encode((unsigned char*)s_szBadCodes[ix],strlen(s_szBadCodes[ix]),szBuff);

      char szTest[512];
      sprintf(szTest,"\"%s\", //%s\n",szBuff,s_szBadCodes[ix]);
      OutputDebugString(szTest);
  }
*/