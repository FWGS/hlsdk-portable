/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
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
// status_icons.cpp
//
#include "hud.h"
#include "cl_util.h"
#include "draw_util.h"
#include "com_model.h"
#include "triangleapi.h"
#include "r_efx.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "entity_types.h"
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "parsemsg.h"
#include "event_api.h"
#include "com_weapons.h"
#include "pm_defs.h"
#include "pmtrace.h"

int CHudStatusIcons::Init( void )
{
	HOOK_MESSAGE( gHUD.m_StatusIcons, StatusIcon );
	HOOK_MESSAGE( gHUD.m_StatusIcons, HudIcons );
	HOOK_MESSAGE( gHUD.m_StatusIcons, CzTool );
	HOOK_MESSAGE( gHUD.m_StatusIcons, CzFiber );

	gHUD.AddHudElem( this );

	Reset();

	return 1;
}

int CHudStatusIcons::VidInit( void )
{
	return 1;
}

int g_czTool = 0;
static int   g_czCount = 0, g_czHold = 0;
static float g_czPlateT = 0;
static int   g_czZoneShown = 0;
int g_czFiber = 0;
static int   g_czZoom = 0;

void CHudStatusIcons::Reset( void )
{
	g_czTool = 0; g_czFiber = 0; g_czZoom = 0; g_czZoneShown = 0; g_czPlateT = 0;
	memset( m_IconList, 0, sizeof m_IconList );
	m_iFlags &= ~HUD_DRAW;
}

// ---- CZ mission tools HUD ----

bool CZ_HasTool() { return g_czTool != 0 && g_czTool != 4; } // the mine is a real weapon: key 5 selects it natively

int CHudStatusIcons::MsgFunc_CzTool( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int tool = reader.ReadByte();
	g_czCount = reader.ReadByte();
	g_czHold = reader.ReadByte();
	// the "EQUIPMENT" plate only when the server says the tool was just picked up / given; a load, a HUD
	// reset or a resend carries the tool the player already had (tests 30.10: the plate popped up after
	// loading the autosave and on picking up ammo)
	int announce = reader.ReadByte();
	if( announce && tool && tool != g_czTool )
		g_czPlateT = gHUD.m_flTime + 3.0f;
	g_czTool = tool;
	m_iFlags |= HUD_DRAW;
	return 1;
}

int CHudStatusIcons::MsgFunc_CzFiber( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	g_czFiber = reader.ReadByte();
	g_czZoom = reader.ReadByte();
	m_iFlags |= HUD_DRAW;
	return 1;
}

static void CZ_DrawSprite( const char *name, int x, int y, int r, int g, int b )
{
	int idx = gHUD.GetSpriteIndex( name );
	if( idx < 0 )
		return;
	HSPRITE spr = gHUD.GetSprite( idx );
	if( !spr )
		return;
	SPR_Set( spr, r, g, b );
	SPR_DrawAdditive( 0, x, y, &gHUD.GetSpriteRect( idx ) );
}

