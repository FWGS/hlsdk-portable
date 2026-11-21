/*
WorldMap.cpp - Condition Zero world map and mission select (CZ:DS)
Own drawing in the menu library: the original window is VGUI2 and its controls are not available.
Layout numbers are from the original resource/UI/WorldMap.res and WorldMapMissionSelect.res.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
*/

#include "BaseMenu.h"
#include "BaseWindow.h"
#include "Image.h"
#include "keydefs.h"
#include "Utils.h"
#include "FontManager.h"
#include <math.h>

#define WORLDMAP_FONT   "Trebuchet MS"
#define WORLDMAP_WEIGHT 500

enum
{
	WORLDMAP_WIDTH = 640,
	WORLDMAP_HEIGHT = 480,
	WORLDMAP_STRING_SIZE = 128,
	WORLDMAP_FIRST_BONUS_BIT = 16384, // the bits from this one are the bonus missions
	WORLDMAP_GAME_FINISHED_BIT = 1048576 // the mod's server sets it when the ending is reached
};

enum
{
	MARKER_CURRENT,
	MARKER_UNLOCKED,
	MARKER_LOCKED,
	MARKER_COUNT
};

enum
{
	WORLDMAP_FONT_SMALL = 14,
	WORLDMAP_FONT_MEDIUM = 16,
	WORLDMAP_FONT_LARGE = 20
};

struct WorldMapRect_t
{
	int x, y, w, h;
};

static const WorldMapRect_t g_CancelButton = { 384, 410, 110, 22 };
static const WorldMapRect_t g_OkButton = { 510, 410, 100, 22 };
static const WorldMapRect_t g_ResumeButton = { 504, 437, 80, 26 };
static const int g_iSkillY[] = { 314, 341, 370 };
static const char *g_SkillKeys[] = { "CZERO_EasySkill", "CZERO_MediumSkill", "CZERO_HardSkill" };
static const int g_iFontHeights[] = { WORLDMAP_FONT_SMALL, WORLDMAP_FONT_MEDIUM, WORLDMAP_FONT_LARGE };

// the world map is made of tiles
static const int g_iTileX[] = { 0, 256, 512 };
static const int g_iTileW[] = { 256, 256, 128 };
static const int g_iTileY[] = { 0, 256 };
static const int g_iTileH[] = { 256, 224 };

// bit of the mission in cl_levellocks (LEVEL_* of the mod)
static const int g_iMissionBits[] =
{
	2, 8, 4, 16, 32, 64, 128, 256, 512, 2048, 1024, 4096, 8192,   // the order of g_Missions
	16384, 32768, 65536, 131072, 262144, 524288                    // the bonus missions
};

struct WorldMapMission_t
{
	const char *prefix;  // #<prefix>_TITLE
	const char *info;    // #<info>_BRIEF, _LOCATION, _TIME, ... (differs from prefix only for Silo)
	const char *image;   // gfx/vgui/<image>_large.tga
	const char *map;
	int x, y, w, h;
};

