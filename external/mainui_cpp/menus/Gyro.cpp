/*
Copyright (C) 2026 Vladislav Sukhov

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

#include "build.h"
#include "Framework.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Slider.h"
#include "CheckBox.h"
#include "Action.h"

#define ART_BANNER			"gfx/shell/head_gyro"

class CMenuGyro : public CMenuFramework
{
public:
	CMenuGyro() : CMenuFramework( "CMenuGyro" ), m_bGamepadMode( false ) { }

	void SetGamepadMode( bool isGamepad ) { m_bGamepadMode = isGamepad; }

private:
	void _Init() override;
	void _VidInit() override;
	void GetConfig();
	void SaveAndPopMenu() override;

	bool m_bGamepadMode;

	CMenuCheckBox enable;
	CMenuSlider pitch, yaw, roll;
	CMenuCheckBox invPitch, invYaw, invRoll;
	CMenuSlider dPitch, dYaw, dRoll;
	CMenuAction sensitivity_label;
	CMenuAction deadzone_label;
};

/*
=================
CMenuGyro::GetConfig
=================
*/
void CMenuGyro::GetConfig( void )
{
	float _pitch, _yaw, _roll;

	if( m_bGamepadMode )
	{
		enable.SetNameAndStatus( L( "Enable Gamepad Gyroscope" ), L( "Use gamepad motion sensors for aiming" ) );
		enable.LinkCvar( "joy_gyro_enable" );

		_pitch = EngFuncs::GetCvarFloat( "joy_gyro_pitch" );
		_yaw = EngFuncs::GetCvarFloat( "joy_gyro_yaw" );
		_roll = EngFuncs::GetCvarFloat( "joy_gyro_roll" );

		dPitch.LinkCvar( "joy_gyro_pitch_deadzone" );
		dYaw.LinkCvar( "joy_gyro_yaw_deadzone" );
		dRoll.LinkCvar( "joy_gyro_roll_deadzone" );
	}
	else
	{
		enable.SetNameAndStatus( L( "Enable Built-in Gyroscope" ), L( "Use device motion sensors for aiming" ) );
		enable.LinkCvar( "gyro_enable" );

		_pitch = EngFuncs::GetCvarFloat( "gyro_pitch" );
		_yaw = EngFuncs::GetCvarFloat( "gyro_yaw" );
		_roll = EngFuncs::GetCvarFloat( "gyro_roll" );

		dPitch.LinkCvar( "gyro_pitch_deadzone" );
		dYaw.LinkCvar( "gyro_yaw_deadzone" );
		dRoll.LinkCvar( "gyro_roll_deadzone" );
	}

	pitch.SetCurrentValue( fabs( _pitch ) );
	yaw.SetCurrentValue( fabs( _yaw ) );
	roll.SetCurrentValue( fabs( _roll ) );

	invPitch.bChecked = _pitch < 0.0f;
	invYaw.bChecked = _yaw < 0.0f;
	invRoll.bChecked = _roll < 0.0f;
}

