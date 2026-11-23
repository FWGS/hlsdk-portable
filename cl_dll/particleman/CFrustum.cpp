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

//The triangle API uses OpenGL constants for certain functions.
#include "windows_lean.h"

#ifndef GL_PROJECTION_MATRIX
#define GL_PROJECTION_MATRIX 0x0BA1
#endif
#ifndef GL_MODELVIEW_MATRIX
#define GL_MODELVIEW_MATRIX 0x0BA6
#endif

#include "wrect.h"
#include "cl_dll.h"
#include "cl_util.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

#include "triangleapi.h"
#include "CFrustum.h"

//TODO: this function always operates on the frustum matrix that's part of this object, so there is no need to pass the address.
void CFrustum::NormalizeFrustumPlane(float frustum[6][4], int side)
{
	auto& elements = frustum[side];

	const float length = Vector{elements[0], elements[1], elements[2]}.Length();

	elements[0] /= length;
	elements[1] /= length;
	elements[2] /= length;
	elements[3] /= length;
}

void CFrustum::CalculateFrustum()
{
	float proj[16];
	gEngfuncs.pTriAPI->GetMatrix(GL_PROJECTION_MATRIX, proj);

	float modl[16];
	gEngfuncs.pTriAPI->GetMatrix(GL_MODELVIEW_MATRIX, modl);

	// clip = proj * modelview; both are OpenGL column-major, element (row r, column c) is [c * 4 + r]
	float clip[16];
	for (int c = 0; c < 4; ++c)
	{
		for (int r = 0; r < 4; ++r)
		{
			float sum = 0;
			for (int k = 0; k < 4; ++k)
				sum += proj[k * 4 + r] * modl[c * 4 + k];
			clip[c * 4 + r] = sum;
		}
	}

	// the planes are the 4th row of clip plus or minus the other rows
	const auto setPlane = [&](int side, int row, float sign)
	{
		for (int i = 0; i < 4; ++i)
			g_flFrustum[side][i] = clip[i * 4 + 3] + sign * clip[i * 4 + row];
		NormalizeFrustumPlane(g_flFrustum, side);
	};

	setPlane(RIGHT, 0, -1.0f);
	setPlane(LEFT, 0, 1.0f);
	setPlane(BOTTOM, 1, 1.0f);
	setPlane(TOP, 1, -1.0f);
	setPlane(BACK, 2, -1.0f);
	setPlane(FRONT, 2, 1.0f);
}

bool CFrustum::PointInsideFrustum(float x, float y, float z)
{
	for (int i = 0; i < 6; ++i)
	{
		if (((g_flFrustum[i][0] * x) + (g_flFrustum[i][1] * y) + (g_flFrustum[i][2] * z) + g_flFrustum[i][3]) <= 0)
		{
			return false;
		}
	}

	return true;
}

bool CFrustum::SphereInsideFrustum(float x, float y, float z, float radius)
{
	for (int i = 0; i < 6; ++i)
	{
		if (((g_flFrustum[i][0] * x) + (g_flFrustum[i][1] * y) + (g_flFrustum[i][2] * z) + g_flFrustum[i][3]) <= -radius)
		{
			return false;
		}
	}

	return true;
}

bool CFrustum::PlaneInsideFrustum(float x, float y, float z, float size)
{
	const float sizes[3][2] =
		{
			{x - size, x + size},
			{y - size, y + size},
			{z - size, z + size}};

	for (int i = 0; i < 6; ++i)
	{
		//Test every permutation of plane sizes.
		for (int zAxis = 0; zAxis < 2; ++zAxis)
		{
			for (int yAxis = 0; yAxis < 2; ++yAxis)
			{
				for (int xAxis = 0; xAxis < 2; ++xAxis)
				{
					if (((sizes[0][xAxis] * g_flFrustum[i][0]) + (sizes[1][yAxis] * g_flFrustum[i][1]) + (sizes[2][zAxis] * g_flFrustum[i][2]) + g_flFrustum[i][3]) <= 0)
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}
