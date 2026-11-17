/*
ColorPicker.h - color picker widget
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
#ifndef COLORPICKER_H
#define COLORPICKER_H

#include "BaseItem.h"
#include "ColorUtils.h"

class CMenuColorPicker : public CMenuBaseItem
{
public:
	CMenuColorPicker();
	virtual ~CMenuColorPicker();

	void VidInit() override;
	void Draw() override;
	bool KeyDown( int key ) override;
	bool KeyUp( int key ) override;
	bool MouseMove( int x, int y ) override;

	void GetRGB( byte &r, byte &g, byte &b ) const;
	void SetRGB( byte r, byte g, byte b, bool rebuild = true );

	float GetHue() const { return m_hue; }
	float GetSat() const { return m_sat; }
	float GetVal() const { return m_val; }
	bool  IsDragging() const { return m_draggingSV || m_draggingHue; }
	void  SetHSV( float h, float s, float v );

private:
	float m_hue;
	float m_sat;
	float m_val;

	Point m_svPos;
	Size  m_svSize;
	Point m_huePos;
	Size  m_hueSize;

	HIMAGE m_svTex;
	HIMAGE m_hueTex;
	HIMAGE m_cursorTex;

	bool m_draggingSV;
	bool m_draggingHue;

	void RebuildSVTexture();
	void BuildHueTexture();
	void BuildCursorTexture();
};

#endif // COLORPICKER_H
