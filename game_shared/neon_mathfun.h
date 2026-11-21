/* NEON implementation of sin, cos, exp and log

   Inspired by Intel Approximate Math library, and based on the
   corresponding algorithms of the cephes math library
*/

/* Copyright (C) 2011  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)
*/

#include <arm_neon.h>
#if defined( __aarch64__ ) || defined( _M_ARM64 )
#define NEON_MLA( acc, a, b )  vfmaq_f32( acc, a, b )
#define NEON_MLS( acc, a, b )  vfmsq_f32( acc, a, b )
#define NEON_MLA_N( acc, b, c )  vfmaq_n_f32( acc, b, c )
#define NEON_MLS_N( acc, b, c )  vfmsq_n_f32( acc, b, c )
#elif __ARM_VFPV4__
#define NEON_MLA( acc, a, b )  vfmaq_f32( acc, a, b )
#define NEON_MLS( acc, a, b )  vfmsq_f32( acc, a, b )
#define NEON_MLA_N( acc, b, c )  vfmaq_f32( acc, b, vdupq_n_f32( c ))
#define NEON_MLS_N( acc, b, c )  vfmsq_f32( acc, b, vdupq_n_f32( c ))
#else
#define NEON_MLA( acc, a, b )  vmlaq_f32( acc, a, b )
#define NEON_MLS( acc, a, b )  vmlsq_f32( acc, a, b )
#define NEON_MLA_N( acc, b, c )  vmlaq_f32( acc, b, vdupq_n_f32( c ))
#define NEON_MLS_N( acc, b, c )  vmlsq_f32( acc, b, vdupq_n_f32( c ))
#endif

typedef float32x4_t v4sf;  // vector of 4 float
typedef uint32x4_t v4su;  // vector of 4 uint32
typedef int32x4_t v4si;  // vector of 4 uint32

#define s4f_x(s4f) vgetq_lane_f32(s4f, 0)
#define s4f_y(s4f) vgetq_lane_f32(s4f, 1)
#define s4f_z(s4f) vgetq_lane_f32(s4f, 2)
#define s4f_w(s4f) vgetq_lane_f32(s4f, 3)

#define c_inv_mant_mask ~0x7f800000u
#define c_cephes_SQRTHF 0.707106781186547524
#define c_cephes_log_p0 7.0376836292E-2
#define c_cephes_log_p1 - 1.1514610310E-1
#define c_cephes_log_p2 1.1676998740E-1
#define c_cephes_log_p3 - 1.2420140846E-1
#define c_cephes_log_p4 + 1.4249322787E-1
#define c_cephes_log_p5 - 1.6668057665E-1
#define c_cephes_log_p6 + 2.0000714765E-1
#define c_cephes_log_p7 - 2.4999993993E-1
#define c_cephes_log_p8 + 3.3333331174E-1
#define c_cephes_log_q1 -2.12194440e-4
#define c_cephes_log_q2 0.693359375

/* natural logarithm computed for 4 simultaneous float 
   return NaN for x <= 0
*/
static inline v4sf log_ps(v4sf x)
{
  v4sf one = vdupq_n_f32(1.0f);

  x = vmaxq_f32(x, vdupq_n_f32(0.0f)); /* force flush to zero on denormal values */
  v4su invalid_mask = vcleq_f32(x, vdupq_n_f32(0.0f));

  v4si emm0 = vsubq_s32(vshrq_n_s32(vreinterpretq_s32_f32(x), 23), vdupq_n_s32(0x7e));

  /* keep only the fractional part */
  v4si ux = vandq_s32(vreinterpretq_s32_f32(x), vdupq_n_s32(c_inv_mant_mask));
  ux = vorrq_s32(ux, vreinterpretq_s32_f32(vdupq_n_f32(0.5f)));
  x = vreinterpretq_f32_s32(ux);

  /* part2:
    if( x < SQRTHF ) {
    e -= 1;
    x = x + x - 1.0;
    } else { x = x - 1.0; }
  */
  v4su mask = vcltq_f32(x, vdupq_n_f32(c_cephes_SQRTHF));
  v4sf tmp = vreinterpretq_f32_u32(vandq_u32(vreinterpretq_u32_f32(x), mask));
  x = vaddq_f32(vsubq_f32(x, one), tmp);

  emm0 = vaddq_s32(emm0, vreinterpretq_s32_u32(mask));
  v4sf e = vcvtq_f32_s32(emm0);

  v4sf z = vmulq_f32(x, x);

  v4sf y = vdupq_n_f32(c_cephes_log_p0);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p1), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p2), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p3), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p4), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p5), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p6), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p7), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_log_p8), y, x);
  y = vmulq_f32(y, z);

  y = NEON_MLA(y, e, vdupq_n_f32(c_cephes_log_q1));
  y = NEON_MLS(y, z, vdupq_n_f32(0.5f));

  x = vaddq_f32(x, y);
  x = NEON_MLA(x, e, vdupq_n_f32(c_cephes_log_q2));
  x = vreinterpretq_f32_u32(vorrq_u32(vreinterpretq_u32_f32(x), invalid_mask)); // negative arg will be NAN
  return x;
}