static void CZ_DrawTools( float flTime )
{
	static const char *toolIcons[] = { "", "icon_camera", "icon_fibercamera", "icon_blowtorch", "icon_radiocontrolledbomb", "icon_radio" };

	// fiber optic camera: round red-tinted view with scan lines
	if( g_czFiber )
	{
		int W = ScreenWidth, H = ScreenHeight;
		int cx = W / 2, cy = H / 2;
		int R = (int)( ( W < H ? W : H ) * 0.46f );
		FillRGBABlend( 0, 0, W, H, 220, 0, 0, 40 );
		const int step = 3;
		for( int y = 0; y < H; y += step )
		{
			int dy = y + step / 2 - cy;
			int half = 0;
			if( dy > -R && dy < R )
				half = (int)sqrtf( (float)( R * R - dy * dy ) );
			if( half <= 0 )
			{
				FillRGBABlend( 0, y, W, step, 0, 0, 0, 255 );
				continue;
			}
			FillRGBABlend( 0, y, cx - half, step, 0, 0, 0, 255 );
			FillRGBABlend( cx + half, y, W - ( cx + half ), step, 0, 0, 0, 255 );
			// scan line
			FillRGBABlend( cx - half, y, half * 2, 1, 0, 0, 0, 70 );
		}
		return;
	}

	// "you are in a ... zone" hint
	if( g_czZoneShown )
	{
		static const struct { int bit; int tool; const char *text; } zones[] =
		{
			{ 4,   1, "You are in a camera zone" },
			{ 8,   2, "You are in a fiber optic camera zone" },
			{ 16,  5, "You are in a radio zone" },
			{ 32,  4, "You are in a radio controlled bomb zone" },
			{ 64,  3, "You are in a blowtorch zone" },
			{ 512, 0, "You are in a bomb defuse zone" },
		};
		for( size_t i = 0; i < sizeof( zones ) / sizeof( zones[0] ); i++ )
		{
			if( ( g_czZoneShown & zones[i].bit ) && ( !zones[i].tool || g_czTool == zones[i].tool ) )
			{
				int len = DrawUtils::HudStringLen( zones[i].text );
				DrawUtils::DrawHudString( ScreenWidth / 2 - len / 2, (int)( ScreenHeight * 0.14f ), ScreenWidth, zones[i].text, 90, 150, 255 );
				break;
			}
		}
	}

	if( g_czTool < 1 || g_czTool > 5 )
		return;

	// "EQUIPMENT" plate: slides in from the right, holds, fades out
	if( flTime < g_czPlateT )
	{
		const float showT = 3.0f, inT = 0.30f, outT = 0.6f;
		float age = showT - ( g_czPlateT - flTime );
		float left = g_czPlateT - flTime;
		float slide = age < inT ? 1.0f - age / inT : 0.0f;
		slide = slide * slide;
		float fade = left < outT ? left / outT : 1.0f;
		if( fade < 0 ) fade = 0;
		if( age < 0 ) age = 0;

		// layout from real pixel sizes (text, sprite) so it looks the same at any resolution
		int iconW = 32, iconH = 32;
		{
			int idx = gHUD.GetSpriteIndex( toolIcons[g_czTool] );
			if( idx >= 0 )
			{
				wrect_t rc = gHUD.GetSpriteRect( idx );
				iconW = rc.right - rc.left;
				iconH = rc.bottom - rc.top;
			}
		}
		int pad = 12, gap = 28;
		int textW = DrawUtils::HudStringLen( "EQUIPMENT" );
		int numW = 0;
		if( g_czTool == 4 )
			numW = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).right - gHUD.GetSpriteRect( gHUD.m_HUD_number_0 ).left + 10;
		int w = 3 + pad + textW + gap + numW + iconW + pad;
		int h = iconH + 16;
		int margin = 0;
		int x = ScreenWidth - w - margin + (int)( slide * ( w + margin ) );
		int y = ScreenHeight - 150 - h - ( g_czTool == 4 ? 70 : 0 );
		int r = 50 * fade, g = 250 * fade, b2 = 50 * fade;

		FillRGBABlend( x, y, w, h, 0, 0, 0, (int)( 120 * fade ) );          // dark glass
		FillRGBABlend( x, y, 3, h, 50, 250, 50, (int)( 230 * fade ) );       // accent bar
		FillRGBABlend( x, y, w, 1, 50, 250, 50, (int)( 90 * fade ) );        // thin top / bottom lines
		FillRGBABlend( x, y + h - 1, w, 1, 50, 250, 50, (int)( 90 * fade ) );
		DrawUtils::DrawHudString( x + 3 + pad, y + h / 2 - 8, ScreenWidth, "EQUIPMENT", r, g, b2 );
		if( g_czTool == 4 )
			DrawUtils::DrawHudNumberString( x + w - pad - iconW - numW + 4, y + h / 2 - 12, 0, g_czCount, r, g, b2 );
		CZ_DrawSprite( toolIcons[g_czTool], x + w - pad - iconW, y + ( h - iconH ) / 2, r, g, b2 );
	}

	// mines: icon and counter in the bottom right corner
	if( g_czTool == 4 )
	{
		int x = ScreenWidth - 150, y = ScreenHeight - 70;
		// the icon and the counter come from the native weapon HUD (the mine is a real weapon now)
		if( g_czHold )
			DrawUtils::DrawHudString( x - 4, y - 14, ScreenWidth, "DETONATOR", 50, 250, 50 );
	}
}

