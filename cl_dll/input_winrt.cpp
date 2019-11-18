/*
input_winrt.cpp
Copyright (C) 2019 Moemod Haoyuan

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#define HSPRITE HSPRITE_win32
#include <wrl.h>
#include <Windows.Devices.Input.h>
#include <Windows.UI.Core.h>
#undef HSPRITE

#include "hud.h"
#include "kbutton.h"
#include "cvardef.h"
#include "usercmd.h"
#include "in_defs.h"
#include "keydefs.h"
#include "input_mouse.h"

#include <atomic>
#include <utility>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::Devices::Input;

#define MOUSE_BUTTON_COUNT 5

// Set this to 1 to show mouse cursor.  Experimental
int	g_iVisibleMouse = 0;

extern cl_enginefunc_t gEngfuncs;

extern int iMouseInUse;

extern kbutton_t	in_strafe;
extern kbutton_t	in_mlook;
extern kbutton_t	in_speed;
extern kbutton_t	in_jlook;


extern cvar_t	*m_pitch;
extern cvar_t	*m_yaw;
extern cvar_t	*m_forward;
extern cvar_t	*m_side;

extern cvar_t *lookstrafe;
extern cvar_t *lookspring;
extern cvar_t *cl_pitchdown;
extern cvar_t *cl_pitchup;
extern cvar_t *cl_yawspeed;
extern cvar_t *cl_sidespeed;
extern cvar_t *cl_forwardspeed;
extern cvar_t *cl_pitchspeed;
extern cvar_t *cl_movespeedkey;

// mouse variables
extern cvar_t *sensitivity;
extern cvar_t *in_joystick;

static int	mouse_oldbuttonstate;
static bool	mouseactive;
static bool	mouseinitialized;

struct alignas(8) MyMouseDelta
{
	int X;
	int Y;
};

static std::atomic<MyMouseDelta> g_atomicMouseDelta;
static ComPtr<ICoreWindow> g_pCoreWindow;
static ComPtr<ICoreWindow2> g_pCoreWindow2;
static ComPtr<IMouseDevice> g_pMouseDevice;
static EventRegistrationToken g_MouseMovedEventToken;

void WinRTInput::WinRT_RestrictMouse()
{
	HRESULT hr;
	hr = g_pCoreWindow->SetPointerCapture();
}

void WinRTInput::WinRT_ReleaseMouse()
{
	HRESULT hr;
	hr = g_pCoreWindow->ReleasePointerCapture();
}

/*
===========
IN_ActivateMouse
===========
*/
void WinRTInput::IN_ActivateMouse(void)
{
	if (mouseinitialized)
	{
		mouseactive = 1;
		WinRT_RestrictMouse();
	}
}

/*
===========
IN_DeactivateMouse
===========
*/
void WinRTInput::IN_DeactivateMouse(void)
{
	if (mouseinitialized)
	{
		mouseactive = 0;
		WinRT_ReleaseMouse();
	}
}

/*
===========
IN_StartupMouse
===========
*/
void WinRTInput::IN_StartupMouse (void)
{
	if ( gEngfuncs.CheckParm ("-nomouse", NULL ) ) 
		return; 

	mouseinitialized = 1;
	g_atomicMouseDelta.store({ 0, 0 });
}

/*
===========
IN_ResetMouseF

FIXME: Call through to engine?
===========
*/
void WinRTInput::IN_ResetMouse( void )
{
	//SetCursorPos ( gEngfuncs.GetWindowCenterX(), gEngfuncs.GetWindowCenterY() );

	HRESULT hr;

	Rect bounds;
	hr = g_pCoreWindow->get_Bounds(&bounds);
	
	g_pCoreWindow2->put_PointerPosition({ bounds.X + bounds.Width / 2, bounds.Y + bounds.Height / 2 });
}

/*
===========
IN_MouseEvent
===========
*/
void WinRTInput::IN_MouseEvent(int mstate)
{
	if ( iMouseInUse || g_iVisibleMouse )
		return;

	// perform button actions
	for (int i=0 ; i< MOUSE_BUTTON_COUNT; i++)
	{
		if ( (mstate & (1<<i)) &&
			!(mouse_oldbuttonstate & (1<<i)) )
		{
			gEngfuncs.Key_Event (K_MOUSE1 + i, 1);
		}

		if ( !(mstate & (1<<i)) &&
			(mouse_oldbuttonstate & (1<<i)) )
		{
			gEngfuncs.Key_Event (K_MOUSE1 + i, 0);
		}
	}	
	
	mouse_oldbuttonstate = mstate;
}