static const WorldMapMission_t g_Missions[] =
{
	{ "TRAINING", "TRAINING", "cz_train", "cz_train01", 90, 115, 180, 15 },
	{ "LOSTCAUSE", "LOSTCAUSE", "cz_lostcause", "cz_lostcause", 479, 197, 155, 15 },
	{ "RECOIL", "RECOIL", "cz_recoil", "cz_recoil-intro", 332, 207, 150, 15 },
	{ "SILO", "SECRETWAR", "cz_silo", "cz_silo01", 453, 74, 180, 15 },
	{ "BRECON", "BRECON", "cz_brecon", "cz_brecon01", 330, 90, 200, 15 },
	{ "DRUGLAB", "DRUGLAB", "cz_druglab", "cz_druglab1", 139, 211, 170, 15 },
	{ "MOTORCADE", "MOTORCADE", "cz_motor", "cz_motor1", 287, 129, 200, 15 },
	{ "THINICE", "THINICE", "cz_thinice", "cz_thinice01", 238, 89, 90, 15 },
	{ "DOWNED", "DOWNED", "cz_downed", "cz_downed1", 149, 257, 200, 15 },
	{ "TURNCRANK", "TURNCRANK", "cz_turncrank", "cz_turncrank", 74, 132, 190, 15 },
	{ "HANKAGAI", "HANKAGAI", "cz_hankagai", "cz_hankagai1", 540, 115, 95, 15 },
	{ "ALAMO", "ALAMO", "cz_alamo", "cz_alamo", 317, 144, 55, 15 },
	{ "HIGHRISE", "HIGHRISE", "cz_highrise", "cz_hr01", 274, 111, 200, 15 },
	{ "TRUTH", "TRUTH", "cz_truth", "cz_truth1", 535, 130, 100, 15 },
	{ "RUN", "RUN", "cz_run", "cz_run1", 535, 99, 90, 15 },
	{ "MIAMI", "MIAMI", "cz_miami", "cz_miami1", 139, 154, 170, 15 },
	{ "FASTLINE", "FASTLINE", "cz_fastline", "cz_fastline1", 537, 148, 95, 15 },
	{ "PIPEDREAM", "PIPEDREAM", "cz_pipedream", "cz_pipedream1", 372, 141, 90, 15 },
	{ "SANDSTORM", "SANDSTORM", "cz_sandstorm", "cz_sandstorm", 370, 158, 160, 15 },
};
static const int g_iMissionCount = V_ARRAYSIZE( g_Missions );

class CMenuWorldMap : public CMenuBaseWindow
{
public:
	CMenuWorldMap() : CMenuBaseWindow( "CMenuWorldMap" ), m_bSelect( false ), m_iSelected( -1 ), m_iSkill( 1 ),
		m_bLoaded( false ), m_flFontScale( 0 ) { for( size_t i = 0; i < V_ARRAYSIZE( m_hFonts ); i++ ) m_hFonts[i] = -1; }

	void Show() override;
	void Draw() override;
	bool KeyDown( int key ) override;
	bool IsRoot() const override { return false; }

private:
	void LoadImages();
	HFont GetFont( int heightAt480 );
	void Layout();
	void Image( CImage &img, int x, int y, int w, int h, bool alpha = false );
	void Text( const char *str, int x, int y, int w, int h, unsigned int col, int charH, uint justify );
	bool In( int mx, int my, int x, int y, int w, int h ) const;
	int  CurrentMission() const;
	bool IsOpen( int idx ) const;
	int  LatestOpen() const;
	int  MissionAt( int mx, int my ) const;
	void DrawMissionInfo( int idx );
	void StartMission();

	bool m_bSelect;
	int  m_iSelected;
	int  m_iSkill;
	bool m_bLoaded;
	float m_flScale;
	int  m_iOffX, m_iOffY;

	HFont m_hFonts[V_ARRAYSIZE( g_iFontHeights )];
	float m_flFontScale;
	CImage m_Tiles1[V_ARRAYSIZE( g_iTileX )][V_ARRAYSIZE( g_iTileY )];
	CImage m_Tiles2[V_ARRAYSIZE( g_iTileX )][V_ARRAYSIZE( g_iTileY )];
	CImage m_Marker[MARKER_COUNT];
	CImage m_MissionImg[g_iMissionCount];
	bool   m_MissionImgLoaded[g_iMissionCount];
};

static CMenuWorldMap *g_pWorldMap = NULL;

static const char *MissionKey( char *key, const char *prefix, const char *suffix )
{
	V_snprintf( key, WORLDMAP_STRING_SIZE, "%s%s", prefix, suffix );
	return key;
}