void CZ_DrawTorchSprites( void );

// Draw status icons along the left-hand side of the screen
int CHudStatusIcons::Draw( float flTime )
{
	if (gEngfuncs.IsSpectateOnly())
		return 1;

	CZ_DrawTools( flTime );
	CZ_DrawTorchSprites();
	// find starting position to draw from, along right-hand side of screen
	int x = 5;
	int y = ScreenHeight / 2;

	// loop through icon list, and draw any valid icons drawing up from the middle of screen
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( m_IconList[i].spr )
		{
			y -= ( m_IconList[i].rc.Height() ) + 5;
			
			if( g_bInBombZone && !strcmp(m_IconList[i].szSpriteName, "c4") && ((int)(flTime * 10) % 2))
				SPR_Set( m_IconList[i].spr, 255, 16, 16 );
			else SPR_Set( m_IconList[i].spr, m_IconList[i].r, m_IconList[i].g, m_IconList[i].b );
			SPR_DrawAdditive( 0, x, y, &m_IconList[i].rc );
		}
	}
	
	return 1;
}

// Message handler for StatusIcon message
// accepts five values:
//		byte   : TRUE = ENABLE icon, FALSE = DISABLE icon
//		string : the sprite name to display
//		byte   : red
//		byte   : green
//		byte   : blue
int CHudStatusIcons::MsgFunc_StatusIcon( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );

	int ShouldEnable = reader.ReadByte();
	char *pszIconName = reader.ReadString();

	if ( ShouldEnable )
	{
		int r = reader.ReadByte();
		int g = reader.ReadByte();
		int b = reader.ReadByte();
		EnableIcon( pszIconName, r, g, b );
		m_iFlags |= HUD_DRAW;
	}
	else
	{
		DisableIcon( pszIconName );
	}

	return 1;
}

// CZ: HudIcons = 3 shorts. First = icon mask to show, second/third = tools the player owns.
int CHudStatusIcons::MsgFunc_HudIcons( const char *pszName, int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int shown = reader.ReadShort();
	int active = reader.ReadShort();
	int owned = reader.ReadShort();
	g_czZoneShown = active & ( owned | 512 );
	extern int g_czToolsOwned;
	g_czToolsOwned = owned;
	gEngfuncs.Con_DPrintf( "HudIcons: shown %d active %d owned %d\n", shown, active, owned );

	static const struct { int bit; const char *name; } icons[] =
	{
		{ 1,    "icon_stealth" },
		{ 4,    "icon_camera" },
		{ 8,    "icon_fibercamera" },
		{ 16,   "icon_radio" },
		{ 32,   "icon_radiocontrolledbomb" },
		{ 64,   "icon_blowtorch" },
		{ 512,  "icon_bombdefuse" },
		{ 1024, "icon_hostagezone" },
	};

	for( size_t k = 0; k < sizeof( icons ) / sizeof( icons[0] ); k++ )
	{
		// bomb room: only the defuse kit icon, the radio icon goes away
		if( ( shown & icons[k].bit ) && !( icons[k].bit == 16 && ( shown & 512 )))
		{
			// tools: green while the player is inside their zone, orange when owned outside it (original CHudIcons::Draw)
			if( icons[k].bit == 1 )
				EnableIcon( icons[k].name, 160, 160, 160 ); // stealth: colourless grey
			else if( ( ( active & icons[k].bit ) && ( owned & icons[k].bit ) ) || icons[k].bit == 512 || icons[k].bit == 1024 )
				EnableIcon( icons[k].name, 50, 250, 50 );
			else if( ( active | owned ) & icons[k].bit )
				EnableIcon( icons[k].name, 255, 128, 0 ); // original CHudIcons::Draw: tool owned but not in its zone (or the other way round) = orange
			else
				EnableIcon( icons[k].name, 255, 0, 0 );
			m_iFlags |= HUD_DRAW;
		}
		else
			DisableIcon( icons[k].name );
	}

	return 1;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon( const char *pszIconName, unsigned char red, unsigned char green, unsigned char blue )
{
	// check to see if the sprite is in the current list
	int i;
	for ( i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
			break;
	}

	if ( i == MAX_ICONSPRITES )
	{
		// icon not in list, so find an empty slot to add to
		for ( i = 0; i < MAX_ICONSPRITES; i++ )
		{
			if ( !m_IconList[i].spr )
				break;
		}
	}

	// if we've run out of space in the list, overwrite the first icon
	if ( i == MAX_ICONSPRITES )
	{
		i = 0;
	}

	// Load the sprite and add it to the list
	// the sprite must be listed in hud.txt
	int spr_index = -1;
	{
		// czeror hud.txt uses different icon names
		static const char *alias[][2] = { {"rescue","icon_hostagezone"}, {"defuser","icon_bombdefuse"}, {"nvg","icon_nightvision"} };
		for( int a = 0; a < 3; a++ )
			if( !strcmp( pszIconName, alias[a][0] ) && !strcmp( gEngfuncs.pfnGetGameDirectory(), "czeror" ) )
				spr_index = gHUD.GetSpriteIndex( alias[a][1] );
	}
	if( spr_index < 0 )
		spr_index = gHUD.GetSpriteIndex( pszIconName );
	if( spr_index < 0 )
		return; // no such sprite in this game's hud.txt: do not draw garbage
	m_IconList[i].spr = gHUD.GetSprite( spr_index );
	if( !m_IconList[i].spr )
		gEngfuncs.Con_Printf( "StatusIcon: sprite for '%s' is not loaded (index %d)\n", pszIconName, spr_index );
	m_IconList[i].rc = gHUD.GetSpriteRect( spr_index );
	m_IconList[i].r = red;
	m_IconList[i].g = green;
	m_IconList[i].b = blue;
	strncpy( m_IconList[i].szSpriteName, pszIconName, MAX_ICONSPRITENAME_LENGTH );
	m_IconList[i].szSpriteName[MAX_ICONSPRITENAME_LENGTH-1]=0;
}

void CHudStatusIcons::DisableIcon( const char *pszIconName )
{
	// find the sprite is in the current list
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
		{
			// clear the item from the list
			memset( &m_IconList[i], 0, sizeof( icon_sprite_t ) );
			return;
		}
	}
}


