//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio_util.h"
#include "build.h"
#if XASH_ARM >= 8
#define XASH_SIMD_NEON 1
#include <arm_neon.h>
#include "neon_mathfun.h"
#endif // XASH_ARM

#if XASH_SIMD_NEON
#if XASH_ARM >= 8
#define NEON_MLA( acc, a, b )  vfmaq_f32( acc, a, b )
#define NEON_MLS( acc, a, b )  vfmsq_f32( acc, a, b )
#elif __ARM_VFPV4__
#define NEON_MLA( acc, a, b )  vfmaq_f32( acc, a, b )
#define NEON_MLS( acc, a, b )  vfmsq_f32( acc, a, b )
#else // XASH_ARM
#define NEON_MLA( acc, a, b )  vmlaq_f32( acc, a, b )
#define NEON_MLS( acc, a, b )  vmlsq_f32( acc, a, b )
#endif // XASH_ARM
#endif // XASH_SIMD_NEON

/*
================
ConcatTransforms

================
*/
void ConcatTransforms( float in1[3][4], float in2[3][4], float out[3][4] )
{
#if XASH_SIMD_NEON
	float32x4_t in2_row0 = vld1q_f32( in2[0] );
	float32x4_t in2_row1 = vld1q_f32( in2[1] );
	float32x4_t in2_row2 = vld1q_f32( in2[2] );

	for( int i = 0; i < 3; i++ )
	{
		float32x4_t in1_row = vld1q_f32( in1[i] );

#if XASH_ARM >= 8
		float32x4_t out_row = vcopyq_laneq_f32( vdupq_n_f32( 0.0f ), 3, in1_row, 3 );
		out_row = vfmaq_laneq_f32( out_row, in2_row0, in1_row, 0 );
		out_row = vfmaq_laneq_f32( out_row, in2_row1, in1_row, 1 );
		out_row = vfmaq_laneq_f32( out_row, in2_row2, in1_row, 2 );
#else // XASH_ARM
		float32x2_t lo = vget_low_f32( in1_row );
		float32x2_t hi = vget_high_f32( in1_row );
		float32x2x2_t z = vzip_f32( vdup_n_f32( 0.0f ), hi );
		float32x4_t out_row = vcombine_f32( vdup_n_f32( 0.0f ), z.val[1] );
#if __ARM_VFPV4__
		out_row = vfmaq_lane_f32( out_row, in2_row0, lo, 0 );
		out_row = vfmaq_lane_f32( out_row, in2_row1, lo, 1 );
		out_row = vfmaq_lane_f32( out_row, in2_row2, hi, 0 );
#else // __ARM_VFPV4__
		out_row = vmlaq_lane_f32( out_row, in2_row0, lo, 0 );
		out_row = vmlaq_lane_f32( out_row, in2_row1, lo, 1 );
		out_row = vmlaq_lane_f32( out_row, in2_row2, hi, 0 );
#endif // __ARM_VFPV4__
#endif // XASH_ARM
		vst1q_f32( out[i], out_row );
	}
#else // XASH_SIMD_NEON
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[0][3] = in1[0][0] * in2[0][3] + in1[0][1] * in2[1][3] +
				in1[0][2] * in2[2][3] + in1[0][3];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[1][3] = in1[1][0] * in2[0][3] + in1[1][1] * in2[1][3] +
				in1[1][2] * in2[2][3] + in1[1][3];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
	out[2][3] = in1[2][0] * in2[0][3] + in1[2][1] * in2[1][3] +
				in1[2][2] * in2[2][3] + in1[2][3];
#endif
}

// angles index are not the same as ROLL, PITCH, YAW

/*
====================
AngleQuaternion

====================
*/
void AngleQuaternion( float *angles, vec4_t quaternion )
{
	float angle;
	float sr, sp, sy, cr, cp, cy;

	// FIXME: rescale the inputs to 1/2 angle
	angle = angles[2] * 0.5f;
	sy = sin( angle );
	cy = cos( angle );
	angle = angles[1] * 0.5f;
	sp = sin( angle );
	cp = cos( angle );
	angle = angles[0] * 0.5f;
	sr = sin( angle );
	cr = cos( angle );

	quaternion[0] = sr * cp * cy - cr * sp * sy; // X
	quaternion[1] = cr * sp * cy + sr * cp * sy; // Y
	quaternion[2] = cr * cp * sy - sr * sp * cy; // Z
	quaternion[3] = cr * cp * cy + sr * sp * sy; // W
}

