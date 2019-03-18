#ifndef GAME_MENU_INT_H
#define GAME_MENU_INT_H

#include "interface.h"
#include "../mainui/font/FontRenderer.h"

#ifdef _WIN32
#include "windows.h"
#define MAINUI_DLLNAME "cl_dlls/menu.dll"
#elif defined(OSX)
#define MAINUI_DLLNAME "cl_dlls/menu.dylib"
#elif defined(LINUX)
#define MAINUI_DLLNAME "cl_dlls/menu.so"
#endif

#define GAMEMENUEXPORTS_INTERFACE_VERSION "GameMenuExports"

class IGameMenuExports : public IBaseInterface
{
public:
	virtual bool Initialize( CreateInterfaceFn factory ) = 0;
	virtual HFont BuildFont( CFontBuilder &builder ) = 0;
	virtual const char *L( const char *szStr ) = 0;
	virtual bool IsActive() = 0;
	virtual bool IsMainMenuActive() = 0;
	virtual void Key( int key, int down ) = 0;
	virtual void MouseMove( int x, int y ) = 0;
	virtual void GetCharABCWide( HFont font, int ch, int &a, int &b, int &c ) = 0;
	virtual int GetFontTall( HFont font ) = 0;
	virtual int GetCharacterWidth(HFont font, int ch, int charH ) = 0;
	virtual void GetTextSize( HFont font, const char *text, int *wide, int *height = 0, int size = -1 ) = 0;
	virtual int	 GetTextHeight( HFont font, const char *text, int size = -1 ) = 0;
	virtual int DrawCharacter( HFont font, int ch, int x, int y, int charH, const unsigned int color, bool forceAdditive = false ) = 0;
	virtual void SetupScoreboard( int xstart, int xend, int ystart, int yend, unsigned int color, bool drawStroke ) = 0;
	virtual void DrawScoreboard( void ) = 0;
	virtual void DrawSpectatorMenu( void ) = 0;
	virtual void ShowVGUIMenu( int menuType, int param1, int param2 ) = 0;
};

#endif
