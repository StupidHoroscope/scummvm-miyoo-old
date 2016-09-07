/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

/* VLC code
 *
 * Original copyright note: * Intel Indeo 4 (IV41, IV42, etc.) video decoder for ffmpeg
 * written, produced, and directed by Alan Smithee
 */

#include "image/codecs/indeo/indeo_dsp.h"

namespace Image {
namespace Indeo {

/**
 * butterfly operation for the inverse Haar transform
 */
#define IVI_HAAR_BFLY(s1, s2, o1, o2, t) \
    t  = ((s1) - (s2)) >> 1;\
    o1 = ((s1) + (s2)) >> 1;\
    o2 = (t);\

/**
 * inverse 8-point Haar transform
 */
#define INV_HAAR8(s1, s5, s3, s7, s2, s4, s6, s8,\
                  d1, d2, d3, d4, d5, d6, d7, d8,\
                  t0, t1, t2, t3, t4, t5, t6, t7, t8) {\
    t1 = (s1) << 1; t5 = (s5) << 1;\
    IVI_HAAR_BFLY(t1, t5, t1, t5, t0); IVI_HAAR_BFLY(t1, s3, t1, t3, t0);\
    IVI_HAAR_BFLY(t5, s7, t5, t7, t0); IVI_HAAR_BFLY(t1, s2, t1, t2, t0);\
    IVI_HAAR_BFLY(t3, s4, t3, t4, t0); IVI_HAAR_BFLY(t5, s6, t5, t6, t0);\
    IVI_HAAR_BFLY(t7, s8, t7, t8, t0);\
    d1 = COMPENSATE(t1);\
    d2 = COMPENSATE(t2);\
    d3 = COMPENSATE(t3);\
    d4 = COMPENSATE(t4);\
    d5 = COMPENSATE(t5);\
    d6 = COMPENSATE(t6);\
    d7 = COMPENSATE(t7);\
    d8 = COMPENSATE(t8); }

/**
 * inverse 4-point Haar transform
 */
#define INV_HAAR4(s1, s3, s5, s7, d1, d2, d3, d4, t0, t1, t2, t3, t4) {\
    IVI_HAAR_BFLY(s1, s3, t0, t1, t4);\
    IVI_HAAR_BFLY(t0, s5, t2, t3, t4);\
    d1 = COMPENSATE(t2);\
    d2 = COMPENSATE(t3);\
    IVI_HAAR_BFLY(t1, s7, t2, t3, t4);\
    d3 = COMPENSATE(t2);\
    d4 = COMPENSATE(t3); }

void IndeoDSP::ff_ivi_inverse_haar_8x8(const int32 *in, int16 *out, uint32 pitch,
                             const uint8 *flags) {
    int     i, shift, sp1, sp2, sp3, sp4;
    const int32 *src;
    int32 *dst;
    int     tmp[64];
    int     t0, t1, t2, t3, t4, t5, t6, t7, t8;

    // apply the InvHaar8 to all columns
#define COMPENSATE(x) (x)
    src = in;
    dst = tmp;
    for (i = 0; i < 8; i++) {
        if (flags[i]) {
            // pre-scaling
            shift = !(i & 4);
            sp1 = src[ 0] << shift;
            sp2 = src[ 8] << shift;
            sp3 = src[16] << shift;
            sp4 = src[24] << shift;
            INV_HAAR8(    sp1,     sp2,     sp3,     sp4,
                      src[32], src[40], src[48], src[56],
                      dst[ 0], dst[ 8], dst[16], dst[24],
                      dst[32], dst[40], dst[48], dst[56],
                      t0, t1, t2, t3, t4, t5, t6, t7, t8);
        } else
            dst[ 0] = dst[ 8] = dst[16] = dst[24] =
            dst[32] = dst[40] = dst[48] = dst[56] = 0;

        src++;
        dst++;
    }
#undef  COMPENSATE

    // apply the InvHaar8 to all rows
#define COMPENSATE(x) (x)
    src = tmp;
    for (i = 0; i < 8; i++) {
        if (   !src[0] && !src[1] && !src[2] && !src[3]
            && !src[4] && !src[5] && !src[6] && !src[7]) {
            memset(out, 0, 8 * sizeof(out[0]));
        } else {
            INV_HAAR8(src[0], src[1], src[2], src[3],
                      src[4], src[5], src[6], src[7],
                      out[0], out[1], out[2], out[3],
                      out[4], out[5], out[6], out[7],
                      t0, t1, t2, t3, t4, t5, t6, t7, t8);
        }
        src += 8;
        out += pitch;
    }
#undef  COMPENSATE
}

