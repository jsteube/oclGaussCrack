/*
 * oclGaussCrack - OpenCL accelerated Gauss hash validator
 *
 * Copyright (C) 2012 Jens Steube <jens.steube@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

__constant uint TARGETS[4][4] =
{
  { 0x9aa08e75, 0xcacb7d14, 0x8b55bdd6, 0xde7407e3 },
  { 0x23172feb, 0xd91e2698, 0x21058d4c, 0x9b915066 },
  { 0x6b4ddd52, 0xc4842d79, 0x8ea0e622, 0xb8ac7242 },
  { 0xeafab353, 0x901bcc53, 0xcf5f2caa, 0xe2f91e83 },
};

/*
__constant uint TARGETS[4][4] =
{
  { 0xe75c4076, 0x355ee7f4, 0xd9d41c2c, 0x41beb6ae },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
  { 0, 0, 0, 0 },
};
*/

#include "md5.c"

#define ITERATIONS  10000

typedef struct __block
{
  uint4 buf[4];

} block_t;

__kernel void oclGaussCrack (const __global block_t *blocks, __global uint *results)
{
  const int gid = get_global_id (0);
  const int lid = get_local_id (0);

  uint4 W[4];

  W[0] = blocks[gid].buf[0];
  W[1] = blocks[gid].buf[1];
  W[2] = blocks[gid].buf[2];
  W[3] = blocks[gid].buf[3];

  const uint W04 = 0x80;
  const uint W14 = 16 * 8;

  uint az = MD5M_A;
  uint bz = MD5M_B;
  uint cz = MD5M_C;
  uint dz = MD5M_D;

  az = az + MD5C00 + MD5_Fo (bz, cz, dz);

  for (int i = 0; i < ITERATIONS; i++)
  {
    uint4 tmp2;

    uint4 a = az;
    uint4 b = bz;
    uint4 c = cz;
    uint4 d = dz;

    MD5_STEPZ(MD5_Fo, a, b, c, d, W[0], MD5C00, MD5S00);
    MD5_STEP (MD5_Fo, d, a, b, c, W[1], MD5C01, MD5S01);
    MD5_STEP (MD5_Fo, c, d, a, b, W[2], MD5C02, MD5S02);
    MD5_STEP (MD5_Fo, b, c, d, a, W[3], MD5C03, MD5S03);
    MD5_STEP (MD5_Fo, a, b, c, d, W04,  MD5C04, MD5S00);
    MD5_STEP0(MD5_Fo, d, a, b, c,       MD5C05, MD5S01);
    MD5_STEP0(MD5_Fo, c, d, a, b,       MD5C06, MD5S02);
    MD5_STEP0(MD5_Fo, b, c, d, a,       MD5C07, MD5S03);
    MD5_STEP0(MD5_Fo, a, b, c, d,       MD5C08, MD5S00);
    MD5_STEP0(MD5_Fo, d, a, b, c,       MD5C09, MD5S01);
    MD5_STEP0(MD5_Fo, c, d, a, b,       MD5C0a, MD5S02);
    MD5_STEP0(MD5_Fo, b, c, d, a,       MD5C0b, MD5S03);
    MD5_STEP0(MD5_Fo, a, b, c, d,       MD5C0c, MD5S00);
    MD5_STEP0(MD5_Fo, d, a, b, c,       MD5C0d, MD5S01);
    MD5_STEP (MD5_Fo, c, d, a, b, W14,  MD5C0e, MD5S02);
    MD5_STEP0(MD5_Fo, b, c, d, a,       MD5C0f, MD5S03);

    MD5_STEP (MD5_Go, a, b, c, d, W[1], MD5C10, MD5S10);
    MD5_STEP0(MD5_Go, d, a, b, c,       MD5C11, MD5S11);
    MD5_STEP0(MD5_Go, c, d, a, b,       MD5C12, MD5S12);
    MD5_STEP (MD5_Go, b, c, d, a, W[0], MD5C13, MD5S13);
    MD5_STEP0(MD5_Go, a, b, c, d,       MD5C14, MD5S10);
    MD5_STEP0(MD5_Go, d, a, b, c,       MD5C15, MD5S11);
    MD5_STEP0(MD5_Go, c, d, a, b,       MD5C16, MD5S12);
    MD5_STEP (MD5_Go, b, c, d, a, W04,  MD5C17, MD5S13);
    MD5_STEP0(MD5_Go, a, b, c, d,       MD5C18, MD5S10);
    MD5_STEP (MD5_Go, d, a, b, c, W14,  MD5C19, MD5S11);
    MD5_STEP (MD5_Go, c, d, a, b, W[3], MD5C1a, MD5S12);
    MD5_STEP0(MD5_Go, b, c, d, a,       MD5C1b, MD5S13);
    MD5_STEP0(MD5_Go, a, b, c, d,       MD5C1c, MD5S10);
    MD5_STEP (MD5_Go, d, a, b, c, W[2], MD5C1d, MD5S11);
    MD5_STEP0(MD5_Go, c, d, a, b,       MD5C1e, MD5S12);
    MD5_STEP0(MD5_Go, b, c, d, a,       MD5C1f, MD5S13);

    MD5_STEP0(MD5_H1, a, b, c, d,       MD5C20, MD5S20);
    MD5_STEP0(MD5_H2, d, a, b, c,       MD5C21, MD5S21);
    MD5_STEP0(MD5_H1, c, d, a, b,       MD5C22, MD5S22);
    MD5_STEP (MD5_H2, b, c, d, a, W14,  MD5C23, MD5S23);
    MD5_STEP (MD5_H1, a, b, c, d, W[1], MD5C24, MD5S20);
    MD5_STEP (MD5_H2, d, a, b, c, W04,  MD5C25, MD5S21);
    MD5_STEP0(MD5_H1, c, d, a, b,       MD5C26, MD5S22);
    MD5_STEP0(MD5_H2, b, c, d, a,       MD5C27, MD5S23);
    MD5_STEP0(MD5_H1, a, b, c, d,       MD5C28, MD5S20);
    MD5_STEP (MD5_H2, d, a, b, c, W[0], MD5C29, MD5S21);
    MD5_STEP (MD5_H1, c, d, a, b, W[3], MD5C2a, MD5S22);
    MD5_STEP0(MD5_H2, b, c, d, a,       MD5C2b, MD5S23);
    MD5_STEP0(MD5_H1, a, b, c, d,       MD5C2c, MD5S20);
    MD5_STEP0(MD5_H2, d, a, b, c,       MD5C2d, MD5S21);
    MD5_STEP0(MD5_H1, c, d, a, b,       MD5C2e, MD5S22);
    MD5_STEP (MD5_H2, b, c, d, a, W[2], MD5C2f, MD5S23);

    MD5_STEP (MD5_I , a, b, c, d, W[0], MD5C30, MD5S30);
    MD5_STEP0(MD5_I , d, a, b, c,       MD5C31, MD5S31);
    MD5_STEP (MD5_I , c, d, a, b, W14,  MD5C32, MD5S32);
    MD5_STEP0(MD5_I , b, c, d, a,       MD5C33, MD5S33);
    MD5_STEP0(MD5_I , a, b, c, d,       MD5C34, MD5S30);
    MD5_STEP (MD5_I , d, a, b, c, W[3], MD5C35, MD5S31);
    MD5_STEP0(MD5_I , c, d, a, b,       MD5C36, MD5S32);
    MD5_STEP (MD5_I , b, c, d, a, W[1], MD5C37, MD5S33);
    MD5_STEP0(MD5_I , a, b, c, d,       MD5C38, MD5S30);
    MD5_STEP0(MD5_I , d, a, b, c,       MD5C39, MD5S31);
    MD5_STEP0(MD5_I , c, d, a, b,       MD5C3a, MD5S32);
    MD5_STEP0(MD5_I , b, c, d, a,       MD5C3b, MD5S33);
    MD5_STEP (MD5_I , a, b, c, d, W04,  MD5C3c, MD5S30);
    MD5_STEP0(MD5_I , d, a, b, c,       MD5C3d, MD5S31);
    MD5_STEP (MD5_I , c, d, a, b, W[2], MD5C3e, MD5S32);
    MD5_STEP0(MD5_I , b, c, d, a,       MD5C3f, MD5S33);

    W[0] = a + MD5M_A;
    W[1] = b + MD5M_B;
    W[2] = c + MD5M_C;
    W[3] = d + MD5M_D;
  }

  for (int i = 0; i < 4; i++)
  {
    const uint target_a = TARGETS[i][0];
    const uint target_b = TARGETS[i][1];
    const uint target_c = TARGETS[i][2];
    const uint target_d = TARGETS[i][3];

    if (all (W[0] != target_a)) continue;
    if (all (W[1] != target_b)) continue;
    if (all (W[2] != target_c)) continue;
    if (all (W[3] != target_d)) continue;

    const int gid4 = gid * 4;

    if ((W[0].s0 == target_a)
     && (W[1].s0 == target_b)
     && (W[2].s0 == target_c)
     && (W[3].s0 == target_d)) results[lid] = gid4 + 0;

    if ((W[0].s1 == target_a)
     && (W[1].s1 == target_b)
     && (W[2].s1 == target_c)
     && (W[3].s1 == target_d)) results[lid] = gid4 + 1;

    if ((W[0].s2 == target_a)
     && (W[1].s2 == target_b)
     && (W[2].s2 == target_c)
     && (W[3].s2 == target_d)) results[lid] = gid4 + 2;

    if ((W[0].s3 == target_a)
     && (W[1].s3 == target_b)
     && (W[2].s3 == target_c)
     && (W[3].s3 == target_d)) results[lid] = gid4 + 3;
  }
}
