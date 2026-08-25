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
// pm_math.c -- math primitives
#include <math.h>
#include "mathlib.h"
#if HAVE_TGMATH_H
#include <tgmath.h>
#endif
#include "const.h"
#include "build.h"

#if XASH_ARM >= 8
#define XASH_SIMD_NEON
#include <arm_neon.h>
#include "neon_mathfun.h"
#endif // XASH_ARM >= 8


// up / down
#define	PITCH	0
// left / right
#define	YAW	1
// fall over
#define	ROLL	2 

#if _MSC_VER
#pragma warning(disable : 4244)
#endif

vec3_t vec3_origin = { 0, 0, 0 };
int nanmask = 255 << 23;

float anglemod( float a )
{
	a = ( 360.0f / 65536.0f ) * ( (int)( a * ( 65536.0f / 360.0f )) & 65535 );
	return a;
}

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * ( M_PI_F * 2.0f / 360.0f );
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[PITCH] * ( M_PI_F * 2.0f / 360.0f );
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[ROLL] * ( M_PI_F * 2.0f / 360.0f );
	sr = sin( angle );
	cr = cos( angle );

	if( forward )
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if( right )
	{
		right[0] = ( -1 * sr * sp * cy + -1 * cr * -sy );
		right[1] = ( -1 * sr * sp * sy + -1 * cr * cy );
		right[2] = -1 * sr * cp;
	}
	if( up )
	{
		up[0] = ( cr * sp * cy + -sr * -sy );
		up[1] = ( cr * sp * sy + -sr * cy );
		up[2] = cr * cp;
	}
}

void AngleVectorsTranspose( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;
	
	angle = angles[YAW] * ( M_PI_F * 2.0f / 360.0f );
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[PITCH] * ( M_PI_F * 2.0f / 360.0f );
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[ROLL] * ( M_PI_F * 2.0f / 360.0f );
	sr = sin( angle );
	cr = cos( angle );

	if( forward )
	{
		forward[0] = cp * cy;
		forward[1] = ( sr * sp * cy + cr * -sy );
		forward[2] = ( cr * sp * cy + -sr * -sy );
	}
	if( right )
	{
		right[0] = cp * sy;
		right[1] = ( sr * sp * sy + cr * cy );
		right[2] = ( cr * sp * sy + -sr * cy );
	}
	if( up )
	{
		up[0] = -sp;
		up[1] = sr * cp;
		up[2] = cr * cp;
	}
}