void IndeoDSP::ff_ivi_row_haar8(const int32 *in, int16 *out, uint32 pitch,
                      const uint8 *flags) {
    int     i;
    int     t0, t1, t2, t3, t4, t5, t6, t7, t8;

    // apply the InvHaar8 to all rows
#define COMPENSATE(x) (x)
    for (i = 0; i < 8; i++) {
        if (   !in[0] && !in[1] && !in[2] && !in[3]
            && !in[4] && !in[5] && !in[6] && !in[7]) {
            memset(out, 0, 8 * sizeof(out[0]));
        } else {
            INV_HAAR8(in[0],  in[1],  in[2],  in[3],
                      in[4],  in[5],  in[6],  in[7],
                      out[0], out[1], out[2], out[3],
                      out[4], out[5], out[6], out[7],
                      t0, t1, t2, t3, t4, t5, t6, t7, t8);
        }
        in  += 8;
        out += pitch;
    }
#undef  COMPENSATE
}

void IndeoDSP::ff_ivi_col_haar8(const int32 *in, int16 *out, uint32 pitch,
                      const uint8 *flags) {
    int     i;
    int     t0, t1, t2, t3, t4, t5, t6, t7, t8;

    // apply the InvHaar8 to all columns
#define COMPENSATE(x) (x)
    for (i = 0; i < 8; i++) {
        if (flags[i]) {
            INV_HAAR8(in[ 0], in[ 8], in[16], in[24],
                      in[32], in[40], in[48], in[56],
                      out[0 * pitch], out[1 * pitch],
                      out[2 * pitch], out[3 * pitch],
                      out[4 * pitch], out[5 * pitch],
                      out[6 * pitch], out[7 * pitch],
                      t0, t1, t2, t3, t4, t5, t6, t7, t8);
        } else
            out[0 * pitch] = out[1 * pitch] =
            out[2 * pitch] = out[3 * pitch] =
            out[4 * pitch] = out[5 * pitch] =
            out[6 * pitch] = out[7 * pitch] = 0;

        in++;
        out++;
    }
#undef  COMPENSATE
}

void IndeoDSP::ff_ivi_inverse_haar_4x4(const int32 *in, int16 *out, uint32 pitch,
                             const uint8 *flags) {
    int     i, shift, sp1, sp2;
    const int32 *src;
    int32 *dst;
    int     tmp[16];
    int     t0, t1, t2, t3, t4;

    // apply the InvHaar4 to all columns
#define COMPENSATE(x) (x)
    src = in;
    dst = tmp;
    for (i = 0; i < 4; i++) {
        if (flags[i]) {
            // pre-scaling
            shift = !(i & 2);
            sp1 = src[0] << shift;
            sp2 = src[4] << shift;
            INV_HAAR4(   sp1,    sp2, src[8], src[12],
                      dst[0], dst[4], dst[8], dst[12],
                      t0, t1, t2, t3, t4);
        } else
            dst[0] = dst[4] = dst[8] = dst[12] = 0;

        src++;
        dst++;
    }
#undef  COMPENSATE

    // apply the InvHaar8 to all rows
#define COMPENSATE(x) (x)
    src = tmp;
    for (i = 0; i < 4; i++) {
        if (!src[0] && !src[1] && !src[2] && !src[3]) {
            memset(out, 0, 4 * sizeof(out[0]));
        } else {
            INV_HAAR4(src[0], src[1], src[2], src[3],
                      out[0], out[1], out[2], out[3],
                      t0, t1, t2, t3, t4);
        }
        src += 4;
        out += pitch;
    }
#undef  COMPENSATE
}

