//++ BulliT
#include "agmapi.h"
#include <windows.h>

typedef BOOL   (WINAPI* ICQAPICall_SetLicenseKey)(char*, char*, char*);
typedef BOOL   (WINAPI* ICQAPICall_SendMessage)(int, char*);

bool AgSendICQ(const char* pszMessage)
{
	// Get instance handle of MAPI32.DLL
	HINSTANCE hlibICQMAPI = LoadLibrary("ICQMAPI.dll");
	if (!hlibICQMAPI)
		return false;

	//  Get the addresses of sendmail api
	ICQAPICall_SetLicenseKey lpfSetLicenseKey = (ICQAPICall_SetLicenseKey)GetProcAddress(hlibICQMAPI, "ICQAPICall_SetLicenseKey");
	if (!lpfSetLicenseKey)
		return false;

  ICQAPICall_SendMessage lpfSendMessage = (ICQAPICall_SendMessage)GetProcAddress(hlibICQMAPI, "ICQAPICall_SendMessage");
  if (!lpfSendMessage)
		return false;

  lpfSetLicenseKey("[pmers]", "pmersbullit", "3EB699A36502539C");
  lpfSendMessage(13243715,(char*)pszMessage);
  return true;
}
//-- Martin Webrant