#if XASH_SIMD_NEON
static inline float32x4_t dot_dup_fast( float32x4_t a, float32x4_t b )
{
	float32x4_t m = vmulq_f32( a, b );
#if XASH_ARM >= 8
	m = vpaddq_f32( m, m );
	m = vpaddq_f32( m, m );
	return m;
#else
	float32x2_t lo = vget_low_f32( m );
	float32x2_t hi = vget_high_f32( m );
	float32x2_t sum2 = vadd_f32( lo, hi );
	float32x2_t final = vpadd_f32( sum2, sum2 );
	return vcombine_f32( final, final );
#endif
}
#endif

/*
====================
QuaternionSlerp

====================
*/
void QuaternionSlerp( vec4_t p, vec4_t q, float t, vec4_t qt )
{
#if XASH_SIMD_NEON
	float32x4_t sclp, sclq;
	float32x4_t p_reg = vld1q_f32( p );
	float32x4_t q_reg = vld1q_f32( q );
	float32x4_t cosom = dot_dup_fast( p_reg, q_reg );
	const uint32x4_t signmask = vdupq_n_u32( 0x80000000 );
	uint32x4_t sign = vandq_u32( vreinterpretq_u32_f32( cosom ), signmask );
	q_reg = vreinterpretq_f32_u32( veorq_u32( vreinterpretq_u32_f32( q_reg ), sign ));
	cosom = vabsq_f32( cosom );
	float cosom_s = vgetq_lane_f32( cosom, 0 );
	if( cosom_s >= 0.999999f )
	{
		float32x4_t t_reg = vdupq_n_f32( t );
		float32x4_t one_minus_t = vsubq_f32( vdupq_n_f32( 1.0f ), t_reg );
		vst1q_f32( qt, NEON_MLA( vmulq_f32( p_reg, one_minus_t ), q_reg, t_reg ));
		return;
	}
	if( cosom_s <= -0.999999f )
	{
		float32x4_t half_pi = vdupq_n_f32( 0.5f * M_PI_F );
		float32x4_t qt_reg = vsetq_lane_f32( vgetq_lane_f32( q_reg, 2 ), vdupq_n_f32( 0.0f ), 3 );
		qt_reg = vsetq_lane_f32( -vgetq_lane_f32( q_reg, 1 ), qt_reg, 0 );
		qt_reg = vsetq_lane_f32(  vgetq_lane_f32( q_reg, 0 ), qt_reg, 1 );
		qt_reg = vsetq_lane_f32( -vgetq_lane_f32( q_reg, 3 ), qt_reg, 2 );
		sclp = sin_ps( vmulq_n_f32( half_pi, 1.0f - t ));
		sclq = sin_ps( vmulq_n_f32( half_pi, t ));
		vst1q_f32( qt, NEON_MLA( vmulq_f32( p_reg, sclp ), qt_reg, sclq ));
		return;
	}
	float32x4_t omega = acos_ps( cosom );
	float32x4_t st, ct;
	sincos_ps( vmulq_n_f32( omega, t ), &st, &ct );
	float32x4_t one = vdupq_n_f32( 1.0f );
	float32x4_t cprod = vmulq_f32( vsubq_f32( one, cosom ), vaddq_f32( one, cosom ));
	const float32x4_t half = vdupq_n_f32( 0.5f );
	const float32x4_t onehalf = vdupq_n_f32( 1.5f );
	float32x4_t r = vrsqrteq_f32( cprod );
	r = vmulq_f32( r, NEON_MLS( onehalf, vmulq_f32( half, cprod ), vmulq_f32( r, r )));
	r = vmulq_f32( r, NEON_MLS( onehalf, vmulq_f32( half, cprod ), vmulq_f32( r, r )));
	float32x4_t sinom_recip = r;
	sclq = vmulq_f32( st, sinom_recip );
	sclp = NEON_MLS( ct, cosom, sclq );   // ct - cosom*sclq
	vst1q_f32( qt, NEON_MLA( vmulq_f32( p_reg, sclp ), q_reg, sclq ));
#else // XASH_SIMD_NEON
	int i;
	float omega, cosom, sinom, sclp, sclq;

	// decide if one of the quaternions is backwards
	float a = 0;
	float b = 0;

	for( i = 0; i < 4; i++ )
	{
		a += ( p[i] - q[i] ) * ( p[i] - q[i] );
		b += ( p[i] + q[i] ) * ( p[i] + q[i] );
	}
	if(a > b)
	{
		for( i = 0; i < 4; i++ )
		{
			q[i] = -q[i];
		}
	}

	cosom = p[0] * q[0] + p[1] * q[1] + p[2] * q[2] + p[3] * q[3];

	if( ( 1.0f + cosom ) > 0.000001f )
	{
		if( ( 1.0f - cosom ) > 0.000001f )
		{
			omega = acos( cosom );
			sinom = sin( omega );
			sclp = sin( ( 1.0f - t ) * omega ) / sinom;
			sclq = sin( t * omega ) / sinom;
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}
		for( i = 0; i < 4; i++ )
		{
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -q[1];
		qt[1] = q[0];
		qt[2] = -q[3];
		qt[3] = q[2];
		sclp = sin( ( 1.0f - t ) * ( 0.5f * M_PI_F ) );
		sclq = sin( t * ( 0.5f * M_PI_F ) );
		for( i = 0; i < 3; i++ )
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
#endif
}

/*
====================
QuaternionSlerpX4

====================
*/
void QuaternionSlerpX4( vec4_t p[4], vec4_t q[4], float t, vec4_t qt[4] )
{
#if XASH_SIMD_NEON
	float32x4x4_t p_reg = vld4q_f32( (const float *)p );
	float32x4x4_t q_reg = vld4q_f32( (const float *)q );
	const float32x4_t one = vdupq_n_f32( 1.0f );
	const float32x4_t one_minus_eps = vdupq_n_f32( 1.0f - 0.000001f );
	float32x4_t dot = vmulq_f32( p_reg.val[0], q_reg.val[0] );
	dot = NEON_MLA( dot, p_reg.val[1], q_reg.val[1] );
	dot = NEON_MLA( dot, p_reg.val[2], q_reg.val[2] );
	dot = NEON_MLA( dot, p_reg.val[3], q_reg.val[3] );
	const uint32x4_t signmask = vdupq_n_u32( 0x80000000 );
	uint32x4_t sign = vandq_u32( vreinterpretq_u32_f32( dot ), signmask );
	float32x4_t cosom = vabsq_f32( dot );
	uint32x4_t use_slerp = vcltq_f32( cosom, one_minus_eps );
	float32x4_t omega = acos_ps( cosom );
	float32x4_t st, ct;
	sincos_ps( vmulq_n_f32( omega, t ), &st, &ct );
	float32x4_t cprod = vmulq_f32( vsubq_f32( one, cosom ), vaddq_f32( one, cosom ));
	const float32x4_t half = vdupq_n_f32( 0.5f );
	const float32x4_t onehalf = vdupq_n_f32( 1.5f );
	float32x4_t r = vrsqrteq_f32( cprod );
	r = vmulq_f32( r, NEON_MLS( onehalf, vmulq_f32( half, cprod ), vmulq_f32( r, r )));
	r = vmulq_f32( r, NEON_MLS( onehalf, vmulq_f32( half, cprod ), vmulq_f32( r, r )));
	float32x4_t sinom_recip = r;
	float32x4_t sclq_s = vmulq_f32( st, sinom_recip );
	float32x4_t sclp_s = NEON_MLS( ct, cosom, sclq_s );
	float32x4_t t_reg = vdupq_n_f32( t );
	float32x4_t sclq = vbslq_f32( use_slerp, sclq_s, t_reg );
	float32x4_t sclp = vbslq_f32( use_slerp, sclp_s, vsubq_f32( one, t_reg ));
	sclq = vreinterpretq_f32_u32( veorq_u32( vreinterpretq_u32_f32( sclq ), sign ));
	float32x4x4_t qt_reg;
	for( int i = 0; i < 4; i++ )
		qt_reg.val[i] = NEON_MLA( vmulq_f32( p_reg.val[i], sclp ), q_reg.val[i], sclq );
	vst4q_f32( (float *)qt, qt_reg );
#else // XASH_SIMD_NEON
	QuaternionSlerp(p[0], q[0], t, qt[0]);
	QuaternionSlerp(p[1], q[1], t, qt[1]);
	QuaternionSlerp(p[2], q[2], t, qt[2]);
	QuaternionSlerp(p[3], q[3], t, qt[3]);
#endif
}

/*
====================
QuaternionMatrix

====================
*/
void QuaternionMatrix( vec4_t quaternion, float (*matrix)[4] )
{
	matrix[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
}

/*
====================
MatrixCopy

====================
*/
void MatrixCopy( float in[3][4], float out[3][4] )
{
	memcpy( out, in, sizeof( float ) * 3 * 4 );
}