void IndeoDSP::ff_ivi_row_haar4(const int32 *in, int16 *out, uint32 pitch,
                      const uint8 *flags) {
    int     i;
    int     t0, t1, t2, t3, t4;

    // apply the InvHaar4 to all rows
#define COMPENSATE(x) (x)
    for (i = 0; i < 4; i++) {
        if (!in[0] && !in[1] && !in[2] && !in[3]) {
            memset(out, 0, 4 * sizeof(out[0]));
        } else {
            INV_HAAR4(in[0], in[1], in[2], in[3],
                      out[0], out[1], out[2], out[3],
                      t0, t1, t2, t3, t4);
        }
        in  += 4;
        out += pitch;
    }
#undef  COMPENSATE
}

void IndeoDSP::ff_ivi_col_haar4(const int32 *in, int16 *out, uint32 pitch,
                      const uint8 *flags) {
    int     i;
    int     t0, t1, t2, t3, t4;

    // apply the InvHaar8 to all columns
#define COMPENSATE(x) (x)
    for (i = 0; i < 4; i++) {
        if (flags[i]) {
            INV_HAAR4(in[0], in[4], in[8], in[12],
                      out[0 * pitch], out[1 * pitch],
                      out[2 * pitch], out[3 * pitch],
                      t0, t1, t2, t3, t4);
        } else
            out[0 * pitch] = out[1 * pitch] =
            out[2 * pitch] = out[3 * pitch] = 0;

        in++;
        out++;
    }
#undef  COMPENSATE
}

void IndeoDSP::ff_ivi_dc_haar_2d(const int32 *in, int16 *out, uint32 pitch,
                       int blk_size) {
    int     x, y;
    int16 dc_coeff;

    dc_coeff = (*in + 0) >> 3;

    for (y = 0; y < blk_size; out += pitch, y++) {
        for (x = 0; x < blk_size; x++)
            out[x] = dc_coeff;
    }
}

//* butterfly operation for the inverse slant transform
#define IVI_SLANT_BFLY(s1, s2, o1, o2, t) \
    t  = (s1) - (s2);\
    o1 = (s1) + (s2);\
    o2 = (t);\

//* This is a reflection a,b = 1/2, 5/4 for the inverse slant transform
#define IVI_IREFLECT(s1, s2, o1, o2, t) \
    t  = (((s1) + (s2)*2 + 2) >> 2) + (s1);\
    o2 = (((s1)*2 - (s2) + 2) >> 2) - (s2);\
    o1 = (t);\

//* This is a reflection a,b = 1/2, 7/8 for the inverse slant transform
#define IVI_SLANT_PART4(s1, s2, o1, o2, t) \
    t  = (s2) + (((s1)*4  - (s2) + 4) >> 3);\
    o2 = (s1) + ((-(s1) - (s2)*4 + 4) >> 3);\
    o1 = (t);\

//* inverse slant8 transform
#define IVI_INV_SLANT8(s1, s4, s8, s5, s2, s6, s3, s7,\
                       d1, d2, d3, d4, d5, d6, d7, d8,\
                       t0, t1, t2, t3, t4, t5, t6, t7, t8) {\
    IVI_SLANT_PART4(s4, s5, t4, t5, t0);\
\
    IVI_SLANT_BFLY(s1, t5, t1, t5, t0); IVI_SLANT_BFLY(s2, s6, t2, t6, t0);\
    IVI_SLANT_BFLY(s7, s3, t7, t3, t0); IVI_SLANT_BFLY(t4, s8, t4, t8, t0);\
\
    IVI_SLANT_BFLY(t1, t2, t1, t2, t0); IVI_IREFLECT  (t4, t3, t4, t3, t0);\
    IVI_SLANT_BFLY(t5, t6, t5, t6, t0); IVI_IREFLECT  (t8, t7, t8, t7, t0);\
    IVI_SLANT_BFLY(t1, t4, t1, t4, t0); IVI_SLANT_BFLY(t2, t3, t2, t3, t0);\
    IVI_SLANT_BFLY(t5, t8, t5, t8, t0); IVI_SLANT_BFLY(t6, t7, t6, t7, t0);\
    d1 = COMPENSATE(t1);\
    d2 = COMPENSATE(t2);\
    d3 = COMPENSATE(t3);\
    d4 = COMPENSATE(t4);\
    d5 = COMPENSATE(t5);\
    d6 = COMPENSATE(t6);\
    d7 = COMPENSATE(t7);\
    d8 = COMPENSATE(t8);}