// blowtorch flame: two camera facing sprites (like env_glow) drawn in the transparent pass, after the view model.
// cvars cz_torch1_* and cz_torch2_*: spr (sprite name), fwd/right/up (offset from the nozzle, units), size (half size, 0.1 unit steps), alpha
extern vec3_t g_czTorchPos, v_angles;
extern bool g_czTorchOn, g_czTorchAtk;
extern float g_czTorchSize;

// Blowtorch flame: two sprites (core + halo) projected onto the screen and drawn in the 2D pass,
// so the view model cannot hide them with its depth.
// cz_torch_mode 0 = native-size HUD sprite draw (SPR_DrawAdditive), 1 = scaled 2D quad (cz_torchN_size)
// flame particles: sprites are spawned all the time at the nozzle, fly forward, shrink and fade; a pressed trigger makes them faster and longer
struct czPart_t { vec3_t org, vel; float born, life; bool on; };
static void CZ_TorchParticles( HSPRITE spr, float baseSize, float al, const char *colStr, const vec3_t tip, const vec3_t fw )
{
	static cvar_t *cvSpd = nullptr, *cvLife, *cvRate, *cvSz;
	static czPart_t parts[48];
	static float lastT = 0, acc = 0;
	if( !cvSpd )
	{
		cvSpd  = gEngfuncs.pfnRegisterVariable( "cz_torch_pspeed", "40", 0 );
		cvLife = gEngfuncs.pfnRegisterVariable( "cz_torch_plife",  "0.12", 0 );
		cvRate = gEngfuncs.pfnRegisterVariable( "cz_torch_prate",  "120", 0 );
		cvSz   = gEngfuncs.pfnRegisterVariable( "cz_torch_psize",  "1.2", 0 );
	}
	extern bool g_czTorchAtk;
	const model_s *m = spr ? gEngfuncs.GetSpritePointer( spr ) : nullptr;
	if( !m )
		return;
	float t = gEngfuncs.GetClientTime();
	float dt = t - lastT;
	if( dt < 0.0f || dt > 0.2f ) dt = 0.0f;
	lastT = t;
	float spd  = cvSpd->value * ( g_czTorchAtk ? 2.5f : 1.0f );
	float life = cvLife->value * ( g_czTorchAtk ? 1.8f : 1.0f );
	acc += dt * cvRate->value;
	while( acc >= 1.0f )
	{
		acc -= 1.0f;
		for( int i = 0; i < 48; i++ )
		{
			if( parts[i].on ) continue;
			czPart_t &p = parts[i];
			p.on = true;
			p.born = t - gEngfuncs.pfnRandomFloat( 0.0f, dt );
			p.life = life * gEngfuncs.pfnRandomFloat( 0.7f, 1.0f );
			for( int j = 0; j < 3; j++ )
			{
				p.org[j] = tip[j];
				p.vel[j] = fw[j] * spd * gEngfuncs.pfnRandomFloat( 0.8f, 1.2f ) + gEngfuncs.pfnRandomFloat( -4.0f, 4.0f );
			}
			break;
		}
	}
	int cr = 255, cg = 217, cb = 235;
	sscanf( colStr, "%d %d %d", &cr, &cg, &cb );
	int w = SPR_Width( spr, 0 );
	static cl_entity_t ents[48];
	for( int i = 0; i < 48; i++ )
	{
		czPart_t &p = parts[i];
		if( !p.on ) continue;
		float age = t - p.born;
		if( age >= p.life ) { p.on = false; continue; }
		float f = age / p.life;
		vec3_t pos;
		for( int j = 0; j < 3; j++ )
			pos[j] = p.org[j] + p.vel[j] * age;
		cl_entity_t *e = &ents[i];
		memset( e, 0, sizeof( *e ));
		e->model = (model_s *)m;
		VectorCopy( pos, e->origin );
		VectorCopy( pos, e->curstate.origin );
		e->curstate.rendermode = kRenderTransAdd;
		e->curstate.renderamt = (int)( 255 * al * ( 1.0f - f ));
		e->curstate.rendercolor.r = (byte)bound( 0, cr, 255 );
		e->curstate.rendercolor.g = (byte)bound( 0, cg, 255 );
		e->curstate.rendercolor.b = (byte)bound( 0, cb, 255 );
		float half = baseSize * 0.1f * cvSz->value * ( 1.0f - f );
		e->curstate.scale = ( w > 0 ) ? ( half * 2.0f / (float)w ) : 1.0f;
		e->curstate.effects |= EF_NOINTERP;
		VectorCopy( pos, e->prevstate.origin );
		VectorCopy( pos, e->latched.prevorigin );
		e->curstate.msg_time = e->prevstate.msg_time = t;
		gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, e );
	}
}