HFont CMenuWorldMap::GetFont( int heightAt480 )
{
	const float fs = uiStatic.scaleY;
	if( fabs( m_flFontScale - fs ) > 0.01f )
	{
		for( size_t i = 0; i < V_ARRAYSIZE( m_hFonts ); i++ )
			if( m_hFonts[i] != -1 )
				g_FontMgr->DeleteFont( m_hFonts[i] );
		for( size_t i = 0; i < V_ARRAYSIZE( m_hFonts ); i++ )
			m_hFonts[i] = CFontBuilder( WORLDMAP_FONT, g_iFontHeights[i] * fs, WORLDMAP_WEIGHT ).Create();
		m_flFontScale = fs;
	}
	return heightAt480 <= WORLDMAP_FONT_SMALL ? m_hFonts[0] : ( heightAt480 <= WORLDMAP_FONT_MEDIUM ? m_hFonts[1] : m_hFonts[2] );
}

void CMenuWorldMap::LoadImages()
{
	if( m_bLoaded )
		return;
	m_bLoaded = true;

	for( size_t i = 0; i < V_ARRAYSIZE( g_iTileX ); i++ )
		for( size_t j = 0; j < V_ARRAYSIZE( g_iTileY ); j++ )
		{
			char path[WORLDMAP_STRING_SIZE];
			V_snprintf( path, sizeof( path ), "gfx/vgui/worldmap_%d_%d.tga", (int)i, (int)j );
			m_Tiles1[i][j].Load( path );
			V_snprintf( path, sizeof( path ), "gfx/vgui/worldmap2_%d_%d.tga", (int)i, (int)j );
			m_Tiles2[i][j].Load( path );
		}
	m_Marker[MARKER_CURRENT].Load( "gfx/vgui/missioncurrent.tga" );
	m_Marker[MARKER_UNLOCKED].Load( "gfx/vgui/missionunlocked.tga" );
	m_Marker[MARKER_LOCKED].Load( "gfx/vgui/missionlocked.tga" );
	for( int i = 0; i < g_iMissionCount; i++ )
		m_MissionImgLoaded[i] = false;
}

void CMenuWorldMap::Layout()
{
	float sx = ScreenWidth / (float)WORLDMAP_WIDTH, sy = ScreenHeight / (float)WORLDMAP_HEIGHT;
	m_flScale = sx < sy ? sx : sy;
	m_iOffX = (int)( ( ScreenWidth - WORLDMAP_WIDTH * m_flScale ) * 0.5f );
	m_iOffY = (int)( ( ScreenHeight - WORLDMAP_HEIGHT * m_flScale ) * 0.5f );
}

void CMenuWorldMap::Image( CImage &img, int x, int y, int w, int h, bool alpha )
{
	UI_DrawPic( m_iOffX + (int)( x * m_flScale ), m_iOffY + (int)( y * m_flScale ),
		(int)( w * m_flScale + 0.5f ), (int)( h * m_flScale + 0.5f ), 0xFFFFFFFF, img,
		alpha ? QM_DRAWTRANS : QM_DRAWNORMAL );
}

void CMenuWorldMap::Text( const char *str, int x, int y, int w, int h, unsigned int col, int charH, uint justify )
{
	int fontH = WORLDMAP_FONT_MEDIUM;
	if( charH >= 18 ) fontH = WORLDMAP_FONT_LARGE;
	else if( charH == 15 ) fontH = WORLDMAP_FONT_SMALL;
	HFont font = GetFont( fontH );
	UI_DrawString( font, m_iOffX + (int)( x * m_flScale ), m_iOffY + (int)( y * m_flScale ),
		(int)( w * m_flScale ), (int)( h * m_flScale ), str, col, (int)( fontH * uiStatic.scaleY ), justify, ETF_FORCECOL );
}

bool CMenuWorldMap::In( int mx, int my, int x, int y, int w, int h ) const
{
	int px = m_iOffX + (int)( x * m_flScale ), py = m_iOffY + (int)( y * m_flScale );
	return mx >= px && my >= py && mx < px + (int)( w * m_flScale ) && my < py + (int)( h * m_flScale );
}

int CMenuWorldMap::MissionAt( int mx, int my ) const
{
	for( int i = 0; i < g_iMissionCount; i++ )
		if( In( mx, my, g_Missions[i].x, g_Missions[i].y, g_Missions[i].w, g_Missions[i].h ))
			return i;
	return -1;
}