void AngleMatrix( const float *angles, float (*matrix)[4] )
{
#if XASH_SIMD_NEON
	static const uint32x4_t AngleMatrix_sign0 = vsetq_lane_u32( 0x80000000, vdupq_n_u32( 0 ), 0 );
	static const uint32x4_t AngleMatrix_sign1 = vsetq_lane_u32( 0x80000000, vdupq_n_u32( 0 ), 1 );
	static const uint32x4_t AngleMatrix_sign2 = vsetq_lane_u32( 0x80000000, vdupq_n_u32( 0 ), 2 );
	float32x4x3_t out_reg;
	float32x4_t angles_reg = {};
	memcpy( &angles_reg, angles, sizeof( float ) * 3 );

	float32x4x2_t sp_sy_sr_0_cp_cy_cr_1;
	sincos_ps( vmulq_n_f32( angles_reg, ( M_PI * 2 / 360 )), &sp_sy_sr_0_cp_cy_cr_1.val[0], &sp_sy_sr_0_cp_cy_cr_1.val[1] );

	float32x4x2_t sp_sr_cp_cr_sy_0_cy_1 = vuzpq_f32( sp_sy_sr_0_cp_cy_cr_1.val[0], sp_sy_sr_0_cp_cy_cr_1.val[1] );
	float32x4x2_t sp_cp_sy_cy_sr_cr_0_1 = vzipq_f32( sp_sy_sr_0_cp_cy_cr_1.val[0], sp_sy_sr_0_cp_cy_cr_1.val[1] );

	float32x4_t _0_sr_cr_0 = vextq_f32( sp_sy_sr_0_cp_cy_cr_1.val[0], sp_cp_sy_cy_sr_cr_0_1.val[1], 3 );
	float32x4_t cp_cr_sr_0 = vcombine_f32( vget_high_f32( sp_sr_cp_cr_sy_0_cy_1.val[0] ), vget_high_f32( sp_sy_sr_0_cp_cy_cr_1.val[0] ));
	float32x4_t cy_sy_sy_0 = vcombine_f32( vrev64_f32( vget_high_f32( sp_cp_sy_cy_sr_cr_0_1.val[0] )), vget_low_f32( sp_sr_cp_cr_sy_0_cy_1.val[1] ));
	float32x4_t sy_cy_cy_1 = vcombine_f32( vget_high_f32( sp_cp_sy_cy_sr_cr_0_1.val[0] ), vget_high_f32( sp_sr_cp_cr_sy_0_cy_1.val[1] ));

	float32x4_t _0_srsp_crsp_0 = vmulq_laneq_f32( _0_sr_cr_0, sp_sy_sr_0_cp_cy_cr_1.val[0], 0 ); // *sp
	out_reg.val[0] = vmulq_laneq_f32( _0_srsp_crsp_0, sp_sy_sr_0_cp_cy_cr_1.val[1], 1 ); // *cy
	out_reg.val[1] = vmulq_laneq_f32( _0_srsp_crsp_0, sp_sy_sr_0_cp_cy_cr_1.val[0], 1 ); // *sy

	cy_sy_sy_0 = vreinterpretq_f32_u32( veorq_u32( vreinterpretq_u32_f32( cy_sy_sy_0 ), AngleMatrix_sign1 ));
	sy_cy_cy_1 = vreinterpretq_f32_u32( veorq_u32( vreinterpretq_u32_f32( sy_cy_cy_1 ), AngleMatrix_sign2 ));
	out_reg.val[0] = vfmaq_f32( out_reg.val[0], cp_cr_sr_0, cy_sy_sy_0 );
	out_reg.val[1] = vfmaq_f32( out_reg.val[1], cp_cr_sr_0, sy_cy_cy_1 );

	float32x4_t cp_cr_0_1 = vcombine_f32( vget_high_f32( sp_sr_cp_cr_sy_0_cy_1.val[0] ), vget_high_f32( sp_cp_sy_cy_sr_cr_0_1.val[1] ));
	float32x4_t _1_cp_cr_0 = vextq_f32( cp_cr_0_1, cp_cr_0_1, 3 );
	out_reg.val[2] = vmulq_f32( sp_sr_cp_cr_sy_0_cy_1.val[0], _1_cp_cr_0 );
	out_reg.val[2] = vreinterpretq_f32_u32( veorq_u32( vreinterpretq_u32_f32( out_reg.val[2] ), AngleMatrix_sign0 ));

	memcpy( matrix, &out_reg, sizeof( float ) * 3 * 4 );
#else
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * ( M_PI_F * 2.0f / 360.0f );
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[PITCH] * ( M_PI_F * 2.0f / 360.0f );
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[ROLL] * ( M_PI_F * 2.0f / 360.0f );
	sr = sin( angle );
	cr = cos( angle );

	// matrix = (YAW * PITCH) * ROLL
	matrix[0][0] = cp * cy;
	matrix[1][0] = cp * sy;
	matrix[2][0] = -sp;
	matrix[0][1] = sr * sp * cy + cr * -sy;
	matrix[1][1] = sr * sp * sy + cr * cy;
	matrix[2][1] = sr * cp;
	matrix[0][2] = (cr * sp * cy + -sr * -sy);
	matrix[1][2] = (cr * sp * sy + -sr* cy);
	matrix[2][2] = cr * cp;
	matrix[0][3] = 0.0f;
	matrix[1][3] = 0.0f;
	matrix[2][3] = 0.0f;
#endif
}

