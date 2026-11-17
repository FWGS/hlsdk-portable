/*
ColorPickerDialog.cpp - color picker dialog window
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

#include "ColorPickerDialog.h"
#include "BaseMenu.h"
#include "extdll_menu.h"
#include "fmtstr.h"
#include "IntegerRangeModel.h"

namespace Layout
{
	static const int DLG_W              = 720;
	static const int DLG_H              = 480;
	static const int PAD                = 20;
	static const int TITLE_H            = 28;
	static const int PICKER_W           = 246;
	static const int PICKER_H           = 220;
	static const int LOGO_W             = 180;
	static const int LOGO_H             = 180;
	static const int BTN_W              = 120;
	static const int BTN_H              = 36;
	static const int FIELD_W            = 56;
	static const int FIELD_H            = 28;
	static const int LABEL_W            = 18;
	static const int MODE_W             = 140;
	static const int INPUT_X            = 130;
	static const int UI_VIRTUAL_HEIGHT  = 768;

	static const int CONTENT_GAP        = 16;
	static const int INPUT_GAP          = 24;
	static const int INPUT_GROUP_GAP    = 14;
	static const int FIELD_GAP          = 8;
	static const int LABEL_GAP          = 4;
	static const int LABEL_Y_OFFSET     = 4;
	static const int BTN_GAP            = 16;

	static const int SWATCH_ROW_H       = 44;
	static const int SWATCH_GAP         = 8;

	static const int MID_LABEL_AREA     = 32;
	static const int MID_GAP            = 14;
}

struct logo_preset_t
{
	const char *name;
	int stripes;
	byte rgb[MAX_LOGO_STRIPES * 3];
};

static const logo_preset_t g_LogoPresets[] =
{
	{
		"Custom",
		1,
		{ 255, 255, 255 },
	},
	{
		"Pride",
		6,
		{
			0xE4, 0x03, 0x03,
			0xFF, 0x8C, 0x00,
			0xFF, 0xED, 0x00,
			0x00, 0x80, 0x26,
			0x24, 0x40, 0x8E,
			0x73, 0x29, 0x82,
		},
	},
	{
		"Lesbian",
		7,
		{
			0xD5, 0x2D, 0x00,
			0xEF, 0x76, 0x27,
			0xFF, 0x9A, 0x56,
			0xFF, 0xFF, 0xFF,
			0xD1, 0x62, 0xA4,
			0xB5, 0x56, 0x90,
			0xA3, 0x02, 0x62,
		},
	},
	{
		"Gay",
		7,
		{
			0x07, 0x8D, 0x70,
			0x26, 0xCE, 0xAA,
			0x98, 0xE8, 0xC1,
			0xFF, 0xFF, 0xFF,
			0x7B, 0xAD, 0xE2,
			0x50, 0x49, 0xCC,
			0x3D, 0x1A, 0x78,
		},
	},
	{
		"Bi",
		5,
		{
			0xD6, 0x02, 0x70,
			0xD6, 0x02, 0x70,
			0x9B, 0x4F, 0x96,
			0x00, 0x38, 0xA8,
			0x00, 0x38, 0xA8,
		},
	},
	{
		"Trans",
		5,
		{
			0x5B, 0xCE, 0xFA,
			0xF5, 0xA9, 0xB8,
			0xFF, 0xFF, 0xFF,
			0xF5, 0xA9, 0xB8,
			0x5B, 0xCE, 0xFA,
		},
	},
	{
		"Pan",
		3,
		{
			0xFF, 0x21, 0x8C,
			0xFF, 0xD8, 0x00,
			0x21, 0xB1, 0xFF,
		},
	},
	{
		"Nonbinary",
		4,
		{
			0xFC, 0xF4, 0x34,
			0xFF, 0xFF, 0xFF,
			0x9C, 0x59, 0xD1,
			0x2C, 0x2C, 0x2C,
		},
	},
};

CMenuColorPickerDialog::CMenuColorPickerDialog()
	: CMenuBaseWindow( "CMenuColorPickerDialog" ),
	  m_modeRGB( true ), m_updatingFields( false ),
	  m_detectPresetPending( false ), m_horizontal( false ),
	  m_stripeCount( 1 ), m_selectedStripe( 0 )
{
	memset( m_stripes, 255, sizeof( m_stripes ));
}

CMenuColorPickerDialog::CSwatchRow::CSwatchRow()
{
	size.w = 0;
	size.h = Layout::SWATCH_ROW_H;
}

void CMenuColorPickerDialog::CSwatchRow::GetSwatchRect( int idx, Point &p, Size &s ) const
{
	const int count = *stripeCount;
	const int gap = Layout::SWATCH_GAP * uiStatic.scaleX;
	int sw = ( m_scSize.w - gap * ( count - 1 )) / count;
	if( sw < 1 ) sw = 1;

	p.x = m_scPos.x + idx * ( sw + gap );
	p.y = m_scPos.y;
	s.w = sw;
	s.h = m_scSize.h;
}

void CMenuColorPickerDialog::CSwatchRow::Draw()
{
	const int count = *stripeCount;
	const int sel   = *selected;

	for( int i = 0; i < count; i++ )
	{
		Point p; Size s;
		GetSwatchRect( i, p, s );

		EngFuncs::FillRGBA( p.x, p.y, s.w, s.h, stripes[i][0], stripes[i][1], stripes[i][2], 255 );

		if( i == sel )
		{
			UI_DrawRectangle( p, s, PackRGBA( 255, 255, 255, 255 ));
			Point p2 = { p.x + 1, p.y + 1 };
			Size  s2 = { s.w - 2, s.h - 2 };
			UI_DrawRectangle( p2, s2, PackRGBA( 0, 0, 0, 255 ));
		}
		else
		{
			UI_DrawRectangle( p, s, PackRGBA( 0, 0, 0, 200 ));
		}
	}
}

bool CMenuColorPickerDialog::CSwatchRow::KeyDown( int key )
{
	if( key != K_MOUSE1 )
		return false;

	const int count = *stripeCount;
	for( int i = 0; i < count; i++ )
	{
		Point p; Size s;
		GetSwatchRect( i, p, s );
		if( !UI_CursorInRect( p, s ))
			continue;

		if( *selected != i )
		{
			*selected = i;
			if( onSwatchSelected )
				onSwatchSelected( this );
		}
		return true;
	}
	return false;
}

void CMenuColorPickerDialog::_Init()
{
	SetRect( ( uiStatic.width - Layout::DLG_W ) / 2, ( Layout::UI_VIRTUAL_HEIGHT - Layout::DLG_H ) / 2, Layout::DLG_W, Layout::DLG_H );

	int currentY = Layout::PAD + Layout::TITLE_H + Layout::CONTENT_GAP;

	m_picker.SetRect( Layout::PAD, currentY, Layout::PICKER_W, Layout::PICKER_H );
	SET_EVENT_MULTI( m_picker.onChanged,
	{
		((CMenuColorPickerDialog *)pSelf->Parent())->OnPickerChanged();
	});

	int logoX = Layout::DLG_W - Layout::PAD - Layout::LOGO_W;
	int logoY = currentY + ( Layout::PICKER_H - Layout::LOGO_H ) / 2;
	m_logoPreviewItem.SetRect( logoX, logoY, Layout::LOGO_W, Layout::LOGO_H );
	m_logoPreviewItem.stripes = m_stripes;
	m_logoPreviewItem.stripeCount = m_stripeCount;
	m_logoPreviewItem.horizontal = &m_horizontal;

	const int midX = Layout::PAD + Layout::PICKER_W + Layout::MID_GAP;
	const int midW = Layout::DLG_W - 2 * Layout::PAD - Layout::LOGO_W - 2 * Layout::MID_GAP - Layout::PICKER_W;
	const int rowStep = Layout::MID_LABEL_AREA + Layout::FIELD_H + Layout::MID_GAP;
	int rowY = currentY;

	static CIntegerRangeModel countModel( 1, MAX_LOGO_STRIPES );
	m_stripeCountSpin.Setup( &countModel );
	m_stripeCountSpin.SetCurrentValue( 0.f );
	m_stripeCountSpin.SetRect( midX, rowY + Layout::MID_LABEL_AREA, midW, Layout::FIELD_H );
	SET_EVENT_MULTI( m_stripeCountSpin.onChanged,
	{
		((CMenuColorPickerDialog *)pSelf->Parent())->OnStripeCountChanged();
	});

	rowY += rowStep;

	static const char *presetItems[V_ARRAYSIZE( g_LogoPresets )];
	for( size_t i = 0; i < V_ARRAYSIZE( g_LogoPresets ); i++ )
		presetItems[i] = L( g_LogoPresets[i].name );
	static CStringArrayModel presetModel( presetItems, V_ARRAYSIZE( g_LogoPresets ));

	m_presetSpin.Setup( &presetModel );
	m_presetSpin.SetCurrentValue( 0.f );
	m_presetSpin.SetRect( midX, rowY + Layout::MID_LABEL_AREA, midW, Layout::FIELD_H );
	SET_EVENT_MULTI( m_presetSpin.onChanged,
	{
		((CMenuColorPickerDialog *)pSelf->Parent())->OnPresetSelected();
	});

	rowY += rowStep;

	static const char *orientationItems[] = { L( "Vertical" ), L( "Horizontal" ) };
	static CStringArrayModel orientationModel( orientationItems, 2 );
	m_orientationSpin.Setup( &orientationModel );
	m_orientationSpin.SetCurrentValue( 0.f );
	m_orientationSpin.SetRect( midX, rowY + Layout::MID_LABEL_AREA, midW, Layout::FIELD_H );
	SET_EVENT_MULTI( m_orientationSpin.onChanged,
	{
		((CMenuColorPickerDialog *)pSelf->Parent())->OnOrientationChanged();
	});

	currentY += Layout::PICKER_H + Layout::INPUT_GAP;

	m_swatchRow.SetRect( Layout::PAD, currentY, Layout::DLG_W - Layout::PAD * 2, Layout::SWATCH_ROW_H );
	m_swatchRow.stripes     = m_stripes;
	m_swatchRow.stripeCount = &m_stripeCount;
	m_swatchRow.selected    = &m_selectedStripe;
	SET_EVENT_MULTI( m_swatchRow.onSwatchSelected,
	{
		((CMenuColorPickerDialog *)pSelf->Parent())->LoadSelectedStripeIntoPicker();
	});

	currentY += Layout::SWATCH_ROW_H + Layout::INPUT_GAP;

	static const char *modeItems[] = { "RGB", "HSV" };
	static CStringArrayModel modeModel( modeItems, 2 );

	m_modeSwitch.Setup( &modeModel );
	m_modeSwitch.SetCurrentValue( 0.f );
	m_modeSwitch.SetRect( Layout::INPUT_X, currentY, Layout::MODE_W, Layout::FIELD_H );
	SET_EVENT_MULTI( m_modeSwitch.onChanged,
	{
		CMenuColorPickerDialog *dlg = (CMenuColorPickerDialog *)pSelf->Parent();
		dlg->SwitchMode( ((CMenuSpinControl *)pSelf)->GetCurrentValue() < 0.5f );
	});

	static const char *rgbLabels[3] = { "R", "G", "B" };
	int fieldX = Layout::INPUT_X + Layout::MODE_W + Layout::INPUT_GROUP_GAP;

	for( int i = 0; i < 3; i++ )
	{
		int lx = fieldX + i * ( Layout::LABEL_W + Layout::FIELD_W + Layout::FIELD_GAP );
		int fx = lx + Layout::LABEL_W + Layout::LABEL_GAP;

		m_labels[i].pos  = { lx, currentY + Layout::LABEL_Y_OFFSET };
		m_labels[i].size = { Layout::LABEL_W, Layout::FIELD_H };
		m_labels[i].text = rgbLabels[i];

		m_fields[i].iMaxLength   = 3;
		m_fields[i].bNumbersOnly = true;
		m_fields[i].SetRect( fx, currentY, Layout::FIELD_W, Layout::FIELD_H );

		SET_EVENT_MULTI( m_fields[i].onChanged,
		{
			((CMenuColorPickerDialog *)pSelf->Parent())->UpdatePickerFromFields();
		});
	}

	const int buttonY  = Layout::DLG_H - Layout::PAD - Layout::BTN_H;
	const int buttonsX = ( Layout::DLG_W - ( Layout::BTN_W * 2 + Layout::BTN_GAP )) / 2;

	m_btnOk.SetPicture( PC_OK );
	m_btnOk.szName = L( "GameUI_OK" );
	m_btnOk.eTextAlignment = QM_CENTER;
	m_btnOk.SetRect( buttonsX, buttonY, Layout::BTN_W, Layout::BTN_H );
	SET_EVENT_MULTI( m_btnOk.onReleased,
	{
		CMenuColorPickerDialog *dlg = (CMenuColorPickerDialog *)pSelf->Parent();
		if( dlg->onOk )
			dlg->onOk( dlg );
		dlg->Hide();
	});

	m_btnCancel.SetPicture( PC_CANCEL );
	m_btnCancel.szName = L( "GameUI_Cancel" );
	m_btnCancel.eTextAlignment = QM_CENTER;
	m_btnCancel.SetRect( buttonsX + Layout::BTN_W + Layout::BTN_GAP, buttonY, Layout::BTN_W, Layout::BTN_H );
	SET_EVENT_MULTI( m_btnCancel.onReleased,
	{
		((CMenuColorPickerDialog *)pSelf->Parent())->Hide();
	});

	AddItem( m_picker );
	AddItem( m_logoPreviewItem );
	AddItem( m_stripeCountSpin );
	AddItem( m_presetSpin );
	AddItem( m_orientationSpin );
	AddItem( m_swatchRow );
	AddItem( m_modeSwitch );
	for( int i = 0; i < 3; i++ )
		AddItem( m_fields[i] );
	AddItem( m_btnOk );
	AddItem( m_btnCancel );
}

void CMenuColorPickerDialog::_VidInit()
{
	SetRect( ( uiStatic.width - Layout::DLG_W ) / 2, ( Layout::UI_VIRTUAL_HEIGHT - Layout::DLG_H ) / 2, Layout::DLG_W, Layout::DLG_H );
	pos.x += uiStatic.xOffset;
	pos.y += uiStatic.yOffset;
}

void CMenuColorPickerDialog::Draw()
{
	UI_FillRect( 0, 0, gpGlobals->scrWidth, gpGlobals->scrHeight, 0x40000000 );

	EngFuncs::FillRGBA( m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h, 20, 20, 20, 235 );

	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );

	UI_DrawString( font, m_scPos.x, m_scPos.y + Layout::PAD * uiStatic.scaleY, m_scSize.w, Layout::TITLE_H * uiStatic.scaleY, L( "Logo Color" ), uiColorHelp, m_scChSize, QM_CENTER, ETF_SHADOW );

	EngFuncs::FillRGBA( m_scPos.x + Layout::PAD * uiStatic.scaleX, m_scPos.y + ( Layout::PAD + Layout::TITLE_H ) * uiStatic.scaleY, m_scSize.w - Layout::PAD * 2 * uiStatic.scaleX, 1, 255, 255, 255, 40 );

	CMenuSpinControl *const midSpins[3] = { &m_stripeCountSpin, &m_presetSpin, &m_orientationSpin };
	const char *const midLabels[3] = { L( "Stripes:" ), L( "Preset:" ), L( "Direction:" ) };
	const int fontH = g_FontMgr->GetFontTall( font );
	const int labelGap = fontH / 3;
	for( int i = 0; i < 3; i++ )
	{
		Point sp = midSpins[i]->GetRenderPosition();
		Size  sz = midSpins[i]->GetRenderSize();
		UI_DrawString( font, sp.x, sp.y - fontH - labelGap, sz.w, fontH, midLabels[i], uiColorHelp, m_scChSize, QM_LEFT, ETF_SHADOW );
	}

	for( int i = 0; i < 3; i++ )
	{
		int lx = m_scPos.x + m_labels[i].pos.x * uiStatic.scaleX;
		int ly = m_scPos.y + m_labels[i].pos.y * uiStatic.scaleY;
		int lw = m_labels[i].size.w * uiStatic.scaleX;
		int lh = m_labels[i].size.h * uiStatic.scaleY;

		UI_DrawString( font, lx, ly, lw, lh, m_labels[i].text, uiColorHelp, m_scChSize, QM_LEFT, ETF_SHADOW );
	}

	BaseClass::Draw();
}

void CMenuColorPickerDialog::CLogoPreview::Draw()
{
	if( !hImage )
	{
		UI_FillRect( m_scPos, m_scSize, uiPromptBgColor );
		UI_DrawString( font, m_scPos, m_scSize, L( "No logo" ), colorBase, m_scChSize, QM_CENTER, ETF_SHADOW );
	}
	else if( stripeCount <= 1 )
	{
		EngFuncs::PIC_Set( hImage, stripes[0][0], stripes[0][1], stripes[0][2] );
		EngFuncs::PIC_DrawTrans( m_scPos, m_scSize );
	}
	else
	{
		const Size img_sz = EngFuncs::PIC_Size( hImage );
		const bool hz = horizontal && *horizontal;

		const double tex_per_stripe = ( hz ? img_sz.w  : img_sz.h  ) / (double)stripeCount;
		const double scr_per_stripe = ( hz ? m_scSize.w : m_scSize.h ) / (double)stripeCount;

		for( int i = 0; i < stripeCount; i++ )
		{
			// derive each stripe's bounds from the next stripe's edge so consecutive stripes touch exactly
			int scr_start = (int)( i * scr_per_stripe + 0.5 );
			int scr_next  = (int)(( i + 1 ) * scr_per_stripe + 0.5 );
			int tex_start = (int)( i * tex_per_stripe + 0.5 );
			int tex_next  = (int)(( i + 1 ) * tex_per_stripe + 0.5 );

			wrect_t rc = {};
			if( hz )
			{
				rc.left   = tex_start;
				rc.right  = tex_next;
				rc.bottom = img_sz.h;
			}
			else
			{
				rc.right  = img_sz.w;
				rc.top    = tex_start;
				rc.bottom = tex_next;
			}

			Point ui_pt = hz ? Point( m_scPos.x + scr_start, m_scPos.y ) : Point( m_scPos.x, m_scPos.y + scr_start );
			Size  ui_sz = hz ? Size( scr_next - scr_start, m_scSize.h ) : Size( m_scSize.w, scr_next - scr_start );

			EngFuncs::PIC_Set( hImage, stripes[i][0], stripes[i][1], stripes[i][2] );
			EngFuncs::PIC_DrawTrans( ui_pt, ui_sz, &rc );
		}
	}
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}

void CMenuColorPickerDialog::UpdateLogoPreview()
{
	m_logoPreviewItem.stripes = m_stripes;
	m_logoPreviewItem.stripeCount = m_stripeCount;
}

void CMenuColorPickerDialog::UpdateFieldsFromPicker()
{
	if( m_updatingFields )
		return;
	m_updatingFields = true;

	if( m_modeRGB )
	{
		byte r, g, b;
		m_picker.GetRGB( r, g, b );
		m_fields[0].SetBuffer( CNumStr( r ));
		m_fields[1].SetBuffer( CNumStr( g ));
		m_fields[2].SetBuffer( CNumStr( b ));
	}
	else
	{
		m_fields[0].SetBuffer( CNumStr( (int)( m_picker.GetHue() + 0.5f )));
		m_fields[1].SetBuffer( CNumStr( (int)( m_picker.GetSat() * 100.f + 0.5f )));
		m_fields[2].SetBuffer( CNumStr( (int)( m_picker.GetVal() * 100.f + 0.5f )));
	}

	m_updatingFields = false;
}

void CMenuColorPickerDialog::UpdatePickerFromFields()
{
	if( m_updatingFields || m_picker.IsDragging() )
		return;
	m_updatingFields = true;

	if( m_modeRGB )
	{
		int r = Q_max( 0, Q_min( 255, atoi( m_fields[0].GetBuffer() )));
		int g = Q_max( 0, Q_min( 255, atoi( m_fields[1].GetBuffer() )));
		int b = Q_max( 0, Q_min( 255, atoi( m_fields[2].GetBuffer() )));
		m_picker.SetRGB( (byte)r, (byte)g, (byte)b );
	}
	else
	{
		float h = Q_max( 0.f, Q_min( 360.f, (float)atoi( m_fields[0].GetBuffer() )));
		float s = Q_max( 0.f, Q_min( 1.f, atoi( m_fields[1].GetBuffer() ) / 100.f ));
		float v = Q_max( 0.f, Q_min( 1.f, atoi( m_fields[2].GetBuffer() ) / 100.f ));
		m_picker.SetHSV( h, s, v );
	}

	m_updatingFields = false;

	OnPickerChanged();
}

void CMenuColorPickerDialog::SwitchMode( bool toRGB )
{
	m_modeRGB = toRGB;
	static const char *rgbLabels[3] = { "R", "G", "B" };
	static const char *hsvLabels[3] = { "H", "S", "V" };
	const char *const *labels       = toRGB ? rgbLabels : hsvLabels;
	for( int i = 0; i < 3; i++ )
		m_labels[i].text = labels[i];
	UpdateFieldsFromPicker();
}

void CMenuColorPickerDialog::OnPickerChanged()
{
	byte r, g, b;
	m_picker.GetRGB( r, g, b );

	m_stripes[m_selectedStripe][0] = r;
	m_stripes[m_selectedStripe][1] = g;
	m_stripes[m_selectedStripe][2] = b;

	UpdateLogoPreview();
	UpdateFieldsFromPicker();

	// defer preset detection until drag ends so the spinner doesn't thrash
	if( m_picker.IsDragging())
		m_detectPresetPending = true;
	else
		DetectAndSyncPreset();
}

void CMenuColorPickerDialog::Think()
{
	BaseClass::Think();

	if( m_detectPresetPending && !m_picker.IsDragging())
	{
		m_detectPresetPending = false;
		DetectAndSyncPreset();
	}
}

void CMenuColorPickerDialog::LoadSelectedStripeIntoPicker()
{
	m_picker.SetRGB( m_stripes[m_selectedStripe][0], m_stripes[m_selectedStripe][1], m_stripes[m_selectedStripe][2] );
	UpdateFieldsFromPicker();
}

void CMenuColorPickerDialog::OnStripeCountChanged()
{
	int newCount = (int)( m_stripeCountSpin.GetCurrentValue() + 0.5f ) + 1;

	if( newCount == m_stripeCount )
		return;

	for( int i = m_stripeCount; i < newCount; i++ )
	{
		m_stripes[i][0] = m_stripes[m_stripeCount - 1][0];
		m_stripes[i][1] = m_stripes[m_stripeCount - 1][1];
		m_stripes[i][2] = m_stripes[m_stripeCount - 1][2];
	}

	m_stripeCount = newCount;
	if( m_selectedStripe >= m_stripeCount )
		m_selectedStripe = m_stripeCount - 1;

	UpdateLogoPreview();
	LoadSelectedStripeIntoPicker();
	DetectAndSyncPreset();
}

void CMenuColorPickerDialog::OnPresetSelected()
{
	int idx = (int)( m_presetSpin.GetCurrentValue() + 0.5f );
	if( idx <= 0 || idx >= (int)V_ARRAYSIZE( g_LogoPresets ))
		return;

	const logo_preset_t &p = g_LogoPresets[idx];

	m_stripeCount = p.stripes;
	for( int i = 0; i < m_stripeCount; i++ )
	{
		m_stripes[i][0] = p.rgb[i * 3 + 0];
		m_stripes[i][1] = p.rgb[i * 3 + 1];
		m_stripes[i][2] = p.rgb[i * 3 + 2];
	}

	m_stripeCountSpin.SetCurrentValue( m_stripeCount - 1 );

	if( m_selectedStripe >= m_stripeCount )
		m_selectedStripe = 0;

	UpdateLogoPreview();
	LoadSelectedStripeIntoPicker();
}

void CMenuColorPickerDialog::DetectAndSyncPreset()
{
	int matchIdx = 0;
	for( size_t i = 1; i < V_ARRAYSIZE( g_LogoPresets ); i++ )
	{
		const logo_preset_t &p = g_LogoPresets[i];
		if( p.stripes != m_stripeCount )
			continue;

		bool match = true;
		for( int j = 0; j < m_stripeCount && match; j++ )
		{
			if( m_stripes[j][0] != p.rgb[j * 3 + 0]
			 || m_stripes[j][1] != p.rgb[j * 3 + 1]
			 || m_stripes[j][2] != p.rgb[j * 3 + 2] )
				match = false;
		}
		if( match )
		{
			matchIdx = (int)i;
			break;
		}
	}

	m_presetSpin.SetCurrentValue( matchIdx );
}

void CMenuColorPickerDialog::OnOrientationChanged()
{
	m_horizontal = m_orientationSpin.GetCurrentValue() >= 0.5f;
}

void CMenuColorPickerDialog::Show( const byte ( *stripes )[3], int stripeCount, bool horizontal, HIMAGE logoImage )
{
	m_stripeCount = Q_max( 1, Q_min( MAX_LOGO_STRIPES, stripeCount ));
	for( int i = 0; i < m_stripeCount; i++ )
	{
		m_stripes[i][0] = stripes[i][0];
		m_stripes[i][1] = stripes[i][1];
		m_stripes[i][2] = stripes[i][2];
	}
	m_selectedStripe = 0;
	m_horizontal = horizontal;

	m_logoPreviewItem.hImage      = logoImage;
	m_logoPreviewItem.stripes     = m_stripes;
	m_logoPreviewItem.stripeCount = m_stripeCount;

	m_stripeCountSpin.SetCurrentValue( m_stripeCount - 1 );
	m_orientationSpin.SetCurrentValue( m_horizontal ? 1.f : 0.f );

	m_modeRGB = true;
	m_modeSwitch.SetCurrentValue( 0.f );
	m_picker.SetRGB( m_stripes[0][0], m_stripes[0][1], m_stripes[0][2], false );
	SwitchMode( true );

	DetectAndSyncPreset();

	BaseClass::Show();
}
