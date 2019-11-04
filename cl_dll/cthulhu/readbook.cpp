/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// battery.cpp
//
// implementation of CHudReadBook class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_TeamFortressViewport.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_ReadBook, ReadBook)

int CHudReadBook::Init(void)
{
	ConsolePrint ("Initialising ReadBook\n");

	pImage = NULL;

	HOOK_MESSAGE(ReadBook);

	gHUD.AddHudElem(this);

	InitHUDData();

	return 1;
};


int CHudReadBook::VidInit(void)
{
	return 1;
};

int CHudReadBook:: MsgFunc_ReadBook(const char *pszName,  int iSize, void *pbuf )
{
	ConsolePrint ("Beginning ReadBook\n");
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	char* szName = READ_STRING();

	if (strcmp(szName,"") == 0)
	{
		ConsolePrint ("Beginning remove ReadBook Image\n");
		if (pImage)
		{
			ConsolePrint ("Removing ReadBook Image\n");
			gViewPort->removeChild(pImage);
			// I think the deletion is managed by the framework...anyway, I do not get a memory leak
			// but if I keep the delete, then I get an exception ONLY in 640x480. Go figure...
			//pImage->setParent(NULL);
			//delete pImage;
			pImage = NULL;
		}
	}
	else
	{
		ConsolePrint ("Beginning create ReadBook Image\n");
		if (pImage == NULL)
		{
			ConsolePrint ("Creating ReadBook Image\n");
			ConsolePrint ("Image name : ");
			ConsolePrint (szName);
			ConsolePrint ("\n");
			pImage = new CImageLabel(szName,0,YRES(30));
			pImage->setParent(gViewPort);
		}
	}

	return 1;
}