#define c_exp_hi 88.3762626647949f
#define c_exp_lo -88.3762626647949f

#define c_cephes_LOG2EF 1.44269504088896341
#define c_cephes_exp_C1 0.693359375
#define c_cephes_exp_C2 -2.12194440e-4

#define c_cephes_exp_p0 1.9875691500E-4
#define c_cephes_exp_p1 1.3981999507E-3
#define c_cephes_exp_p2 8.3334519073E-3
#define c_cephes_exp_p3 4.1665795894E-2
#define c_cephes_exp_p4 1.6666665459E-1
#define c_cephes_exp_p5 5.0000001201E-1

/* exp() computed for 4 float at once */
static inline v4sf exp_ps(v4sf x)
{
  v4si mm;
  x = vminq_f32(x, vdupq_n_f32(c_exp_hi));
  x = vmaxq_f32(x, vdupq_n_f32(c_exp_lo));

  /* express exp(x) as exp(g + n*log(2)) */
  v4sf t = vmulq_f32(x, vdupq_n_f32(c_cephes_LOG2EF));

  /* perform a floorf */
#if defined( __aarch64__ ) || defined( _M_ARM64 )
  mm = vcvtnq_s32_f32(t);
#else
  mm = vcvtq_s32_f32(vaddq_f32(t, vdupq_n_f32(0.5f)));
  /* if greater, substract 1 */
  v4su mask = vcltq_f32(t, vcvtq_f32_s32(mm));
  mm = vaddq_s32(mm, vreinterpretq_s32_u32(mask));
#endif

  v4sf e = vcvtq_f32_s32(mm);
  x = NEON_MLS(x, e, vdupq_n_f32(c_cephes_exp_C1));
  x = NEON_MLS(x, e, vdupq_n_f32(c_cephes_exp_C2));

  v4sf z = vmulq_f32(x, x);

  v4sf y = vdupq_n_f32(c_cephes_exp_p0);
  y = NEON_MLA(vdupq_n_f32(c_cephes_exp_p1), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_exp_p2), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_exp_p3), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_exp_p4), y, x);
  y = NEON_MLA(vdupq_n_f32(c_cephes_exp_p5), y, x);

  y = NEON_MLA(x, y, z);
  y = vaddq_f32(y, vdupq_n_f32(1.0f));

  /* build 2^n */
  mm = vaddq_s32(mm, vdupq_n_s32(0x7f));
  return vmulq_f32(y, vreinterpretq_f32_s32(vshlq_n_s32(mm, 23)));
}

#define c_minus_cephes_DP1 -0.78515625
#define c_minus_cephes_DP2 -2.4187564849853515625e-4
#define c_minus_cephes_DP3 -3.77489497744594108e-8
#define c_sincof_p0 -1.9515295891E-4
#define c_sincof_p1  8.3321608736E-3
#define c_sincof_p2 -1.6666654611E-1
#define c_coscof_p0  2.443315711809948E-005
#define c_coscof_p1 -1.388731625493765E-003
#define c_coscof_p2  4.166664568298827E-002
#define c_cephes_FOPI 1.27323954473516 // 4 / M_PI

/* evaluation of 4 sines & cosines at once.

   The code is the exact rewriting of the cephes sinf function.
   Precision is excellent as long as x < 8192 (I did not bother to
   take into account the special handling they have for greater values
   -- it does not return garbage for arguments over 8192, though, but
   the extra precision is missing).

   Note that it is such that sinf((float)M_PI) = 8.74e-8, which is the
   surprising but correct result.

   Note also that when you compute sin(x), cos(x) is available at
   almost no extra price so both sin_ps and cos_ps make use of
   sincos_ps..
  */