/*
=================
CMenuGyro::SaveAndPopMenu
=================
*/
void CMenuGyro::SaveAndPopMenu()
{
	float _pitch, _yaw, _roll;

	enable.WriteCvar();

	_pitch = pitch.GetCurrentValue();
	if( invPitch.bChecked ) _pitch *= -1.0f;

	_yaw = yaw.GetCurrentValue();
	if( invYaw.bChecked ) _yaw *= -1.0f;

	_roll = roll.GetCurrentValue();
	if( invRoll.bChecked ) _roll *= -1.0f;

	if( m_bGamepadMode )
	{
		EngFuncs::CvarSetValue( "joy_gyro_pitch", _pitch );
		EngFuncs::CvarSetValue( "joy_gyro_yaw", _yaw );
		EngFuncs::CvarSetValue( "joy_gyro_roll", _roll );
	}
	else
	{
		EngFuncs::CvarSetValue( "gyro_pitch", _pitch );
		EngFuncs::CvarSetValue( "gyro_yaw", _yaw );
		EngFuncs::CvarSetValue( "gyro_roll", _roll );
	}

	dPitch.WriteCvar();
	dYaw.WriteCvar();
	dRoll.WriteCvar();

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
CMenuGyro::_Init
=================
*/
void CMenuGyro::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	sensitivity_label.eTextAlignment = QM_CENTER;
	sensitivity_label.iFlags = QMF_INACTIVE | QMF_DROPSHADOW;
	sensitivity_label.colorBase = uiColorHelp;
	sensitivity_label.szName = L( "Sensitivity" );

	pitch.Setup( 0.0f, 20.0f, 0.1f );
	pitch.SetNameAndStatus( L( "Pitch" ), L( "Vertical gyro sensitivity" ) );
	invPitch.SetNameAndStatus( L( "Invert" ), L( "Invert vertical gyro axis" ) );

	yaw.Setup( 0.0f, 20.0f, 0.1f );
	yaw.SetNameAndStatus( L( "Yaw" ), L( "Horizontal gyro sensitivity" ) );
	invYaw.SetNameAndStatus( L( "Invert" ), L( "Invert horizontal gyro axis" ) );

	roll.Setup( 0.0f, 20.0f, 0.1f );
	roll.SetNameAndStatus( L( "Roll" ), L( "Sideways tilt gyro sensitivity" ) );
	invRoll.SetNameAndStatus( L( "Invert" ), L( "Invert roll gyro axis" ) );

	deadzone_label.eTextAlignment = QM_CENTER;
	deadzone_label.iFlags = QMF_INACTIVE | QMF_DROPSHADOW;
	deadzone_label.colorBase = uiColorHelp;
	deadzone_label.szName = L( "Deadzones" );

	dPitch.Setup( 0.0f, 10.0f, 0.1f );
	dPitch.SetNameAndStatus( L( "Pitch" ), L( "Gyro pitch deadzone" ) );

	dYaw.Setup( 0.0f, 10.0f, 0.1f );
	dYaw.SetNameAndStatus( L( "Yaw" ), L( "Gyro yaw deadzone" ) );

	dRoll.Setup( 0.0f, 10.0f, 0.1f );
	dRoll.SetNameAndStatus( L( "Roll" ), L( "Gyro roll deadzone" ) );

	AddItem( banner );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuGyro::SaveAndPopMenu ) );
	AddButton( L( "GameUI_Cancel" ), nullptr, PC_CANCEL, VoidCb( &CMenuFramework::Hide ) );

	AddItem( enable );
	AddItem( sensitivity_label );
	AddItem( pitch );
	AddItem( invPitch );
	AddItem( yaw );
	AddItem( invYaw );
	AddItem( roll );
	AddItem( invRoll );
	AddItem( deadzone_label );
	AddItem( dPitch );
	AddItem( dYaw );
	AddItem( dRoll );
}

/*
=================
CMenuGyro::_VidInit
=================
*/
void CMenuGyro::_VidInit()
{
	// Left column: Buttons and Sensitivity
	int lx = 72;
	int ly = 350;

	enable.SetCoord( lx, ly );
	ly += 70;

	sensitivity_label.SetCoord( lx, ly );
	
	// Right column: Deadzones
	int rx = 550;
	int ry = ly; // Align Deadzone header with Sensitivity header

	deadzone_label.SetCoord( rx, ry );

	// Increments for sliders
	ly += 70;
	ry += 70;

	int sliderAlign = invPitch.size.h - pitch.size.h;

	// Pitch row
	pitch.SetCoord( lx, ly + sliderAlign );
	invPitch.SetCoord( lx + 220, ly );
	dPitch.SetCoord( rx, ry + sliderAlign );

	// Yaw row
	ly += 55;
	ry += 55;
	yaw.SetCoord( lx, ly + sliderAlign );
	invYaw.SetCoord( lx + 220, ly );
	dYaw.SetCoord( rx, ry + sliderAlign );

	// Roll row
	ly += 55;
	ry += 55;
	roll.SetCoord( lx, ly + sliderAlign );
	invRoll.SetCoord( lx + 220, ly );
	dRoll.SetCoord( rx, ry + sliderAlign );

	GetConfig();
}

ADD_MENU( menu_gyro, CMenuGyro, UI_Gyro_Menu );

void UI_GamePadGyro_Menu( void )
{
	menu_gyro->SetGamepadMode( true );
	menu_gyro->Show();
}

void UI_MobileGyro_Menu( void )
{
	menu_gyro->SetGamepadMode( false );
	menu_gyro->Show();
}
