/*
ColorUtils.h - color utilities
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
#ifndef COLORUTILS_H
#define COLORUTILS_H

#include <math.h>
#include "Utils.h"

namespace ColorUtils
{

inline void HSVtoRGB( float h, float s, float v, byte &r, byte &g, byte &b )
{
	float R, G, B;

	// Normalize hue to [0, 360)
	h = fmodf( h, 360.f );
	if( h < 0.f )
		h += 360.f;

	if( s <= 0.f )
	{
		R = G = B = v;
	}
	else
	{
		float hh = h / 60.f;
		int   i  = (int)hh % 6;
		float ff = hh - (int)hh;
		float p  = v * ( 1.f - s );
		float q  = v * ( 1.f - s * ff );
		float t  = v * ( 1.f - s * ( 1.f - ff ) );

		switch( i )
		{
		case 0: R = v; G = t; B = p; break;
		case 1: R = q; G = v; B = p; break;
		case 2: R = p; G = v; B = t; break;
		case 3: R = p; G = q; B = v; break;
		case 4: R = t; G = p; B = v; break;
		default:R = v; G = p; B = q; break;
		}
	}

	r = (byte)( R * 255.f + 0.5f );
	g = (byte)( G * 255.f + 0.5f );
	b = (byte)( B * 255.f + 0.5f );
}

inline void RGBtoHSV( byte r, byte g, byte b, float &h, float &s, float &v )
{
	float R = r / 255.f;
	float G = g / 255.f;
	float B = b / 255.f;

	float cmax  = Q_max( R, Q_max( G, B ) );
	float cmin  = Q_min( R, Q_min( G, B ) );
	float delta = cmax - cmin;

	v = cmax;
	s = ( cmax > 0.f ) ? ( delta / cmax ) : 0.f;

	if( delta <= 0.f )
	{
		h = 0.f;
	}
	else if( cmax == R )
	{
		h = 60.f * fmodf( ( G - B ) / delta, 6.f );
	}
	else if( cmax == G )
	{
		h = 60.f * ( ( B - R ) / delta + 2.f );
	}
	else
	{
		h = 60.f * ( ( R - G ) / delta + 4.f );
	}

	if( h < 0.f )
		h += 360.f;
}

} // namespace ColorUtils

#endif // COLORUTILS_H