//* inverse slant4 transform
#define IVI_INV_SLANT4(s1, s4, s2, s3, d1, d2, d3, d4, t0, t1, t2, t3, t4) {\
    IVI_SLANT_BFLY(s1, s2, t1, t2, t0); IVI_IREFLECT  (s4, s3, t4, t3, t0);\
\
    IVI_SLANT_BFLY(t1, t4, t1, t4, t0); IVI_SLANT_BFLY(t2, t3, t2, t3, t0);\
    d1 = COMPENSATE(t1);\
    d2 = COMPENSATE(t2);\
    d3 = COMPENSATE(t3);\
    d4 = COMPENSATE(t4);}

void IndeoDSP::ff_ivi_inverse_slant_8x8(const int32 *in, int16 *out, uint32 pitch, const uint8 *flags) {
    int     i;
    const int32 *src;
    int32 *dst;
    int     tmp[64];
    int     t0, t1, t2, t3, t4, t5, t6, t7, t8;

#define COMPENSATE(x) (x)
    src = in;
    dst = tmp;
    for (i = 0; i < 8; i++) {
        if (flags[i]) {
            IVI_INV_SLANT8(src[0], src[8], src[16], src[24], src[32], src[40], src[48], src[56],
                           dst[0], dst[8], dst[16], dst[24], dst[32], dst[40], dst[48], dst[56],
                           t0, t1, t2, t3, t4, t5, t6, t7, t8);
        } else
            dst[0] = dst[8] = dst[16] = dst[24] = dst[32] = dst[40] = dst[48] = dst[56] = 0;

            src++;
            dst++;
    }
#undef COMPENSATE

#define COMPENSATE(x) (((x) + 1)>>1)
    src = tmp;
    for (i = 0; i < 8; i++) {
        if (!src[0] && !src[1] && !src[2] && !src[3] && !src[4] && !src[5] && !src[6] && !src[7]) {
            memset(out, 0, 8*sizeof(out[0]));
        } else {
            IVI_INV_SLANT8(src[0], src[1], src[2], src[3], src[4], src[5], src[6], src[7],
                           out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
                           t0, t1, t2, t3, t4, t5, t6, t7, t8);
        }
        src += 8;
        out += pitch;
    }
#undef COMPENSATE
}

void IndeoDSP::ff_ivi_inverse_slant_4x4(const int32 *in, int16 *out, uint32 pitch, const uint8 *flags) {
    int     i;
    const int32 *src;
    int32 *dst;
    int     tmp[16];
    int     t0, t1, t2, t3, t4;

#define COMPENSATE(x) (x)
    src = in;
    dst = tmp;
    for (i = 0; i < 4; i++) {
        if (flags[i]) {
            IVI_INV_SLANT4(src[0], src[4], src[8], src[12],
                           dst[0], dst[4], dst[8], dst[12],
                           t0, t1, t2, t3, t4);
        } else
            dst[0] = dst[4] = dst[8] = dst[12] = 0;

            src++;
            dst++;
    }
#undef COMPENSATE

#define COMPENSATE(x) (((x) + 1)>>1)
    src = tmp;
    for (i = 0; i < 4; i++) {
        if (!src[0] && !src[1] && !src[2] && !src[3]) {
            out[0] = out[1] = out[2] = out[3] = 0;
        } else {
            IVI_INV_SLANT4(src[0], src[1], src[2], src[3],
                           out[0], out[1], out[2], out[3],
                           t0, t1, t2, t3, t4);
        }
        src += 4;
        out += pitch;
    }
#undef COMPENSATE
}

void IndeoDSP::ff_ivi_dc_slant_2d(const int32 *in, int16 *out, uint32 pitch,
		int blk_size) {
    int     x, y;
    int16 dc_coeff;

    dc_coeff = (*in + 1) >> 1;

    for (y = 0; y < blk_size; out += pitch, y++) {
        for (x = 0; x < blk_size; x++)
            out[x] = dc_coeff;
    }
}

