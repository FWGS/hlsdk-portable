// CrossThreadsMessagingDevice.cpp
// Made by Adi Degani - http://www.codeguru.com/network/irc.html

#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include "hud.h"
#include "cl_util.h"

#define ASSERT

#include "CrossThreadsMessagingDevice.h"


LPCSTR CCrossThreadsMessagingDevice::m_lpszClassName = "CCrossThreadsMessagingDevice_HiddenWindow";
int CCrossThreadsMessagingDevice::m_iCount = 0;


CCrossThreadsMessagingDevice::CCrossThreadsMessagingDevice()
	: m_hWnd(NULL), m_pMonitor(NULL)
{
	if( m_iCount++ == 0 )
	{
  	  const WNDCLASS wc = 
			{
				0,
				HiddenWindowProc,
				sizeof(DWORD) * 2,
				sizeof(DWORD) * 2,
				GetModuleHandle("client.dll"),
				(HICON)NULL,
				(HCURSOR)NULL,
				(HBRUSH)(COLOR_WINDOW + 1),
				(LPCSTR)NULL,
				m_lpszClassName
			};

		if( !RegisterClass(&wc) )
			return;
  }

  m_hWnd = CreateWindow(
			m_lpszClassName,
			"",
			WS_OVERLAPPED,
			0, 0, 0, 0,
			(HWND)NULL,
			(HMENU)NULL,
			GetModuleHandle("client.dll"),
			this
			);
}

CCrossThreadsMessagingDevice::~CCrossThreadsMessagingDevice()
{
	if( ::IsWindow(m_hWnd) )
		DestroyWindow(m_hWnd);

	if( --m_iCount == 0 )
	{
		UnregisterClass(m_lpszClassName, GetModuleHandle("client.dll"));
	}
}

void CCrossThreadsMessagingDevice::Post(WPARAM wParam, LPARAM lParam)
{
	ASSERT(::IsWindow(m_hWnd));
	PostMessage(m_hWnd, HWM_DATA, wParam, lParam);
}

LRESULT WINAPI CCrossThreadsMessagingDevice::HiddenWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT rc = 0;

	CCrossThreadsMessagingDevice* pThis = 
		(CCrossThreadsMessagingDevice*)GetWindowLong(hWnd, GWL_USERDATA);

	switch( uMsg )
	{
		case WM_NCCREATE :
		{
			LPCREATESTRUCT lpcs = (LPCREATESTRUCT)lParam;
			ASSERT(lpcs->lpCreateParams != NULL);
			SetWindowLong(hWnd, GWL_USERDATA, (LONG)lpcs->lpCreateParams);
			rc = TRUE;
			break;
		}

		case HWM_DATA :
		{
			ASSERT(pThis != NULL);
			if( pThis->m_pMonitor )
				pThis->m_pMonitor->OnCrossThreadsMessage(wParam, lParam);
			break;
		}

		default :
		{
			rc = DefWindowProc(hWnd, uMsg, wParam, lParam);
			break;
		}
	}

	return rc;
}

