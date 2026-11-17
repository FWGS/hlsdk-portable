/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "BaseMenu.h" // ADD_MENU
#include "Framework.h"
#include "mobility_int.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Slider.h"
#include "Field.h"
#include "Action.h"
#include "Table.h"
#include "CheckBox.h"
#include "YesNoMessageBox.h"
#include "StringArrayModel.h"
#include "KbActListModel.h"
#define ART_BANNER	  	"gfx/shell/head_touch_buttons"

class CMenuTouchButtons;

class CMenuCommandPickerDialog : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;
	CMenuCommandPickerDialog() : BaseClass( "CMenuCommandPickerDialog" ), model( this )
	{
		iFlags |= QMF_DIALOG;
	}

	void _Init() override;
	void _VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;
	void Show() override;

	class CCommandListModel : public CMenuKbActListModel
	{
	public:
		CCommandListModel( CMenuCommandPickerDialog *parent )
			: CMenuKbActListModel( VIEW_PICKER ), parent( parent ) { }

		void OnActivateEntry( int line ) override
		{
			if( IsLineUsable( line ))
				parent->Confirm();
		}

	private:
		CMenuCommandPickerDialog *parent;
	} model;

	void Confirm();

	CMenuTouchButtons *pCaller;
	CMenuAction title;
	CMenuTable list;
	CMenuPicButton ok;
	CMenuPicButton cancel;
};

class CMenuTouchButtons : public CMenuFramework
{
public:
	CMenuTouchButtons() : CMenuFramework( "CMenuTouchButtons" ), model( this ) { }

private:
	void _Init() override;
	void _VidInit() override;
public:
	void DeleteButton();
	void ResetButtons();
	void OpenFileDialog();
	void UpdateTexture();
	void UpdateSP();
	void UpdateMP();
	void SaveButton();
	void RemoveMsgBox();
	void ResetMsgBox();
	void UpdateFields();
	void UpdateCommandFlags();

	static void ExitMenuCb(CMenuBaseItem *pSelf, void *pExtra);
	// Use an event system here!
	// Don't make this static method cry.
	static void FileDialogCallback( bool success );

	HIMAGE textureid;
	char selectedName[256];
	int curflags;

	
	CMenuPicButton	done;
	CMenuPicButton	cancel;

	CMenuSlider	red;
	CMenuSlider	green;
	CMenuSlider	blue;
	CMenuSlider	alpha;
	CMenuCheckBox	hide;
	CMenuCheckBox	sp;
	CMenuCheckBox	mp;
	CMenuCheckBox	lock;
	CMenuCheckBox	additive;
	CMenuCheckBox	precision;
	CMenuCheckBox	stick;
	CMenuCheckBox	latch;
	CMenuPicButton	reset;
	CMenuPicButton	remove;
	CMenuPicButton	save;
	CMenuPicButton	select;
	CMenuPicButton	selectCmd;
	CMenuPicButton	editor;
	CMenuField	command;
	CMenuField	texture;
	CMenuField	name;
	class CMenuColor : public CMenuBaseItem
	{
	public:
		void Draw() override;
	} color;
	class CMenuButtonPreview : public CMenuBaseItem
	{
	public:
		CMenuButtonPreview() : CMenuBaseItem(), textureId( 0 ) { }

		void Draw() override;
		HIMAGE textureId;
	} preview;

	class CButtonListModel : public CMenuBaseArrayModel
	{
	public:
		CButtonListModel( CMenuTouchButtons *parent ) : parent( parent ) { }

		void Update() override;
		void AddButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags );

		const char *GetText( int line ) final override
		{
			return buttons[line].szName;
		}

		int GetRows() const final override
		{
			return buttons.Count();
		}

		struct button_t
		{
			char szName[128];
			char szTexture[128];
			char szCommand[128];
			byte bColors[4];
			int  iFlags;
		};

		CUtlVector<button_t> buttons;

		bool gettingList;
		bool initialized;
		CMenuTouchButtons *parent;
	} model;
	CMenuTable buttonList;

	// prompt dialog
	CMenuYesNoMessageBox msgBox;

	CMenuCommandPickerDialog cmdPicker;

	void OpenCommandPicker();
	void OnCommandPicked( const char *cmd );
};