void IndeoDSP::ff_ivi_row_slant8(const int32 *in, int16 *out, uint32 pitch,
		const uint8 *flags) {
    int     i;
    int     t0, t1, t2, t3, t4, t5, t6, t7, t8;

#define COMPENSATE(x) (((x) + 1)>>1)
    for (i = 0; i < 8; i++) {
        if (!in[0] && !in[1] && !in[2] && !in[3] && !in[4] && !in[5] && !in[6] && !in[7]) {
            memset(out, 0, 8*sizeof(out[0]));
        } else {
            IVI_INV_SLANT8( in[0],  in[1],  in[2],  in[3],  in[4],  in[5],  in[6],  in[7],
                           out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7],
                           t0, t1, t2, t3, t4, t5, t6, t7, t8);
        }
        in += 8;
        out += pitch;
    }
#undef COMPENSATE
}

void IndeoDSP::ff_ivi_dc_row_slant(const int32 *in, int16 *out, uint32 pitch, int blk_size) {
    int     x, y;
    int16 dc_coeff;

    dc_coeff = (*in + 1) >> 1;

    for (x = 0; x < blk_size; x++)
        out[x] = dc_coeff;

    out += pitch;

    for (y = 1; y < blk_size; out += pitch, y++) {
        for (x = 0; x < blk_size; x++)
            out[x] = 0;
    }
}

void IndeoDSP::ff_ivi_col_slant8(const int32 *in, int16 *out, uint32 pitch, const uint8 *flags) {
    int     i, row2, row4, row8;
    int     t0, t1, t2, t3, t4, t5, t6, t7, t8;

    row2 = pitch << 1;
    row4 = pitch << 2;
    row8 = pitch << 3;

#define COMPENSATE(x) (((x) + 1)>>1)
    for (i = 0; i < 8; i++) {
        if (flags[i]) {
            IVI_INV_SLANT8(in[0], in[8], in[16], in[24], in[32], in[40], in[48], in[56],
                           out[0], out[pitch], out[row2], out[row2 + pitch], out[row4],
                           out[row4 + pitch],  out[row4 + row2], out[row8 - pitch],
                           t0, t1, t2, t3, t4, t5, t6, t7, t8);
        } else {
            out[0] = out[pitch] = out[row2] = out[row2 + pitch] = out[row4] =
            out[row4 + pitch] =  out[row4 + row2] = out[row8 - pitch] = 0;
        }

        in++;
        out++;
    }
#undef COMPENSATE
}

void IndeoDSP::ff_ivi_dc_col_slant(const int32 *in, int16 *out, uint32 pitch, int blk_size) {
    int     x, y;
    int16 dc_coeff;

    dc_coeff = (*in + 1) >> 1;

    for (y = 0; y < blk_size; out += pitch, y++) {
        out[0] = dc_coeff;
        for (x = 1; x < blk_size; x++)
            out[x] = 0;
    }
}

void IndeoDSP::ff_ivi_row_slant4(const int32 *in, int16 *out,
		uint32 pitch, const uint8 *flags) {
    int     i;
    int     t0, t1, t2, t3, t4;

#define COMPENSATE(x) (((x) + 1)>>1)
    for (i = 0; i < 4; i++) {
        if (!in[0] && !in[1] && !in[2] && !in[3]) {
            memset(out, 0, 4*sizeof(out[0]));
        } else {
            IVI_INV_SLANT4( in[0],  in[1],  in[2],  in[3],
                           out[0], out[1], out[2], out[3],
                           t0, t1, t2, t3, t4);
        }
        in  += 4;
        out += pitch;
    }
#undef COMPENSATE
}

void IndeoDSP::ff_ivi_col_slant4(const int32 *in, int16 *out, uint32 pitch,
		const uint8 *flags) {
    int     i, row2;
    int     t0, t1, t2, t3, t4;

    row2 = pitch << 1;

#define COMPENSATE(x) (((x) + 1)>>1)
    for (i = 0; i < 4; i++) {
        if (flags[i]) {
            IVI_INV_SLANT4(in[0], in[4], in[8], in[12],
                           out[0], out[pitch], out[row2], out[row2 + pitch],
                           t0, t1, t2, t3, t4);
        } else {
            out[0] = out[pitch] = out[row2] = out[row2 + pitch] = 0;
        }

        in++;
        out++;
    }
#undef COMPENSATE
}

void IndeoDSP::ff_ivi_put_pixels_8x8(const int32 *in, int16 *out, uint32 pitch,
		const uint8 *flags) {
    int     x, y;

    for (y = 0; y < 8; out += pitch, in += 8, y++)
        for (x = 0; x < 8; x++)
            out[x] = in[x];
}