static void CZ_TorchDraw( int pass )
{
	static cvar_t *cv[2][6];
	static cvar_t *cvCol[2] = { nullptr, nullptr };
	static cvar_t *cvMode = nullptr;
	static HSPRITE hspr[2] = { 0, 0 };
	static model_t *mspr[2] = { nullptr, nullptr };
	static char sprName[2][64] = { "", "" };
	if( !cv[0][0] )
	{
		static const char *defName[2] = { "sprites/flare3.spr", "sprites/flare3.spr" };
		static const char *defSize[2] = { "5.5", "35" };
		static const char *defAlpha[2] = { "1.0", "0.2" };
		static const char *defFwd[2] = { "-1", "-2" };
		static const char *defCol[2] = { "205 220 255", "255 217 235" };
		cvMode = gEngfuncs.pfnRegisterVariable( "cz_torch_mode", "4", 0 );
		for( int k = 0; k < 2; k++ )
		{
			char n[64];
			snprintf( n, sizeof n, "cz_torch%d_spr", k + 1 );   cv[k][0] = gEngfuncs.pfnRegisterVariable( n, defName[k], 0 );
			snprintf( n, sizeof n, "cz_torch%d_fwd", k + 1 );   cv[k][1] = gEngfuncs.pfnRegisterVariable( n, defFwd[k], 0 );
			snprintf( n, sizeof n, "cz_torch%d_right", k + 1 ); cv[k][2] = gEngfuncs.pfnRegisterVariable( n, "0", 0 );
			snprintf( n, sizeof n, "cz_torch%d_up", k + 1 );    cv[k][3] = gEngfuncs.pfnRegisterVariable( n, k ? "0" : "-0.16", 0 );
			snprintf( n, sizeof n, "cz_torch%d_color", k + 1 ); cvCol[k] = gEngfuncs.pfnRegisterVariable( n, defCol[k], 0 );
			snprintf( n, sizeof n, "cz_torch%d_size", k + 1 );  cv[k][4] = gEngfuncs.pfnRegisterVariable( n, defSize[k], 0 );
			snprintf( n, sizeof n, "cz_torch%d_alpha", k + 1 ); cv[k][5] = gEngfuncs.pfnRegisterVariable( n, defAlpha[k], 0 );
		}
	}

	extern vec3_t g_czTorchPos;
	extern bool g_czTorchOn, g_czTorchAtk;
	extern float g_czTorchSize;
	if( !g_czTorchOn || g_czTool != 3 || g_czFiber )
		return;

	// mode 0: 2D pass over everything (native size); other modes: camera facing quad drawn BEFORE the view model,
	// so the hands and the torch cover it
	// pass 0 = 2D (mode 0), 1 = TriAPI quad (modes 1, 2), 2 = real sprite entities (mode 3: engine draws them, scalable, behind the hands)
	{
		int m = (int)( cvMode->value + 0.5f );
		int want = ( m <= 0 ) ? 0 : ( m >= 3 ? 2 : 1 );
		if( pass != want )
			return;
	}
	bool worldPass = ( pass == 1 );

	vec3_t fw, rt, up;
	gEngfuncs.pfnAngleVectors( v_angles, fw, rt, up );

	for( int k = 0; k < 2; k++ )
	{
		const char *nm = cv[k][0]->string;
		if( strcmp( nm, sprName[k] ) )
		{
			snprintf( sprName[k], sizeof sprName[k], "%s", nm );
			hspr[k] = SPR_Load( sprName[k] );
			mspr[k] = gEngfuncs.LoadMapSprite( sprName[k] );
		}

		vec3_t c, scr;
		for( int i = 0; i < 3; i++ )
			c[i] = g_czTorchPos[i] + fw[i] * cv[k][1]->value + rt[i] * cv[k][2]->value + up[i] * cv[k][3]->value;
		float al = cv[k][5]->value * ( g_czTorchAtk ? 1.3f : 1.0f );
		if( al > 1.0f ) al = 1.0f;
		if( al < 0.0f ) al = 0.0f;

		if( pass == 2 && (int)( cvMode->value + 0.5f ) >= 4 )
		{
			// mode 4: the two flame sprites are drawn while the view model is drawn (CZ_TorchViewModelDraw); only the particles here
			extern vec3_t g_czTorchAtt;
			extern bool g_czTorchAttOk;
			if( k == 0 && hspr[0] )
			{
				vec3_t tip;
				const float *src = g_czTorchAttOk ? (const float *)g_czTorchAtt : (const float *)c;
				tip[0] = src[0]; tip[1] = src[1]; tip[2] = src[2];
				CZ_TorchParticles( hspr[0], cv[0][4]->value, al, cvCol[0]->string, tip, fw );
			}
			continue;
		}
		if( pass == 2 )
		{
			static cl_entity_t ents[2];
			const model_s *m = hspr[k] ? gEngfuncs.GetSpritePointer( hspr[k] ) : nullptr;
			if( !m )
				continue;
			cl_entity_t *e = &ents[k];
			memset( e, 0, sizeof( *e ));
			e->model = (model_s *)m;
			VectorCopy( c, e->origin );
			VectorCopy( c, e->curstate.origin );
			e->curstate.rendermode = kRenderTransAdd;
			e->curstate.renderamt = (int)( 255 * al );
			{
				int cr = 255, cg = 217, cb = 235;
				sscanf( cvCol[k]->string, "%d %d %d", &cr, &cg, &cb );
				e->curstate.rendercolor.r = (byte)bound( 0, cr, 255 );
				e->curstate.rendercolor.g = (byte)bound( 0, cg, 255 );
				e->curstate.rendercolor.b = (byte)bound( 0, cb, 255 );
			}
			e->curstate.renderfx = 0;
			e->curstate.frame = 0;
			int w = SPR_Width( hspr[k], 0 );
			float half = cv[k][4]->value * 0.1f * g_czTorchSize; // units
			// keep the sprite in front of world geometry (doors, tables): pull it towards the eye, keep apparent size
			{
				extern vec3_t v_origin;
				vec3_t d; VectorSubtract( c, v_origin, d );
				float dist = VectorLength( d );
				if( dist > 1.0f )
				{
					pmtrace_t tr;
					gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
					gEngfuncs.pEventAPI->EV_PushPMStates();
					gEngfuncs.pEventAPI->EV_SetSolidPlayers( gEngfuncs.GetLocalPlayer()->index - 1 );
					gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
					gEngfuncs.pEventAPI->EV_PlayerTrace( v_origin, c, PM_NORMAL, -1, &tr );
					gEngfuncs.pEventAPI->EV_PopPMStates();
					if( tr.fraction < 1.0f )
					{
						float nf = tr.fraction * dist - 2.0f;
						if( nf < 4.0f ) nf = 4.0f;
						float ratio = nf / dist;
						for( int i = 0; i < 3; i++ )
							e->origin[i] = e->curstate.origin[i] = v_origin[i] + d[i] * ratio;
						half *= ratio;
					}
				}
			}
			e->curstate.scale = ( w > 0 ) ? ( half * 2.0f / (float)w ) : 1.0f;
			// rigid to the view: no interpolation
			e->curstate.effects |= EF_NOINTERP;
			VectorCopy( e->origin, e->prevstate.origin );
			VectorCopy( e->origin, e->latched.prevorigin );
			e->curstate.msg_time = e->prevstate.msg_time = gEngfuncs.GetClientTime();
			e->curstate.animtime = 0;
			gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, e );
			if( k == 0 )
				CZ_TorchParticles( hspr[0], cv[0][4]->value, al, cvCol[0]->string, c, fw );
		}
		else if( !worldPass )
		{
			if( gEngfuncs.pTriAPI->WorldToScreen( c, scr ))
				continue; // behind the camera
			float cx = ( 1.0f + scr[0] ) * 0.5f * ScreenWidth;
			float cy = ( 1.0f - scr[1] ) * 0.5f * ScreenHeight;
			if( !hspr[k] )
				continue;
			int w = SPR_Width( hspr[k], 0 ), h = SPR_Height( hspr[k], 0 );
			SPR_Set( hspr[k], (int)( 255 * al ), (int)( 217 * al ), (int)( 235 * al ));
			SPR_DrawAdditive( 0, (int)cx - w / 2, (int)cy - h / 2, nullptr );
		}
		else
		{
			if( !mspr[k] )
				continue;
			// half size in world units: cz_torchN_size is in 0.1 unit steps
			float half = cv[k][4]->value * 0.1f * g_czTorchSize;
			vec3_t a, b, p0, p1, p2, p3;
			for( int i = 0; i < 3; i++ )
			{
				a[i] = rt[i] * half;
				b[i] = up[i] * half;
				p0[i] = c[i] - a[i] + b[i];
				p1[i] = c[i] + a[i] + b[i];
				p2[i] = c[i] + a[i] - b[i];
				p3[i] = c[i] - a[i] - b[i];
			}
			gEngfuncs.pTriAPI->SpriteTexture( mspr[k], 0 );
			gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
			gEngfuncs.pTriAPI->CullFace( TRI_NONE );
			gEngfuncs.pTriAPI->Begin( TRI_QUADS );
			gEngfuncs.pTriAPI->Color4f( 1.0f, 0.85f, 0.92f, al );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 0 ); gEngfuncs.pTriAPI->Vertex3fv( p0 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 0 ); gEngfuncs.pTriAPI->Vertex3fv( p1 );
			gEngfuncs.pTriAPI->TexCoord2f( 1, 1 ); gEngfuncs.pTriAPI->Vertex3fv( p2 );
			gEngfuncs.pTriAPI->TexCoord2f( 0, 1 ); gEngfuncs.pTriAPI->Vertex3fv( p3 );
			gEngfuncs.pTriAPI->End();
			gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
		}
	}
}

