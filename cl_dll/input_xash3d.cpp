#include "hud.h"
#include "usercmd.h"
#include "cvardef.h"
#include "kbutton.h"
#include "keydefs.h"
cvar_t		*sensitivity;
cvar_t		*in_joystick;
#define	PITCH	0
#define	YAW		1
#define	ROLL	2 

extern "C"  void DLLEXPORT IN_ClientMoveEvent( float forwardmove, float sidemove );
extern "C" void DLLEXPORT IN_ClientLookEvent( float relyaw, float relpitch );

extern kbutton_t	in_strafe;
extern kbutton_t	in_mlook;
extern kbutton_t	in_speed;
extern kbutton_t	in_jlook;
extern kbutton_t	in_forward;
extern kbutton_t	in_back;
extern kbutton_t	in_moveleft;
extern kbutton_t	in_moveright;

extern cvar_t	*m_pitch;
extern cvar_t	*m_yaw;
extern cvar_t	*m_forward;
extern cvar_t	*m_side;
extern cvar_t	*lookstrafe;
extern cvar_t	*lookspring;
extern cvar_t	*cl_pitchdown;
extern cvar_t	*cl_pitchup;
extern cvar_t	*cl_yawspeed;
extern cvar_t	*cl_sidespeed;
extern cvar_t	*cl_forwardspeed;
extern cvar_t	*cl_pitchspeed;
extern cvar_t	*cl_movespeedkey;
cvar_t	*cl_laddermode;


float ac_forwardmove;
float ac_sidemove;
int ac_movecount;
float rel_yaw;
float rel_pitch;

#define F 1U<<0	// Forward
#define B 1U<<1	// Back
#define L 1U<<2	// Left
#define R 1U<<3	// Right
#define T 1U<<4	// Forward stop
#define S 1U<<5	// Side stop

#define BUTTON_DOWN		1
#define IMPULSE_DOWN	2
#define IMPULSE_UP		4

int CL_IsDead( void );
Vector dead_viewangles(0, 0, 0);

void IN_ToggleButtons( float forwardmove, float sidemove )
{
	static unsigned int moveflags = T | S;

	if( forwardmove )
		moveflags &= ~T;
	else
	{
		//if( in_forward.state || in_back.state ) gEngfuncs.Con_Printf("Buttons pressed f%d b%d\n", in_forward.state, in_back.state);
		if( !( moveflags & T ) )
		{
			//IN_ForwardUp();
			//IN_BackUp();
			//gEngfuncs.Con_Printf("Reset forwardmove state f%d b%d\n", in_forward.state, in_back.state);
			in_forward.state &= ~BUTTON_DOWN;
			in_back.state &= ~BUTTON_DOWN;
			moveflags |= T;
		}
	}
	if( sidemove )
		moveflags &= ~S;
	else
	{
		//gEngfuncs.Con_Printf("l%d r%d\n", in_moveleft.state, in_moveright.state);
		//if( in_moveleft.state || in_moveright.state ) gEngfuncs.Con_Printf("Buttons pressed l%d r%d\n", in_moveleft.state, in_moveright.state);
		if( !( moveflags & S ) )
		{
			//IN_MoverightUp();
			//IN_MoveleftUp();
			//gEngfuncs.Con_Printf("Reset sidemove state f%d b%d\n", in_moveleft.state, in_moveright.state);
			in_moveleft.state &= ~BUTTON_DOWN;
			in_moveright.state &= ~BUTTON_DOWN;
			moveflags |= S;
		}
	}

	if( forwardmove > 0.7 && !( moveflags & F ) )
	{
		moveflags |= F;
		in_forward.state |= BUTTON_DOWN;
	}
	if( forwardmove < 0.7 && ( moveflags & F ) )
	{
		moveflags &= ~F;
		in_forward.state &= ~BUTTON_DOWN;
	}
	if( forwardmove < -0.7 && !( moveflags & B ) )
	{
		moveflags |= B;
		in_back.state |= BUTTON_DOWN;
	}
	if( forwardmove > -0.7 && ( moveflags & B ) )
	{
		moveflags &= ~B;
		in_back.state &= ~BUTTON_DOWN;
	}
	if( sidemove > 0.9 && !( moveflags & R ) )
	{
		moveflags |= R;
		in_moveright.state |= BUTTON_DOWN;
	}
	if( sidemove < 0.9 && ( moveflags & R ) )
	{
		moveflags &= ~R;
		in_moveright.state &= ~BUTTON_DOWN;
	}
	if( sidemove < -0.9 && !( moveflags & L ) )
	{
		moveflags |= L;
		in_moveleft.state |= BUTTON_DOWN;
	}
	if( sidemove > -0.9 && ( moveflags & L ) )
	{
		moveflags &= ~L;
		in_moveleft.state &= ~BUTTON_DOWN;
	}
}

void IN_ClientMoveEvent( float forwardmove, float sidemove )
{
	//gEngfuncs.Con_Printf("IN_MoveEvent\n");

	ac_forwardmove += forwardmove;
	ac_sidemove += sidemove;
	ac_movecount++;
}