void IndeoDSP::ff_ivi_put_dc_pixel_8x8(const int32 *in, int16 *out, uint32 pitch,
		int blk_size) {
    int     y;

    out[0] = in[0];
    memset(out + 1, 0, 7*sizeof(out[0]));
    out += pitch;

    for (y = 1; y < 8; out += pitch, y++)
        memset(out, 0, 8*sizeof(out[0]));
}

#define IVI_MC_TEMPLATE(size, suffix, OP) \
static void ivi_mc_ ## size ##x## size ## suffix(int16 *buf, \
                                                 uint32 dpitch, \
                                                 const int16 *ref_buf, \
                                                 uint32 pitch, int mc_type) \
{ \
    int     i, j; \
    const int16 *wptr; \
\
    switch (mc_type) { \
    case 0: /* fullpel (no interpolation) */ \
        for (i = 0; i < size; i++, buf += dpitch, ref_buf += pitch) { \
            for (j = 0; j < size; j++) {\
                OP(buf[j], ref_buf[j]); \
            } \
        } \
        break; \
    case 1: /* horizontal halfpel interpolation */ \
        for (i = 0; i < size; i++, buf += dpitch, ref_buf += pitch) \
            for (j = 0; j < size; j++) \
                OP(buf[j], (ref_buf[j] + ref_buf[j+1]) >> 1); \
        break; \
    case 2: /* vertical halfpel interpolation */ \
        wptr = ref_buf + pitch; \
        for (i = 0; i < size; i++, buf += dpitch, wptr += pitch, ref_buf += pitch) \
            for (j = 0; j < size; j++) \
                OP(buf[j], (ref_buf[j] + wptr[j]) >> 1); \
        break; \
    case 3: /* vertical and horizontal halfpel interpolation */ \
        wptr = ref_buf + pitch; \
        for (i = 0; i < size; i++, buf += dpitch, wptr += pitch, ref_buf += pitch) \
            for (j = 0; j < size; j++) \
                OP(buf[j], (ref_buf[j] + ref_buf[j+1] + wptr[j] + wptr[j+1]) >> 2); \
        break; \
    } \
} \
\
void IndeoDSP::ff_ivi_mc_ ## size ##x## size ## suffix(int16 *buf, const int16 *ref_buf, \
                                             uint32 pitch, int mc_type) \
{ \
    ivi_mc_ ## size ##x## size ## suffix(buf, pitch, ref_buf, pitch, mc_type); \
}

#define IVI_MC_AVG_TEMPLATE(size, suffix, OP) \
void IndeoDSP::ff_ivi_mc_avg_ ## size ##x## size ## suffix(int16 *buf, \
                                                 const int16 *ref_buf, \
                                                 const int16 *ref_buf2, \
                                                 uint32 pitch, \
                                               int mc_type, int mc_type2) \
{ \
    int16 tmp[size * size]; \
    int i, j; \
\
    ivi_mc_ ## size ##x## size ## _no_delta(tmp, size, ref_buf, pitch, mc_type); \
    ivi_mc_ ## size ##x## size ## _delta(tmp, size, ref_buf2, pitch, mc_type2); \
    for (i = 0; i < size; i++, buf += pitch) { \
        for (j = 0; j < size; j++) {\
            OP(buf[j], tmp[i * size + j] >> 1); \
        } \
    } \
}

#define OP_PUT(a, b)  (a) = (b)
#define OP_ADD(a, b)  (a) += (b)

IVI_MC_TEMPLATE(8, _no_delta, OP_PUT)
IVI_MC_TEMPLATE(8, _delta,    OP_ADD)
IVI_MC_TEMPLATE(4, _no_delta, OP_PUT)
IVI_MC_TEMPLATE(4, _delta,    OP_ADD)
IVI_MC_AVG_TEMPLATE(8, _no_delta, OP_PUT)
IVI_MC_AVG_TEMPLATE(8, _delta,    OP_ADD)
IVI_MC_AVG_TEMPLATE(4, _no_delta, OP_PUT)
IVI_MC_AVG_TEMPLATE(4, _delta,    OP_ADD)

} // End of namespace Indeo
} // End of namespace Image