void AngleIMatrix( const vec3_t angles, float matrix[3][4] )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	angle = angles[YAW] * ( M_PI_F * 2.0f / 360.0f );
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[PITCH] * ( M_PI_F * 2.0f / 360.0f );
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[ROLL] * ( M_PI_F * 2.0f / 360.0f );
	sr = sin( angle );
	cr = cos( angle );

	// matrix = ( YAW * PITCH ) * ROLL
	matrix[0][0] = cp * cy;
	matrix[0][1] = cp * sy;
	matrix[0][2] = -sp;
	matrix[1][0] = sr * sp * cy + cr * -sy;
	matrix[1][1] = sr * sp * sy + cr * cy;
	matrix[1][2] = sr * cp;
	matrix[2][0] = ( cr * sp * cy + -sr * -sy );
	matrix[2][1] = ( cr * sp * sy + -sr * cy );
	matrix[2][2] = cr * cp;
	matrix[0][3] = 0.0;
	matrix[1][3] = 0.0;
	matrix[2][3] = 0.0;
}

void NormalizeAngles( float *angles )
{
	int i;
	// Normalize angles
	for( i = 0; i < 3; i++ )
	{
		if( angles[i] > 180.0f )
		{
			angles[i] -= 360.0f;
		}
		else if( angles[i] < -180.0f )
		{
			angles[i] += 360.0f;
		}
	}
}

/*
===================
InterpolateAngles

Interpolate Euler angles.
FIXME:  Use Quaternions to avoid discontinuities
Frac is 0.0 to 1.0 ( i.e., should probably be clamped, but doesn't have to be )
===================
*/
void InterpolateAngles( float *start, float *end, float *output, float frac )
{
	int i;
	float ang1, ang2;
	float d;

	NormalizeAngles( start );
	NormalizeAngles( end );

	for( i = 0; i < 3; i++ )
	{
		ang1 = start[i];
		ang2 = end[i];

		d = ang2 - ang1;
		if( d > 180.0f )
		{
			d -= 360.0f;
		}
		else if( d < -180.0f )
		{	
			d += 360.0f;
		}

		output[i] = ang1 + d * frac;
	}

	NormalizeAngles( output );
}

/*
===================
AngleBetweenVectors

===================
*/
float AngleBetweenVectors( const vec3_t v1, const vec3_t v2 )
{
	float angle;
	float l1 = Length( v1 );
	float l2 = Length( v2 );

	if( !l1 || !l2 )
		return 0.0f;

	angle = acos( DotProduct( v1, v2 ) / ( l1 * l2 ));
	angle = ( angle  * 180.0f ) / M_PI_F;

	return angle;
}

void VectorTransform( const vec3_t in1, float in2[3][4], vec3_t out )
{
#if XASH_SIMD_NEON
	float32x4_t in1_reg = {};
	memcpy( &in1_reg, in1, sizeof( float ) * 3 );

	float32x4x4_t in_t;
	memcpy( &in_t, in2, sizeof( float ) * 3 * 4 );
	//memset( &in_t.val[3], 0, sizeof( in_t.val[3] ) );
	in_t = vld4q_f32((const float*)&in_t );

	float32x4_t out_reg = in_t.val[3];
	out_reg = vfmaq_laneq_f32( out_reg, in_t.val[0], in1_reg, 0 );
	out_reg = vfmaq_laneq_f32( out_reg, in_t.val[1], in1_reg, 1 );
	out_reg = vfmaq_laneq_f32( out_reg, in_t.val[2], in1_reg, 2 );

	memcpy( out, &out_reg, sizeof( float ) * 3 );
#else
	out[0] = DotProduct( in1, in2[0] ) + in2[0][3];
	out[1] = DotProduct( in1, in2[1] ) + in2[1][3];
	out[2] = DotProduct( in1, in2[2] ) + in2[2][3];
#endif
}

int VectorCompare( const vec3_t v1, const vec3_t v2 )
{
#if XASH_SIMD_NEON
	// is this really works?
	float32x4_t v1_reg = {}, v2_reg = {};
	memcpy( &v1_reg, v1, sizeof( float ) * 3 );
	memcpy( &v2_reg, v2, sizeof( float ) * 3 );
	return !vaddvq_u32( vceqq_f32( v1_reg, v2_reg ));
#else
	int i;

	for( i = 0; i < 3; i++ )
		if( v1[i] != v2[i] )
			return 0;

	return 1;
#endif
}

void VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc )
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

vec_t _DotProduct( vec3_t v1, vec3_t v2 )
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void _VectorSubtract( vec3_t veca, vec3_t vecb, vec3_t out )
{
	out[0] = veca[0] - vecb[0];
	out[1] = veca[1] - vecb[1];
	out[2] = veca[2] - vecb[2];
}

void _VectorAdd( vec3_t veca, vec3_t vecb, vec3_t out )
{
	out[0] = veca[0] + vecb[0];
	out[1] = veca[1] + vecb[1];
	out[2] = veca[2] + vecb[2];
}

void _VectorCopy( vec3_t in, vec3_t out )
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
#if XASH_SIMD_NEON
	float32x4_t v1_reg = {}, v2_reg = {};
	memcpy( &v1_reg, v1, sizeof( float ) * 3 );
	memcpy( &v2_reg, v2, sizeof( float ) * 3 );

	float32x2_t xy_a = vget_low_f32( v1_reg );
	float32x2_t xy_b = vget_low_f32( v2_reg );
	float32x4_t yzxy_a = vcombine_f32( vext_f32( xy_a, vget_high_f32( v1_reg ), 1 ), xy_a ); // [aj, ak, ai, aj]
	float32x4_t yzxy_b = vcombine_f32( vext_f32( xy_b, vget_high_f32( v2_reg ), 1 ), xy_b ); // [bj, bk, bi, bj]
	float32x4_t zxyy_a = vextq_f32( yzxy_a, yzxy_a, 1 ); // [ak, ai, aj, aj]
	float32x4_t zxyy_b = vextq_f32( yzxy_b, yzxy_b, 1 ); // [bk, ai, bj, bj]
	float32x4_t cross_reg = vfmsq_f32( vmulq_f32( yzxy_a, zxyy_b ), zxyy_a, yzxy_b ); // [ajbk-akbj, akbi-aibk, aibj-ajbi, 0]

	memcpy( cross, &cross_reg, sizeof( float ) * 3 );
#else
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
#endif
}

float Length( const vec3_t v )
{
	int i;
	float length = 0.0f;

	for( i = 0; i < 3; i++ )
		length += v[i] * v[i];
	length = sqrt( length );		// FIXME

	return length;
}

float Distance( const vec3_t v1, const vec3_t v2 )
{
	vec3_t d;
	VectorSubtract( v2, v1, d );
	return Length( d );
}

float VectorNormalize( vec3_t v )
{
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt( length );		// FIXME

	if( length )
	{
		ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

void VectorInverse( vec3_t v )
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale( const vec3_t in, vec_t scale, vec3_t out )
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

int Q_log2( int val )
{
	int answer = 0;
	while( val >>= 1 )
		answer++;
	return answer;
}

void VectorMatrix( vec3_t forward, vec3_t right, vec3_t up )
{
	vec3_t tmp;

	if( forward[0] == 0.0f && forward[1] == 0.0f )
	{
		right[0] = 1.0f;
		right[1] = 0.0f;
		right[2] = 0.0f;
		up[0] = -forward[2]; 
		up[1] = 0.0f;
		up[2] = 0.0f;
		return;
	}

	tmp[0] = 0.0f; tmp[1] = 0.0f; tmp[2] = 1.0f;
	CrossProduct( forward, tmp, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
	VectorNormalize( up );
}

void VectorAngles( const vec3_t forward, vec3_t angles )
{
	float tmp, yaw, pitch;

	if( forward[1] == 0.0f && forward[0] == 0.0f )
	{
		yaw = 0.0f;
		if( forward[2] > 0.0f )
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else
	{
		yaw = ( atan2( forward[1], forward[0] ) * 180.0f / M_PI_F );
		if( yaw < 0.0f )
			yaw += 360.0f;

		tmp = sqrt( forward[0] * forward[0] + forward[1] * forward[1] );
		pitch = ( atan2( forward[2], tmp ) * 180.0f / M_PI_F );
		if( pitch < 0.0f )
			pitch += 360.0f;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0.0f;
}