void IN_ClientLookEvent( float relyaw, float relpitch )
{
	rel_yaw += relyaw;
	rel_pitch += relpitch;
}

// Rotate camera and add move values to usercmd
void IN_Move( float frametime, usercmd_t *cmd )
{
	Vector viewangles;
	bool fLadder = false;

	if( gHUD.m_iIntermission )
		return; // we can't move during intermission

	if( cl_laddermode->value != 2 )
	{
		cl_entity_t *pplayer = gEngfuncs.GetLocalPlayer();
		if( pplayer )
			fLadder = pplayer->curstate.movetype == MOVETYPE_FLY;
	}
	//if(ac_forwardmove || ac_sidemove)
	//gEngfuncs.Con_Printf("Move: %f %f %f %f\n", ac_forwardmove, ac_sidemove, rel_pitch, rel_yaw);
#if 0
	if( in_mlook.state & 1 )
	{
		V_StopPitchDrift();
	}
#endif
	if( CL_IsDead() )
	{
		viewangles = dead_viewangles; // HACKHACK: see below
	}
	else
	{
		gEngfuncs.GetViewAngles( viewangles );
	}
	if( gHUD.GetSensitivity() != 0 )
	{
		rel_yaw *= gHUD.GetSensitivity();
		rel_pitch *= gHUD.GetSensitivity();
	}
	else
	{
		rel_yaw *= sensitivity->value;
		rel_pitch *= sensitivity->value;
	}
	viewangles[YAW] += rel_yaw;
	if( fLadder )
	{
		if( cl_laddermode->value == 1 )
			viewangles[YAW] -= ac_sidemove * 5;
		ac_sidemove = 0;
	}
	if( gHUD.m_MOTD.m_bShow )
		gHUD.m_MOTD.scroll += rel_pitch;
	else
		viewangles[PITCH] += rel_pitch;

	if( viewangles[PITCH] > cl_pitchdown->value )
		viewangles[PITCH] = cl_pitchdown->value;
	if( viewangles[PITCH] < -cl_pitchup->value )
		viewangles[PITCH] = -cl_pitchup->value;
	
	// HACKHACK: change viewangles directly in viewcode, 
	// so viewangles when player is dead will not be changed on server
	if( !CL_IsDead() )
	{
		gEngfuncs.SetViewAngles( viewangles );
	}

	dead_viewangles = viewangles; // keep them actual
	if( ac_movecount )
	{
		IN_ToggleButtons( ac_forwardmove / ac_movecount, ac_sidemove / ac_movecount );

		if( ac_forwardmove )
			cmd->forwardmove = ac_forwardmove * cl_forwardspeed->value / ac_movecount;
		if( ac_sidemove )
			cmd->sidemove  = ac_sidemove * cl_sidespeed->value / ac_movecount;
		if( ( in_speed.state & 1 ) && ( ac_sidemove || ac_forwardmove ) )
		{
			cmd->forwardmove *= cl_movespeedkey->value;
			cmd->sidemove *= cl_movespeedkey->value;
		}
	}

	ac_sidemove = ac_forwardmove = rel_pitch = rel_yaw = 0;
	ac_movecount = 0;
}

extern "C" void DLLEXPORT IN_MouseEvent( int mstate )
{
	static int mouse_oldbuttonstate;
	// perform button actions
	for( int i = 0; i < 5; i++ )
	{
		if( ( mstate & ( 1 << i ) ) && !( mouse_oldbuttonstate & ( 1 << i ) ) )
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 1 );
		}

		if( !( mstate & ( 1 << i ) ) && ( mouse_oldbuttonstate & ( 1 << i ) ) )
		{
			gEngfuncs.Key_Event( K_MOUSE1 + i, 0 );
		}
	}	

	mouse_oldbuttonstate = mstate;
}

// Stubs

extern "C" void DLLEXPORT IN_ClearStates( void )
{
	//gEngfuncs.Con_Printf( "IN_ClearStates\n" );
}

extern "C" void DLLEXPORT IN_ActivateMouse( void )
{
	//gEngfuncs.Con_Printf( "IN_ActivateMouse\n" );
}

extern "C" void DLLEXPORT IN_DeactivateMouse( void )
{
	//gEngfuncs.Con_Printf( "IN_DeactivateMouse\n" );
}

extern "C" void DLLEXPORT IN_Accumulate( void )
{
	//gEngfuncs.Con_Printf( "IN_Accumulate\n" );
}

void IN_Commands( void )
{
	//gEngfuncs.Con_Printf( "IN_Commands\n" );
}

void IN_Shutdown( void )
{
}

// Register cvars and reset data
void IN_Init( void )
{
	sensitivity = gEngfuncs.pfnRegisterVariable( "sensitivity", "3", FCVAR_ARCHIVE );
	in_joystick = gEngfuncs.pfnRegisterVariable( "joystick", "0", FCVAR_ARCHIVE );
	cl_laddermode = gEngfuncs.pfnRegisterVariable( "cl_laddermode", "2", FCVAR_ARCHIVE );
	ac_forwardmove = ac_sidemove = rel_yaw = rel_pitch = 0;
}