void CMenuTouchButtons::CButtonListModel::AddButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags )
{
	if( !gettingList )
		return;

	button_t button;

	Q_strncpy( button.szName, name, sizeof( button.szName ) );
	Q_strncpy( button.szTexture, texture, sizeof( button.szTexture ) );
	Q_strncpy( button.szCommand, command, sizeof( button.szCommand ) );
	memcpy( button.bColors, color, sizeof( button.bColors ) );
	button.iFlags = flags;

	buttons.AddToTail( button );
}

void CMenuTouchButtons::CButtonListModel::Update()
{
	if( !initialized )
		return;

	buttons.RemoveAll();

	EngFuncs::ClientCmd( true, "" ); // perform Cbuf_Execute()

	gettingList = true;
	EngFuncs::ClientCmd( true, "touch_list\n" );
	gettingList = false;

	parent->UpdateFields();
}

void CMenuTouchButtons::CMenuColor::Draw()
{
	CMenuTouchButtons *parent = (CMenuTouchButtons *)Parent();

	uint color = ((uint)parent->blue.GetCurrentValue()) |
				(((uint)parent->green.GetCurrentValue()) << 8) |
				(((uint)parent->red.GetCurrentValue()) << 16) |
				(((uint)parent->alpha.GetCurrentValue()) << 24);

	UI_FillRect( m_scPos, m_scSize, color );
}

void CMenuTouchButtons::CMenuButtonPreview::Draw()
{
	CMenuTouchButtons *parent = (CMenuTouchButtons *)Parent();

	UI_FillRect( m_scPos.x - 2, m_scPos.y - 2, m_scSize.w + 4, m_scSize.h + 4, 0xFFC0C0C0 );
	UI_FillRect( m_scPos, m_scSize, 0xFF808080 );

	EngFuncs::PIC_Set( textureId,
		(int)parent->red.GetCurrentValue(),
		(int)parent->green.GetCurrentValue(),
		(int)parent->blue.GetCurrentValue(),
		(int)parent->alpha.GetCurrentValue() );
	if( parent->additive.bChecked )
	{
		EngFuncs::PIC_DrawAdditive( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );
	}
	else
	{
		EngFuncs::PIC_DrawTrans( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );
	}
}

void CMenuTouchButtons::DeleteButton()
{
	int i = buttonList.GetCurrentIndex();
	if( i > 0 ) buttonList.SetCurrentIndex( i - 1 );
	char command[512];
	snprintf(command, 512, "touch_removebutton \"%s\"\n", selectedName );
	EngFuncs::ClientCmd(1, command);
	model.Update();
}

void CMenuTouchButtons::ResetButtons()
{
	EngFuncs::ClientCmd( 0, "touch_removeall\n" );
	EngFuncs::ClientCmd( 1, "touch_loaddefaults\n" );
	model.Update();
}

void CMenuTouchButtons::UpdateFields( )
{
	int i = buttonList.GetCurrentIndex();

	if( !model.buttons.IsValidIndex( i ))
		return;

	Q_strncpy( selectedName, model.buttons[i].szName, sizeof( selectedName ));
	red.SetCurrentValue( model.buttons[i].bColors[0] );
	green.SetCurrentValue( model.buttons[i].bColors[1] );
	blue.SetCurrentValue( model.buttons[i].bColors[2] );
	alpha.SetCurrentValue( model.buttons[i].bColors[3] );

	curflags = model.buttons[i].iFlags;
	mp.bChecked = !!( curflags & TOUCH_FL_MP );
	sp.bChecked = !!( curflags & TOUCH_FL_SP );
	lock.bChecked = !!( curflags & TOUCH_FL_NOEDIT );
	hide.bChecked = !!( curflags & TOUCH_FL_HIDE );
	additive.bChecked = !!( curflags & TOUCH_FL_DRAW_ADDITIVE );
	precision.bChecked = !!( curflags & TOUCH_FL_PRECISION );
	stick.bChecked = !!( curflags & TOUCH_FL_STICK );
	latch.bChecked = !!( curflags & TOUCH_FL_COMMAND_LATCH );

	name.Clear();
	texture.SetBuffer( model.buttons[i].szTexture );
	UpdateTexture();

	command.SetBuffer( model.buttons[i].szCommand );
	UpdateCommandFlags();
}

void CMenuTouchButtons::UpdateCommandFlags()
{
	const char *cmd = command.GetBuffer();

	// these flags only affect specific commands
	stick.SetGrayed( strcmp( cmd, "_joy" ) != 0 );
	latch.SetGrayed( cmd[0] != '+' );
}