bool CMenuWorldMap::IsOpen( int idx ) const
{
	return ( (int)EngFuncs::GetCvarFloat( "cl_levellocks" ) & g_iMissionBits[idx] ) != 0;
}

// the highest bit among the open campaign missions is the next one to play
int CMenuWorldMap::LatestOpen() const
{
	int best = -1;

	// the game is finished: all the open missions are done, none of them is the current one
	if( (int)EngFuncs::GetCvarFloat( "cl_levellocks" ) & WORLDMAP_GAME_FINISHED_BIT )
		return best;

	for( int i = 0; i < g_iMissionCount; i++ )
	{
		if( g_iMissionBits[i] < WORLDMAP_FIRST_BONUS_BIT && IsOpen( i ) && ( best < 0 || g_iMissionBits[i] > g_iMissionBits[best] ))
			best = i;
	}
	return best;
}

int CMenuWorldMap::CurrentMission() const
{
	const char *map = EngFuncs::GetCvarString( "hostmap" );
	if( map && map[0] )
	{
		for( int i = 0; i < g_iMissionCount; i++ )
		{
			size_t l = strlen( g_Missions[i].image );
			if( !strncmp( map, g_Missions[i].image, l ))
				return i;
		}
	}
	return 0;
}

void CMenuWorldMap::Show()
{
	Layout();
	m_bSelect = false;
	m_iSelected = -1;
	m_iSkill = (int)EngFuncs::GetCvarFloat( "skill" );
	if( m_iSkill < 1 || m_iSkill > 3 ) m_iSkill = 1;
	if( !uiStatic.menu.IsActive() )
		UI_SetActiveMenu( true );
	CMenuBaseWindow::Show();
}

void CMenuWorldMap::StartMission()
{
	if( m_iSelected < 0 )
		return;

	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", (float)m_iSkill );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f );
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::PlayBackgroundTrack( NULL, NULL );
	EngFuncs::ClientCmdF( false, "map %s\n", g_Missions[m_iSelected].map );
}

void CMenuWorldMap::DrawMissionInfo( int idx )
{
	const unsigned int green = PackRGBA( 40, 210, 60, 255 );
	const unsigned int gray = PackRGBA( 190, 190, 190, 255 );
	const char *p = g_Missions[idx].prefix;
	const char *pi = g_Missions[idx].info;
	char key[WORLDMAP_STRING_SIZE];

	if( !m_MissionImgLoaded[idx] )
	{
		char path[WORLDMAP_STRING_SIZE];
		V_snprintf( path, sizeof( path ), "gfx/vgui/%s_large.tga", g_Missions[idx].image );
		m_MissionImg[idx].Load( path );
		m_MissionImgLoaded[idx] = true;
	}
	Image( m_MissionImg[idx], 20, 374, 87, 87, true );
	Text( L( "CZERO_Mission" ), 139, 361, 90, 39, PackRGBA( 215, 215, 215, 255 ), 22, QM_LEFT );
	Text( L( MissionKey( key, p, "_TITLE" )), 225, 361, 400, 39, green, 22, QM_LEFT );
	Text( L( MissionKey( key, pi, "_BRIEF" )), 140, 392, 300, 70, gray, 14, QM_TOPLEFT );
}

