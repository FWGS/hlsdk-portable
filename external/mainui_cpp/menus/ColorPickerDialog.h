/*
ColorPickerDialog.h - color picker dialog window
Copyright (C) 2026 $_Vladislav

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#pragma once
#ifndef COLORPICKERDIALOG_H
#define COLORPICKERDIALOG_H

#include "Framework.h"
#include "PicButton.h"
#include "Field.h"
#include "SpinControl.h"
#include "ColorPicker.h"
#include "StringArrayModel.h"

#define MAX_LOGO_STRIPES 8

class CMenuColorPickerDialog : public CMenuBaseWindow
{
public:
	typedef CMenuBaseWindow BaseClass;
	CMenuColorPickerDialog();

	void Show( const byte ( *stripes )[3], int stripeCount, bool horizontal, HIMAGE logoImage );

	void GetStripes( byte ( *out )[3], int &count, bool &horizontal ) const
	{
		count = m_stripeCount;
		horizontal = m_horizontal;
		for( int i = 0; i < m_stripeCount; i++ )
		{
			out[i][0] = m_stripes[i][0];
			out[i][1] = m_stripes[i][1];
			out[i][2] = m_stripes[i][2];
		}
	}

	CEventCallback onOk;

private:
	void _Init() override;
	void _VidInit() override;
	void Draw() override;
	void Think() override;

	void UpdateLogoPreview();
	void UpdateFieldsFromPicker();
	void UpdatePickerFromFields();
	void SwitchMode( bool toRGB );

	void LoadSelectedStripeIntoPicker();
	void OnPickerChanged();
	void OnStripeCountChanged();
	void OnPresetSelected();
	void OnOrientationChanged();
	void DetectAndSyncPreset();

	CMenuColorPicker  m_picker;
	CMenuPicButton    m_btnOk;
	CMenuPicButton    m_btnCancel;
	CMenuSpinControl  m_modeSwitch;
	CMenuField        m_fields[3];
	CMenuSpinControl  m_stripeCountSpin;
	CMenuSpinControl  m_presetSpin;
	CMenuSpinControl  m_orientationSpin;
	bool              m_modeRGB;
	bool              m_updatingFields;
	bool              m_detectPresetPending;
	bool              m_horizontal;

	byte m_stripes[MAX_LOGO_STRIPES][3];
	int  m_stripeCount;
	int  m_selectedStripe;

	struct FieldLabel { Point pos; Size size; const char *text; } m_labels[3];

	class CLogoPreview : public CMenuBaseItem
	{
	public:
		void Draw() override;
		HIMAGE hImage = 0;
		const byte ( *stripes )[3] = nullptr;
		int stripeCount = 1;
		const bool *horizontal = nullptr;
	} m_logoPreviewItem;

	class CSwatchRow : public CMenuBaseItem
	{
	public:
		CSwatchRow();
		void Draw() override;
		bool KeyDown( int key ) override;

		const byte ( *stripes )[3] = nullptr;
		const int *stripeCount = nullptr;
		int *selected = nullptr;
		CEventCallback onSwatchSelected;

	private:
		void GetSwatchRect( int idx, Point &p, Size &s ) const;
	} m_swatchRow;
};

#endif // COLORPICKERDIALOG_H