static inline void sincos_ps(v4sf x, v4sf *ysin, v4sf *ycos) { // any x
  v4su sign_mask_sin = vcltq_f32(x, vdupq_n_f32(0));
  x = vabsq_f32(x);

  /* scale by 4/Pi */
  v4sf y = vmulq_n_f32(x, c_cephes_FOPI);

  /* store the integer part of y in mm0 */
  v4su emm2 = vcvtq_u32_f32(y);
  /* j=(j+1) & (~1) (see the cephes sources) */
  emm2 = vandq_u32(vaddq_u32(emm2, vdupq_n_u32(1)), vdupq_n_u32(~1));
  y = vcvtq_f32_u32(emm2);

  /* get the polynom selection mask
     there is one polynom for 0 <= x <= Pi/4
     and another one for Pi/4<x<=Pi/2

     Both branches will be computed.
  */
  v4su poly_mask = vtstq_u32(emm2, vdupq_n_u32(2));

  /* The magic pass: "Extended precision modular arithmetic"
     x = ((x - y * DP1) - y * DP2) - y * DP3; */
  x = NEON_MLA_N(x, y, c_minus_cephes_DP1);
  x = NEON_MLA_N(x, y, c_minus_cephes_DP2);
  x = NEON_MLA_N(x, y, c_minus_cephes_DP3);

  sign_mask_sin = veorq_u32(sign_mask_sin, vtstq_u32(emm2, vdupq_n_u32(4)));
  v4su sign_mask_cos = vtstq_u32(vsubq_u32(emm2, vdupq_n_u32(2)), vdupq_n_u32(4));

  /* Evaluate the first polynom  (0 <= x <= Pi/4) in y1,
     and the second polynom      (Pi/4 <= x <= 0) in y2 */
  v4sf z = vmulq_f32(x, x);
  v4sf y1 = NEON_MLA_N(vdupq_n_f32(c_coscof_p1), z, c_coscof_p0);
  v4sf y2 = NEON_MLA_N(vdupq_n_f32(c_sincof_p1), z, c_sincof_p0);
  y1 = NEON_MLA(vdupq_n_f32(c_coscof_p2), y1, z);
  y2 = NEON_MLA(vdupq_n_f32(c_sincof_p2), y2, z);
  y1 = vmulq_f32(y1, z);
  y2 = vmulq_f32(y2, z);
  y1 = vmulq_f32(y1, z);
  y1 = NEON_MLS_N(y1, z, 0.5f);
  y2 = NEON_MLA(x, y2, x);
  y1 = vaddq_f32(y1, vdupq_n_f32(1.0f));

  /* select the correct result from the two polynoms */
  v4sf ys = vbslq_f32(poly_mask, y1, y2);
  v4sf yc = vbslq_f32(poly_mask, y2, y1);
  *ysin = vbslq_f32(sign_mask_sin, vnegq_f32(ys), ys);
  *ycos = vbslq_f32(sign_mask_cos, yc, vnegq_f32(yc));
}

static inline v4sf sin_ps(v4sf x) {
  v4sf ysin, ycos;
  sincos_ps(x, &ysin, &ycos);
  return ysin;
}

static inline v4sf cos_ps(v4sf x) {
  v4sf ysin, ycos;
  sincos_ps(x, &ysin, &ycos);
  return ycos;
}

static inline v4sf rsqrt_ps(v4sf val)
{
  v4sf est = vrsqrteq_f32(val);
  v4sf vec = vmulq_f32(est, est);
  vec = vrsqrtsq_f32(val, vec);
  est = vmulq_f32(vec, est);
  vec = vmulq_f32(est, est);
  vec = vrsqrtsq_f32(val, vec);
  est = vmulq_f32(vec, est);
  vec = vmulq_f32(est, est);
  vec = vrsqrtsq_f32(val, vec);
  est = vmulq_f32(vec, est);
  return est;
}

static inline v4sf sqrt_ps(v4sf val)
{
#if defined(__aarch64__)
  return vsqrtq_f32(val);
#else
  return vmulq_f32(rsqrt_ps(val), val);
#endif
}

static const float asinf_lut[7] = {
        1.5707961728,
        -0.2145852647,
        0.0887556286,
        -0.0488025043,
        0.0268999482,
        -0.0111462294,
        0.0022959648
};

static inline void asincos_ps(v4sf x, v4sf *yasin, v4sf *yacos )
{
  const v4su sign = vcltq_f32(x, vdupq_n_f32(0.0f));
  x = vabsq_f32(x);

  const v4sf one  = vdupq_n_f32(1.0f);
  const v4sf sat  = vdupq_n_f32(0.9999999f);
  const v4sf pi_2 = vdupq_n_f32(0.5f * M_PI_F);

  const v4sf c0 = vdupq_n_f32(asinf_lut[0]);
  const v4sf c1 = vdupq_n_f32(asinf_lut[1]);
  const v4sf c2 = vdupq_n_f32(asinf_lut[2]);
  const v4sf c3 = vdupq_n_f32(asinf_lut[3]);
  const v4sf c4 = vdupq_n_f32(asinf_lut[4]);
  const v4sf c5 = vdupq_n_f32(asinf_lut[5]);
  const v4sf c6 = vdupq_n_f32(asinf_lut[6]);

  const v4su saturate = vcgeq_f32(x, one);
  x = vbslq_f32(saturate, sat, x);

  v4sf x2 = vmulq_f32(x, x);
  v4sf x4 = vmulq_f32(x2, x2);

  v4sf p01 = NEON_MLA(c0, c1, x);
  v4sf p23 = NEON_MLA(c2, c3, x);
  // v4sf p23 = NEON_MLA(c2, c3, x2);
  v4sf p0123 = NEON_MLA(p01, p23, x2);
  v4sf p45 = NEON_MLA(c4, c5, x);
  v4sf p456 = NEON_MLA(p45, c6, x2);
  v4sf p = NEON_MLA(p0123, p456, x4);

  v4sf y = sqrt_ps(vsubq_f32(one, x));
  v4sf a = NEON_MLS(pi_2, y, p);

  *yasin = vbslq_f32(sign, vnegq_f32(a), a);
  *yacos = vsubq_f32(pi_2, *yasin);
}

static inline v4sf asin_ps(v4sf x)
{
  v4sf yasin, yacos;
  asincos_ps(x, &yasin, &yacos);
  return yasin;
}

static inline v4sf acos_ps(v4sf x)
{
  v4sf yasin, yacos;
  asincos_ps(x, &yasin, &yacos);
  return yacos;
}