/*
===========
IN_MouseMove
===========
*/
void WinRTInput::IN_MouseMove ( float frametime, usercmd_t *cmd)
{
	vec3_t viewangles;

	gEngfuncs.GetViewAngles( (float *)viewangles );

	//jjb - this disbles normal mouse control if the user is trying to 
	//      move the camera, or if the mouse cursor is visible or if we're in intermission
	if ( !iMouseInUse && !g_iVisibleMouse && !gHUD.m_iIntermission )
	{
		MyMouseDelta dxdy = g_atomicMouseDelta.exchange({0, 0});
		
		int mouse_x = dxdy.X;
		int mouse_y = dxdy.Y;

		if ( gHUD.GetSensitivity() != 0 )
		{
			mouse_x *= gHUD.GetSensitivity();
			mouse_y *= gHUD.GetSensitivity();
		}
		else
		{
			mouse_x *= sensitivity->value;
			mouse_y *= sensitivity->value;
		}

		// add mouse X/Y movement to cmd
		if ( (in_strafe.state & 1) || (lookstrafe->value && (in_mlook.state & 1) ))
			cmd->sidemove += m_side->value * mouse_x;
		else
			viewangles[YAW] -= m_yaw->value * mouse_x;

		if ( (in_mlook.state & 1) && !(in_strafe.state & 1))
		{
			viewangles[PITCH] += m_pitch->value * mouse_y;
			if (viewangles[PITCH] > cl_pitchdown->value)
				viewangles[PITCH] = cl_pitchdown->value;
			if (viewangles[PITCH] < -cl_pitchup->value)
				viewangles[PITCH] = -cl_pitchup->value;
		}
		else
		{
			if ((in_strafe.state & 1) && gEngfuncs.IsNoClipping() )
			{
				cmd->upmove -= m_forward->value * mouse_y;
			}
			else
			{
				cmd->forwardmove -= m_forward->value * mouse_y;
			}
		}

		// if the mouse has moved, force it to the center, so there's room to move
		if (mouse_x || mouse_y)
		{
			IN_ResetMouse();
		}
	}

	gEngfuncs.SetViewAngles( (float *)viewangles );

}

/*
===========
IN_Accumulate
===========
*/
void WinRTInput::IN_Accumulate (void)
{
	if (!mouseactive)
		return;
	
	IN_ResetMouse();
}

/*
===================
IN_ClearStates
===================
*/
void WinRTInput::IN_ClearStates (void)
{
	if ( !mouseactive )
		return;

	mouse_oldbuttonstate = 0;
}

/*
===========
IN_Commands
===========
*/
void WinRTInput::IN_Commands(void)
{

}

/*
===========
IN_Move
===========
*/
void WinRTInput::IN_Move ( float frametime, usercmd_t *cmd)
{
	if ( !iMouseInUse && mouseactive )
	{
		IN_MouseMove ( frametime, cmd);
	}
}

// MoeMod : may call by system from another thread
HRESULT WinRT_OnMouseMoved(IMouseDevice *sender, IMouseEventArgs *e)
{
	HRESULT hr;
	
	MouseDelta now_dxdy;
	hr = e->get_MouseDelta(&now_dxdy);
	MyMouseDelta acc_dxdy;

	MyMouseDelta old_dxdy = g_atomicMouseDelta.load();
	do
	{
		acc_dxdy.X = now_dxdy.X + old_dxdy.X;
		acc_dxdy.Y = now_dxdy.Y + old_dxdy.Y;
		// CAS in case that IN_MouseMove already handled old_dxdy
	} while (!g_atomicMouseDelta.compare_exchange_weak(old_dxdy, acc_dxdy));
	
	return S_OK;
}

/*
===========
IN_Init
===========
*/
void WinRTInput::IN_Init (void)
{
	sensitivity				= gEngfuncs.pfnRegisterVariable ( "sensitivity","3", FCVAR_ARCHIVE ); // user mouse sensitivity setting.

	IN_StartupMouse();

	in_joystick				= gEngfuncs.pfnRegisterVariable ( "joystick","0", FCVAR_ARCHIVE );

	HRESULT hr;
	ComPtr<ICoreWindowStatic> coreWindowStatic;
	hr = Windows::Foundation::GetActivationFactory(
		HStringReference(RuntimeClass_Windows_UI_Core_CoreWindow).Get(),
		&coreWindowStatic);

	ComPtr<ICoreWindow> coreWindow;
	hr = coreWindowStatic->GetForCurrentThread(&coreWindow);
	g_pCoreWindow = coreWindow;
	
	ComPtr<ICoreWindow2> coreWindow2;
	hr = coreWindow.As(&coreWindow2);
	g_pCoreWindow2 = coreWindow2;

	ComPtr<IMouseDeviceStatics> mouseDeviceStatics;
	hr = Windows::Foundation::GetActivationFactory(
		HStringReference(RuntimeClass_Windows_Devices_Input_MouseDevice).Get(),
		&mouseDeviceStatics);

	ComPtr<IMouseDevice> mouseDevice;
	hr = mouseDeviceStatics->GetForCurrentView(&mouseDevice);
	g_pMouseDevice = mouseDevice;

	auto callback = Callback<__FITypedEventHandler_2_Windows__CDevices__CInput__CMouseDevice_Windows__CDevices__CInput__CMouseEventArgs>(WinRT_OnMouseMoved);
	hr = g_pMouseDevice->add_MouseMoved(callback.Get(), &g_MouseMovedEventToken);
	
}

/*
===========
IN_Shutdown
===========
*/
void WinRTInput::IN_Shutdown(void)
{
	IN_DeactivateMouse();

	HRESULT hr;
	hr = g_pMouseDevice->remove_MouseMoved(std::exchange(g_MouseMovedEventToken, {}));
}
