//++ BulliT
#include "agmapi.h"
#undef EXPORT
#include <windows.h>
#include <mapi.h>

bool AgSendMail(const char* pszMessage)
{
	// Get instance handle of MAPI32.DLL
	HINSTANCE hlibMAPI = LoadLibrary ("MAPI32.dll");
	if (!hlibMAPI)
		return false;

	//  Get the addresses of sendmail api
	LPMAPISENDMAIL lpfMAPISendMail = (LPMAPISENDMAIL)GetProcAddress(hlibMAPI, "MAPISendMail");
	if (!lpfMAPISendMail)
		return false;

	MapiMessage Message;
	ZeroMemory(&Message,sizeof(Message));
	Message.lpszSubject = "AGMOD Cheatdetection";
	Message.lpszNoteText = (char*)pszMessage;
	
	MapiRecipDesc arMailRecipients[1];
	ZeroMemory(&arMailRecipients,sizeof(arMailRecipients));
	arMailRecipients[0].lpszName	  = "BulliT";
	arMailRecipients[0].lpszAddress  = "SMTP:Martin Webrant";
	arMailRecipients[0].ulRecipClass = MAPI_TO;

	Message.nRecipCount = 1;
	Message.lpRecips = arMailRecipients;

	ULONG result = lpfMAPISendMail(NULL,NULL,&Message,0,0);
	if (SUCCESS_SUCCESS == result)
		return true;

	return false;
}
//-- Martin Webrant