void CMenuWorldMap::Draw()
{
	LoadImages();
	Layout();

	UI_FillRect( 0, 0, ScreenWidth, ScreenHeight, PackRGBA( 0, 0, 0, 255 ));

	const int mx = uiStatic.cursorX, my = uiStatic.cursorY;
	const unsigned int white = PackRGBA( 255, 255, 255, 255 );

	if( !m_bSelect )
	{
		for( size_t i = 0; i < V_ARRAYSIZE( g_iTileX ); i++ )
			for( size_t j = 0; j < V_ARRAYSIZE( g_iTileY ); j++ )
				Image( m_Tiles1[i][j], g_iTileX[i], g_iTileY[j], g_iTileW[i], g_iTileH[j] );

		int cur = CurrentMission();
		const int latest = LatestOpen();
		int hover = MissionAt( mx, my );

		for( int i = 0; i < g_iMissionCount; i++ )
		{
			const WorldMapMission_t &m = g_Missions[i];
			const bool open = IsOpen( i );
			CImage &mark = m_Marker[ !open ? MARKER_LOCKED : ( i == latest ? MARKER_CURRENT : MARKER_UNLOCKED ) ];
			Image( mark, m.x, m.y + 5, 6, 6 );
			char key[WORLDMAP_STRING_SIZE];
			Text( L( MissionKey( key, m.prefix, "_TITLE" )), m.x + 10, m.y, m.w, m.h,
				i == hover ? PackRGBA( 255, 255, 0, 255 ) : white, 15, QM_LEFT );
		}

		DrawMissionInfo( hover >= 0 ? hover : cur );

		// Resume: only when a game is running
		const bool inGame = EngFuncs::ClientInGame() != 0;
		const bool onResume = inGame && In( mx, my, g_ResumeButton.x, g_ResumeButton.y, g_ResumeButton.w, g_ResumeButton.h );
		UI_DrawRectangle( m_iOffX + (int)( 504 * m_flScale ), m_iOffY + (int)( 437 * m_flScale ),
			(int)( 80 * m_flScale ), (int)( 26 * m_flScale ), inGame ? white : PackRGBA( 110, 110, 110, 255 ));
		Text( L( "CZERO_Resume" ), 504, 437, 80, 26, onResume ? PackRGBA( 255, 255, 0, 255 ) : ( inGame ? white : PackRGBA( 110, 110, 110, 255 )), 14, QM_CENTER );
	}
	else
	{
		for( size_t i = 0; i < V_ARRAYSIZE( g_iTileX ); i++ )
			for( size_t j = 0; j < V_ARRAYSIZE( g_iTileY ); j++ )
				Image( m_Tiles2[i][j], g_iTileX[i], g_iTileY[j], g_iTileW[i], g_iTileH[j] );

		const WorldMapMission_t &m = g_Missions[m_iSelected];
		const char *p = m.prefix;
		const char *pi = m.info;
		char key[WORLDMAP_STRING_SIZE];
		const unsigned int label = PackRGBA( 215, 215, 215, 255 );
		const unsigned int value = PackRGBA( 190, 190, 190, 255 );

		Text( L( "CZERO_Mission" ), 20, 60, 90, 20, label, 20, QM_LEFT );
		Text( L( MissionKey( key, p, "_TITLE" )), 110, 60, 250, 20, PackRGBA( 40, 210, 60, 255 ), 20, QM_LEFT );

		Text( L( "CZERO_Time" ), 10, 90, 110, 20, label, 14, QM_TOPRIGHT );
		Text( L( MissionKey( key, pi, "_TIME" )), 130, 90, 230, 20, value, 14, QM_TOPLEFT );
		Text( L( "CZERO_Locale" ), 10, 110, 110, 40, label, 14, QM_TOPRIGHT );
		Text( L( MissionKey( key, pi, "_LOCATION" )), 130, 110, 230, 40, value, 14, QM_TOPLEFT );
		Text( L( "CZERO_Organization" ), 10, 150, 110, 20, label, 14, QM_TOPRIGHT );
		Text( L( MissionKey( key, pi, "_ORGANIZATION" )), 130, 150, 230, 20, value, 14, QM_TOPLEFT );
		Text( L( "CZERO_Overview" ), 10, 170, 110, 20, label, 14, QM_TOPRIGHT );
		Text( L( MissionKey( key, pi, "_OVERVIEW" )), 130, 170, 230, 150, value, 14, QM_TOPLEFT );
		Text( L( "CZERO_Objective" ), 10, 320, 110, 20, label, 14, QM_TOPRIGHT );
		Text( L( MissionKey( key, pi, "_OBJECTIVE" )), 130, 320, 230, 100, value, 14, QM_TOPLEFT );

		if( !m_MissionImgLoaded[m_iSelected] )
		{
			char path[WORLDMAP_STRING_SIZE];
			V_snprintf( path, sizeof( path ), "gfx/vgui/%s_large.tga", m.image );
			m_MissionImg[m_iSelected].Load( path );
			m_MissionImgLoaded[m_iSelected] = true;
		}
		Image( m_MissionImg[m_iSelected], 381, 60, 243, 243, true );

		for( int i = 0; i < (int)V_ARRAYSIZE( g_iSkillY ); i++ )
		{
			const bool sel = m_iSkill == i + 1;
			const bool hov = In( mx, my, 430, g_iSkillY[i], 200, 30 );
			const int cx = 430 + 6, cy = g_iSkillY[i] + 6;
			UI_DrawRectangle( m_iOffX + (int)( cx * m_flScale ), m_iOffY + (int)( cy * m_flScale ),
				(int)( 10 * m_flScale ), (int)( 10 * m_flScale ), white );
			if( sel )
				UI_FillRect( m_iOffX + (int)( ( cx + 3 ) * m_flScale ), m_iOffY + (int)( ( cy + 3 ) * m_flScale ),
					(int)( 4 * m_flScale ), (int)( 4 * m_flScale ), white );
			Text( L( g_SkillKeys[i] ), 430 + 24, g_iSkillY[i], 176, 30,
				sel ? white : ( hov ? PackRGBA( 255, 255, 0, 255 ) : PackRGBA( 170, 170, 170, 255 )), 14, QM_LEFT );
		}

		const WorldMapRect_t *buttons[] = { &g_CancelButton, &g_OkButton };
		const char *buttonKeys[] = { "CZERO_Cancel", "CZERO_Okay" };
		for( size_t i = 0; i < V_ARRAYSIZE( buttons ); i++ )
		{
			const WorldMapRect_t &b = *buttons[i];
			const bool hov = In( mx, my, b.x, b.y, b.w, b.h );
			UI_DrawRectangle( m_iOffX + (int)( b.x * m_flScale ), m_iOffY + (int)( b.y * m_flScale ),
				(int)( b.w * m_flScale ), (int)( b.h * m_flScale ), white );
			Text( L( buttonKeys[i] ), b.x, b.y, b.w, b.h, hov ? PackRGBA( 255, 255, 0, 255 ) : white, 13, QM_CENTER );
		}
	}
}