void CZ_DrawTorchSprites( void )
{
	CZ_TorchDraw( 0 );
}

void CZ_TorchEntities( void )
{
	CZ_TorchDraw( 2 );
}

void CZ_DrawTorchWorld( void )
{
	CZ_TorchDraw( 1 );
}

// Mode 4: called from the studio renderer right after the view model is drawn. att = view model attachment 0 of THIS frame,
// so the flame is rigid to the nozzle (no lag), and it is drawn with the view model's depth range (hands cover it).
vec3_t g_czTorchAtt;
bool g_czTorchAttOk = false;
void CZ_TorchViewModelDraw( const float *att )
{
	extern bool g_czTorchOn, g_czTorchAtk;
	extern float g_czTorchSize;
	g_czTorchAttOk = false;
	cvar_t *cm = gEngfuncs.pfnGetCvarPointer( "cz_torch_mode" );
	if( !cm || (int)( cm->value + 0.5f ) < 4 || !g_czTorchOn || g_czTool != 3 || g_czFiber )
		return;
	VectorCopy( att, g_czTorchAtt );
	g_czTorchAttOk = true;
	static HSPRITE hspr[2] = { 0, 0 };
	static char nm[2][64] = { "", "" };
	vec3_t fw, rt, up;
	gEngfuncs.pfnAngleVectors( v_angles, fw, rt, up );
	for( int k = 0; k < 2; k++ )
	{
		char n[64];
		snprintf( n, sizeof n, "cz_torch%d_spr", k + 1 );
		cvar_t *cs = gEngfuncs.pfnGetCvarPointer( n );
		if( !cs ) return;
		if( strcmp( cs->string, nm[k] ))
		{
			snprintf( nm[k], sizeof nm[k], "%s", cs->string );
			hspr[k] = SPR_Load( nm[k] );
		}
		const model_s *m = hspr[k] ? gEngfuncs.GetSpritePointer( hspr[k] ) : nullptr;
		if( !m ) continue;
		auto cvf = [&]( const char *suffix, float def ) {
			snprintf( n, sizeof n, "cz_torch%d_%s", k + 1, suffix );
			cvar_t *c = gEngfuncs.pfnGetCvarPointer( n );
			return c ? c->value : def;
		};
		float fo = cvf( "fwd", 0 ), ro = cvf( "right", 0 ), uo = cvf( "up", 0 );
		float half = cvf( "size", 5 ) * 0.1f * g_czTorchSize;
		float al = cvf( "alpha", 1 ) * ( g_czTorchAtk ? 1.3f : 1.0f );
		if( al > 1.0f ) al = 1.0f;
		int cr = 255, cg = 217, cb = 235;
		snprintf( n, sizeof n, "cz_torch%d_color", k + 1 );
		cvar_t *cc = gEngfuncs.pfnGetCvarPointer( n );
		if( cc ) sscanf( cc->string, "%d %d %d", &cr, &cg, &cb );
		vec3_t c, p0, p1, p2, p3;
		for( int i = 0; i < 3; i++ )
		{
			c[i] = att[i] + fw[i] * fo + rt[i] * ro + up[i] * uo;
			p0[i] = c[i] - rt[i] * half + up[i] * half;
			p1[i] = c[i] + rt[i] * half + up[i] * half;
			p2[i] = c[i] + rt[i] * half - up[i] * half;
			p3[i] = c[i] - rt[i] * half - up[i] * half;
		}
		gEngfuncs.pTriAPI->SpriteTexture( (model_s *)m, 0 );
		gEngfuncs.pTriAPI->RenderMode( kRenderTransAdd );
		gEngfuncs.pTriAPI->CullFace( TRI_NONE );
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );
		gEngfuncs.pTriAPI->Color4f( cr / 255.0f, cg / 255.0f, cb / 255.0f, al );
		gEngfuncs.pTriAPI->TexCoord2f( 0, 0 ); gEngfuncs.pTriAPI->Vertex3fv( p0 );
		gEngfuncs.pTriAPI->TexCoord2f( 1, 0 ); gEngfuncs.pTriAPI->Vertex3fv( p1 );
		gEngfuncs.pTriAPI->TexCoord2f( 1, 1 ); gEngfuncs.pTriAPI->Vertex3fv( p2 );
		gEngfuncs.pTriAPI->TexCoord2f( 0, 1 ); gEngfuncs.pTriAPI->Vertex3fv( p3 );
		gEngfuncs.pTriAPI->End();
		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}
}