void CMenuTouchButtons::OpenFileDialog()
{
	// TODO: Remove uiFileDialogGlobal
	// TODO: Make uiFileDialog menu globally known
	// TODO: Make FileDialogCallback as event
	uiFileDialogGlobal.npatterns = 7;
	Q_strncpy( uiFileDialogGlobal.patterns[0], "touch/*", sizeof( uiFileDialogGlobal.patterns[0] ));
	Q_strncpy( uiFileDialogGlobal.patterns[1], "touch_default/*", sizeof( uiFileDialogGlobal.patterns[0] ));
	Q_strncpy( uiFileDialogGlobal.patterns[2], "gfx/touch/*", sizeof( uiFileDialogGlobal.patterns[0] ));
	Q_strncpy( uiFileDialogGlobal.patterns[3], "gfx/vgui/*", sizeof( uiFileDialogGlobal.patterns[0] ));
	Q_strncpy( uiFileDialogGlobal.patterns[4], "gfx/shell/*", sizeof( uiFileDialogGlobal.patterns[0] ));
	Q_strncpy( uiFileDialogGlobal.patterns[5], "*.tga", sizeof( uiFileDialogGlobal.patterns[0] ));
	Q_strncpy( uiFileDialogGlobal.patterns[6], "*.png", sizeof( uiFileDialogGlobal.patterns[0] ));
	uiFileDialogGlobal.preview = true;
	uiFileDialogGlobal.valid = true;
	uiFileDialogGlobal.callback = CMenuTouchButtons::FileDialogCallback;
	UI_FileDialog_Menu();
}

void CMenuTouchButtons::UpdateTexture()
{
	const char *buf = texture.GetBuffer();
	if( buf[0] && buf[0] != '#' )
		preview.textureId = EngFuncs::PIC_Load( buf );
	else
		preview.textureId = 0;
}

void CMenuTouchButtons::UpdateSP()
{
	if( sp.bChecked )
	{
		curflags |= TOUCH_FL_SP;
		curflags &= ~TOUCH_FL_MP;
		mp.bChecked = false;
	}
	else
	{
		curflags &= ~TOUCH_FL_SP;
	}
}

void CMenuTouchButtons::UpdateMP()
{
	if( mp.bChecked )
	{
		curflags |= TOUCH_FL_MP;
		curflags &= ~TOUCH_FL_SP;
		sp.bChecked = false;
	}
	else
	{
		curflags &= ~TOUCH_FL_MP;
	}
}

void CMenuTouchButtons::SaveButton()
{
	char command[4096];
	char cmd[256];

	Com_EscapeCommand( cmd, this->command.GetBuffer(), 256 );

	if( name.GetBuffer()[0] )
	{
		snprintf( command, sizeof( command ), "touch_addbutton \"%s\" \"%s\" \"%s\"\n",
			name.GetBuffer(),
			texture.GetBuffer(),
			cmd );
		EngFuncs::ClientCmd(0, command);
		snprintf( command, sizeof( command ), "touch_setflags \"%s\" %i\n", name.GetBuffer(), curflags );
		EngFuncs::ClientCmd(0, command);
		snprintf( command, sizeof( command ), "touch_setcolor \"%s\" %u %u %u %u\n", name.GetBuffer(),
			(uint)red.GetCurrentValue(),
			(uint)green.GetCurrentValue(),
			(uint)blue.GetCurrentValue(),
			(uint)alpha.GetCurrentValue() );
		EngFuncs::ClientCmd(1, command);
		name.Clear();
	}
	else
	{
		snprintf( command, sizeof( command ), "touch_settexture \"%s\" \"%s\"\n", selectedName, texture.GetBuffer() );
		EngFuncs::ClientCmd(0, command);
		snprintf( command, sizeof( command ), "touch_setcommand \"%s\" \"%s\"\n", selectedName, cmd );
		EngFuncs::ClientCmd(0, command);
		snprintf( command, sizeof( command ), "touch_setflags \"%s\" %i\n", selectedName, curflags );
		EngFuncs::ClientCmd(0, command);
		snprintf( command, sizeof( command ), "touch_setcolor \"%s\" %u %u %u %u\n", selectedName,
			(uint)red.GetCurrentValue(),
			(uint)green.GetCurrentValue(),
			(uint)blue.GetCurrentValue(),
			(uint)alpha.GetCurrentValue() );
		EngFuncs::ClientCmd(1, command);
	}

	model.Update();
}