bool CMenuWorldMap::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ))
	{
		if( m_bSelect )
			m_bSelect = false;
		else
			Hide();
		return true;
	}

	if( UI::Key::IsConsole( key ))
	{
		EngFuncs::KEY_SetDest( KEY_CONSOLE );
		return true;
	}

	if( key != K_MOUSE1 )
		return true;

	const int mx = uiStatic.cursorX, my = uiStatic.cursorY;

	if( !m_bSelect )
	{
		int hit = MissionAt( mx, my );
		if( hit >= 0 && IsOpen( hit ))
		{
			m_iSelected = hit;
			m_bSelect = true;
		}
		else if( EngFuncs::ClientInGame() && In( mx, my, 504, 437, 80, 26 ))
			UI_CloseMenu();
	}
	else
	{
		for( int i = 0; i < (int)V_ARRAYSIZE( g_iSkillY ); i++ )
			if( In( mx, my, 430, g_iSkillY[i], 200, 30 ))
				m_iSkill = i + 1;

		if( In( mx, my, g_CancelButton.x, g_CancelButton.y, g_CancelButton.w, g_CancelButton.h ))
			m_bSelect = false;
		else if( In( mx, my, g_OkButton.x, g_OkButton.y, g_OkButton.w, g_OkButton.h ))
			StartMission();
	}
	return true;
}

// console: menu_worldmap
ADD_MENU( menu_worldmap, CMenuWorldMap, UI_WorldMap_Menu );
