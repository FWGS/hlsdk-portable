/*
Btns.h - WON menu buttons atlas manager
Copyright (C) 2011 CrazyRussian
Copyright (C) Xash3D FWGS contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef BTNS_H
#define BTNS_H

#include "enginecallback_menu.h"

enum EDefaultBtns
{
	PC_NEW_GAME = 0,
	PC_RESUME_GAME,
	PC_HAZARD_COURSE,
	PC_CONFIG,
	PC_LOAD_GAME,
	PC_SAVE_LOAD_GAME,
	PC_VIEW_README,
	PC_QUIT,
	PC_MULTIPLAYER,
	PC_EASY,
	PC_MEDIUM,
	PC_DIFFICULT,
	PC_SAVE_GAME,
	PC_LOAD_GAME2,
	PC_CANCEL,
	PC_GAME_OPTIONS,
	PC_VIDEO,
	PC_AUDIO,
	PC_CONTROLS,
	PC_DONE,
	PC_QUICKSTART,
	PC_USE_DEFAULTS,
	PC_OK,
	PC_VID_OPT,
	PC_VID_MODES,
	PC_ADV_CONTROLS,
	PC_ORDER_HL,
	PC_DELETE,
	PC_INET_GAME,
	PC_CHAT_ROOMS,
	PC_LAN_GAME,
	PC_CUSTOMIZE,
	PC_SKIP,
	PC_EXIT,
	PC_CONNECT,
	PC_REFRESH,
	PC_FILTER,
	PC_FILTER2,
	PC_CREATE,
	PC_CREATE_GAME,
	PC_CHAT_ROOMS2,
	PC_LIST_ROOMS,
	PC_SEARCH,
	PC_SERVERS,
	PC_JOIN,
	PC_FIND,
	PC_CREATE_ROOM,
	PC_JOIN_GAME,
	PC_SEARCH_GAMES,
	PC_FIND_GAME,
	PC_START_GAME,
	PC_VIEW_GAME_INFO,
	PC_UPDATE,
	PC_ADD_SERVER,
	PC_DISCONNECT,
	PC_CONSOLE,
	PC_CONTENT_CONTROL,
	PC_UPDATE2,
	PC_VISIT_WON,
	PC_PREVIEWS,
	PC_ADV_OPT,
	PC_3DINFO_SITE,
	PC_CUSTOM_GAME,
	PC_ACTIVATE,
	PC_INSTALL,
	PC_VISIT_WEB_SITE,
	PC_REFRESH_LIST,
	PC_DEACTIVATE,
	PC_ADV_OPT2,
	PC_SPECTATE_GAME,
	PC_SPECTATE_GAMES,
	PC_BUTTONCOUNT		// must be last
};

// Non-standard button images start here
#define PC_GAMEPAD       "gfx/shell/btn_gamepad"
#define PC_TOUCH         "gfx/shell/btn_touch"
#define PC_TOUCH_OPTIONS "gfx/shell/btn_touch_options"
#define PC_TOUCH_BUTTONS "gfx/shell/btn_touch_buttons"
#define PC_TOUCH_SAVE    "gfx/shell/btn_touch_save"
#define PC_TOUCH_EDITOR  "gfx/shell/btn_touch_editor"
#define PC_TOUCH_SELECT  "gfx/shell/btn_touch_select"
#define PC_TOUCH_RESET   "gfx/shell/btn_touch_reset"
#define PC_FAVORITE      "gfx/shell/btn_favorite"
#define PC_UNFAVORITE    "gfx/shell/btn_unfavorite"
#define PC_GYRO          "gfx/shell/btn_gyro"

#define BUTTON_NOFOCUS	0
#define BUTTON_FOCUS	1
#define BUTTON_PRESSED	2

class CBtnsManager
{
public:
	CBtnsManager() : pics{}, x{}, y{}, width( 0 ), height( 0 ), tex_h( 0 ), tex_stride( 0 ) {}

	void LoadBmpButtons();

	HIMAGE GetPic( int id ) const { return pics[id]; }
	int GetWidth() const { return width; }
	int GetHeight() const { return height; }
	int GetTexH() const { return tex_h; }
	int GetTexStride() const { return tex_stride; }
	int GetX( int id ) const { return x[id]; }
	int GetY( int id ) const { return y[id]; }

private:
	HIMAGE	pics[PC_BUTTONCOUNT];

	int		width;      // per-button width from BMP
	int		height;     // virtual button height for layout (26)
	int		tex_h;      // actual texture height per button state
	int		tex_stride; // step between states in atlas (tex_h + guard)
	int		x[PC_BUTTONCOUNT]; // x position in atlas
	int		y[PC_BUTTONCOUNT]; // y position in atlas
};

#endif // BTNS_H