void CMenuTouchButtons::RemoveMsgBox()
{
	msgBox.SetMessage( L( "Delete selected button?" ) );					// Delete ??? Может быть Clear ? Или Reset ?
	msgBox.onPositive = VoidCb( &CMenuTouchButtons::DeleteButton );
	msgBox.Show();
}

void CMenuTouchButtons::ResetMsgBox()
{
	msgBox.SetMessage( L( "Reset all buttons?" ) );
	msgBox.onPositive = VoidCb( &CMenuTouchButtons::ResetButtons );
	msgBox.Show();
}

void CMenuTouchButtons::ExitMenuCb(CMenuBaseItem *pSelf, void *pExtra)
{
	const char *cmd = (const char *)pExtra;

	EngFuncs::ClientCmd( 0, cmd );
	pSelf->Parent()->Hide();
}

/*
=================
UI_TouchButtons_Init
=================
*/
void CMenuTouchButtons::_Init( void )
{
	model.initialized = true;

	banner.SetPicture(ART_BANNER);

	done.szName = L( "Done" );
	done.SetPicture( PC_DONE );
	done.onReleased = ExitMenuCb;
	done.onReleased.pExtra = (void*)"touch_writeconfig\n";

	cancel.szName = L( "GameUI_Cancel" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onReleased = ExitMenuCb;
	cancel.onReleased.pExtra = (void*)"touch_reloadconfig\n";

	red.eFocusAnimation = QM_PULSEIFFOCUS;
	red.SetNameAndStatus( L( "Red:" ), L( "Texture red channel" ) );
	red.Setup( 0, 255, 1 );

	green.eFocusAnimation = QM_PULSEIFFOCUS;
	green.SetNameAndStatus( L( "Green:" ), L( "Texture green channel" ) );
	green.Setup( 0, 255, 1 );

	blue.eFocusAnimation = QM_PULSEIFFOCUS;
	blue.SetNameAndStatus( L( "Blue:" ), L( "Texture blue channel" ) );
	blue.Setup( 0, 255, 1 );

	alpha.eFocusAnimation = QM_PULSEIFFOCUS;
	alpha.SetNameAndStatus( L( "Alpha:" ), L( "Texture alpha channel" ) );
	alpha.Setup( 0, 255, 1 );

	hide.SetNameAndStatus( L( "Hide" ), L( "Show/hide button" ) );
	hide.iMask = TOUCH_FL_HIDE;
	hide.onChanged.pExtra = &curflags;
	hide.onChanged = CMenuCheckBox::BitMaskCb;

	additive.SetNameAndStatus( L( "Additive" ), L( "Set button additive draw mode" ) );
	additive.iMask = TOUCH_FL_DRAW_ADDITIVE;
	additive.onChanged.pExtra = &curflags;
	additive.onChanged = CMenuCheckBox::BitMaskCb;

	mp.SetNameAndStatus( L( "MP" ), L( "Show button only in multiplayer" ) );
	mp.onChanged = VoidCb( &CMenuTouchButtons::UpdateMP );

	sp.SetNameAndStatus( L( "SP" ), L( "Show button only in singleplayer" ) );
	sp.onChanged = VoidCb( &CMenuTouchButtons::UpdateSP );

	lock.SetNameAndStatus( L( "Lock" ), L( "Lock button editing" ) );
	lock.iMask = TOUCH_FL_NOEDIT;
	lock.onChanged.pExtra = &curflags;
	lock.onChanged = CMenuCheckBox::BitMaskCb;

	precision.SetNameAndStatus( L( "Look precision" ), L( "Increase look precision" ) );
	precision.iMask = TOUCH_FL_PRECISION;
	precision.onChanged.pExtra = &curflags;
	precision.onChanged = CMenuCheckBox::BitMaskCb;

	stick.SetNameAndStatus( L( "Analog stick" ), L( "Move with an analog stick, for _joy buttons" ) );
	stick.iMask = TOUCH_FL_STICK;
	stick.onChanged.pExtra = &curflags;
	stick.onChanged = CMenuCheckBox::BitMaskCb;

	latch.SetNameAndStatus( L( "Toggle" ), L( "Press once to hold, press again to release, for + commands" ) );
	latch.iMask = TOUCH_FL_COMMAND_LATCH;
	latch.onChanged.pExtra = &curflags;
	latch.onChanged = CMenuCheckBox::BitMaskCb;

	save.SetNameAndStatus( L( "GameUI_Save" ), L( "Save as new button" ) );
	save.SetPicture( PC_TOUCH_SAVE );
	save.onReleased = VoidCb( &CMenuTouchButtons::SaveButton );

	editor.SetNameAndStatus( L( "Editor" ), L( "Open interactive editor" ) );
	editor.SetPicture( PC_TOUCH_EDITOR );
	editor.onReleased = UI_TouchEdit_Menu;

	select.SetNameAndStatus( L( "Select" ), L( "Select texture from list" ) );
	select.SetPicture( PC_TOUCH_SELECT );
	select.onReleased = VoidCb( &CMenuTouchButtons::OpenFileDialog );

	selectCmd.SetNameAndStatus( L( "Pick" ), L( "Pick command from key bindings list" ) );
	selectCmd.SetPicture( PC_TOUCH_SELECT );
	selectCmd.onReleased = VoidCb( &CMenuTouchButtons::OpenCommandPicker );
	cmdPicker.pCaller = this;
	cmdPicker.Link( this );

	name.szName = L( "New Button:" );
	name.iMaxLength = 255;

	command.szName = L( "Command:" );
	command.iMaxLength = 255;
	command.onChanged = VoidCb( &CMenuTouchButtons::UpdateCommandFlags );

	texture.szName = L( "Texture:" );
	texture.iMaxLength = 255;
	texture.onChanged = VoidCb( &CMenuTouchButtons::UpdateTexture );
	texture.eTextAlignment = QM_RIGHT;

	reset.SetNameAndStatus( L( "Reset" ), L( "Reset touch to default state" ) );
	reset.SetPicture( PC_TOUCH_RESET );
	reset.onReleased = VoidCb( &CMenuTouchButtons::ResetMsgBox );

	remove.SetNameAndStatus( L( "Delete" ), L( "Delete selected button" ) );		// Delete - уже было раньше
	remove.SetPicture( PC_DELETE );
	remove.onReleased = VoidCb( &CMenuTouchButtons::RemoveMsgBox );

	buttonList.SetModel( &model );
	buttonList.onChanged = VoidCb( &CMenuTouchButtons::UpdateFields );
	UpdateFields();
	msgBox.Link( this );

	AddItem( remove );
	AddItem( reset );
	AddItem( done );
	AddItem( cancel );
	AddItem( red );
	AddItem( green );
	AddItem( blue );
	AddItem( alpha );
	AddItem( hide );
	AddItem( additive );
	AddItem( precision );
	AddItem( stick );
	AddItem( latch );
	AddItem( sp );
	AddItem( mp );
	AddItem( lock );

	AddItem( buttonList );

	AddItem( save );
	AddItem( select );
	AddItem( selectCmd );
	AddItem( editor );

	AddItem( banner );
	AddItem( color );
	AddItem( preview );
	AddItem( command );
	AddItem( texture );
	AddItem( name );
}

void CMenuTouchButtons::_VidInit()
{
	int sliders_x = uiStatic.width - 344;
	int field_w = sliders_x - 400 - 100; // leave room for 95-wide select button + 5px gap
	if( field_w < 175 ) field_w = 175;

	banner.SetCoord( 72, 0 );
	done.SetCoord( 72, 550 );
	cancel.SetCoord( 72, 600 );

	red.SetCoord( sliders_x, 150 );
	green.SetCoord( sliders_x, 210 );
	blue.SetCoord( sliders_x, 270 );
	alpha.SetCoord( sliders_x, 330 );

	additive.SetCoord( 650, 465 );

	mp.SetCoord( 400, 420 );
	sp.SetCoord( 160 - 72 + 400, 420 );
	lock.SetCoord( 256 - 72 + 400, 420 );
	hide.SetCoord( 384 - 72 + 400, 420 );

	precision.SetCoord( 400, 465 );
	stick.SetCoord( 400, 510 );
	latch.SetCoord( 650, 510 );
	buttonList.SetRect( 72, 135, 300, 395 );

	save.SetRect( 384 - 42 + 320, 550, 170, 50 );
	editor.SetRect( 384 - 42 + 320, 600, 170, 50 );

	name.SetRect( 400, 550, 205, 32 );
	command.SetRect( 400, 150, field_w, 32 );
	selectCmd.SetRect( 400 + field_w + 5, 150, 95, 32 );
	texture.SetRect( 400, 250, field_w, 32 );
	select.SetRect( 400 + field_w + 5, 250, 95, 32 );
	color.SetRect( sliders_x + 120, 360, 70, 50 );
	preview.SetRect( 400, 300, 70, 70 );

	reset.SetRect( 384 - 72 + 520, 600, 170, 50 );
	remove.SetRect( 384 - 72 + 520, 550, 170, 50 );
}

ADD_MENU3( menu_touchbuttons, CMenuTouchButtons, UI_TouchButtons_Menu );

void CMenuTouchButtons::FileDialogCallback( bool success )
{
	if( success )
	{
		menu_touchbuttons->texture.SetBuffer( uiFileDialogGlobal.result );
		menu_touchbuttons->UpdateTexture();
	}
}

void CMenuTouchButtons::OpenCommandPicker()
{
	cmdPicker.Show();
}

void CMenuTouchButtons::OnCommandPicked( const char *cmd )
{
	command.SetBuffer( cmd );
	UpdateCommandFlags();
}

void CMenuCommandPickerDialog::Confirm()
{
	int i = list.GetCurrentIndex();
	if( !model.IsLineUsable( i ))
		return;

	if( pCaller )
		pCaller->OnCommandPicked( model.entries[i].bind );

	Hide();
}

void CMenuCommandPickerDialog::_Init()
{
	title.szName = L( "Pick command" );
	title.iFlags = QMF_INACTIVE | QMF_DROPSHADOW;
	title.eTextAlignment = QM_TOP;
	title.SetCharSize( QM_DEFAULTFONT );
	title.SetRect( 0, 16, 720, 32 );

	list.SetRect( 16, 56, 720 - 32, 480 - 56 - 64 );
	list.SetModel( &model );
	list.SetupColumn( 0, L( "GameUI_Action" ), 0.6f );
	list.SetupColumn( 1, L( "Command" ), 0.4f );

	ok.szName = L( "GameUI_OK" );
	ok.SetPicture( PC_OK );
	ok.SetRect( 720 / 2 - UI_BUTTONS_WIDTH / 2 - 8, 480 - 56, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );
	SET_EVENT_MULTI( ok.onReleased,
	{
		((CMenuCommandPickerDialog *)pSelf->Parent())->Confirm();
	});

	cancel.szName = L( "GameUI_Cancel" );
	cancel.SetPicture( PC_CANCEL );
	cancel.SetRect( 720 / 2 + 8, 480 - 56, UI_BUTTONS_WIDTH / 2, UI_BUTTONS_HEIGHT );
	SET_EVENT_MULTI( cancel.onReleased,
	{
		((CMenuCommandPickerDialog *)pSelf->Parent())->Hide();
	});

	AddItem( title );
	AddItem( list );
	AddItem( ok );
	AddItem( cancel );
}

void CMenuCommandPickerDialog::_VidInit()
{
	SetRect(( uiStatic.width - 720 ) / 2, ( 768 - 480 ) / 2, 720, 480 );
	pos.x += uiStatic.xOffset;
	pos.y += uiStatic.yOffset;
	CalcPosition();
	CalcSizes();
}

void CMenuCommandPickerDialog::Draw()
{
	UI_FillRect( 0, 0, gpGlobals->scrWidth, gpGlobals->scrHeight, 0x40000000 );

	EngFuncs::FillRGBA( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, 20, 20, 20, 235 );
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );

	EngFuncs::FillRGBA(
		m_scPos.x + 16 * uiStatic.scaleX,
		m_scPos.y + ( 16 + 32 ) * uiStatic.scaleY,
		m_scSize.w - 32 * uiStatic.scaleX,
		1, 255, 255, 255, 40 );

	BaseClass::Draw();
}

bool CMenuCommandPickerDialog::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ))
	{
		Hide();
		return true;
	}
	return BaseClass::KeyDown( key );
}

void CMenuCommandPickerDialog::Show()
{
	model.Update();
	BaseClass::Show();
}

/*
=================
UI_TouchButtons_Menu
=================
*/
void UI_TouchButtons_Menu( void )
{
	UI_TouchButtons_GetButtonList();
	menu_touchbuttons->Show();
}

void UI_TouchButtons_GetButtonList()
{
	menu_touchbuttons->model.Update();
}

// Engine callback
extern "C" EXPORT void AddTouchButtonToList( const char *name, const char *texture, const char *command, unsigned char *color, int flags )
{
	menu_touchbuttons->model.AddButtonToList( name, texture, command, color, flags );
}
