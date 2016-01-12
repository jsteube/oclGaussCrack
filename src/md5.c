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

#define MD5M_A 0x67452301u
#define MD5M_B 0xefcdab89u
#define MD5M_C 0x98badcfeu
#define MD5M_D 0x10325476u

#define MD5S00  7u
#define MD5S01 12u
#define MD5S02 17u
#define MD5S03 22u
#define MD5S10  5u
#define MD5S11  9u
#define MD5S12 14u
#define MD5S13 20u
#define MD5S20  4u
#define MD5S21 11u
#define MD5S22 16u
#define MD5S23 23u
#define MD5S30  6u
#define MD5S31 10u
#define MD5S32 15u
#define MD5S33 21u

#define MD5C00 0xd76aa478u
#define MD5C01 0xe8c7b756u
#define MD5C02 0x242070dbu
#define MD5C03 0xc1bdceeeu
#define MD5C04 0xf57c0fafu
#define MD5C05 0x4787c62au
#define MD5C06 0xa8304613u
#define MD5C07 0xfd469501u
#define MD5C08 0x698098d8u
#define MD5C09 0x8b44f7afu
#define MD5C0a 0xffff5bb1u
#define MD5C0b 0x895cd7beu
#define MD5C0c 0x6b901122u
#define MD5C0d 0xfd987193u
#define MD5C0e 0xa679438eu
#define MD5C0f 0x49b40821u
#define MD5C10 0xf61e2562u
#define MD5C11 0xc040b340u
#define MD5C12 0x265e5a51u
#define MD5C13 0xe9b6c7aau
#define MD5C14 0xd62f105du
#define MD5C15 0x02441453u
#define MD5C16 0xd8a1e681u
#define MD5C17 0xe7d3fbc8u
#define MD5C18 0x21e1cde6u
#define MD5C19 0xc33707d6u
#define MD5C1a 0xf4d50d87u
#define MD5C1b 0x455a14edu
#define MD5C1c 0xa9e3e905u
#define MD5C1d 0xfcefa3f8u
#define MD5C1e 0x676f02d9u
#define MD5C1f 0x8d2a4c8au
#define MD5C20 0xfffa3942u
#define MD5C21 0x8771f681u
#define MD5C22 0x6d9d6122u
#define MD5C23 0xfde5380cu
#define MD5C24 0xa4beea44u
#define MD5C25 0x4bdecfa9u
#define MD5C26 0xf6bb4b60u
#define MD5C27 0xbebfbc70u
#define MD5C28 0x289b7ec6u
#define MD5C29 0xeaa127fau
#define MD5C2a 0xd4ef3085u
#define MD5C2b 0x04881d05u
#define MD5C2c 0xd9d4d039u
#define MD5C2d 0xe6db99e5u
#define MD5C2e 0x1fa27cf8u
#define MD5C2f 0xc4ac5665u
#define MD5C30 0xf4292244u
#define MD5C31 0x432aff97u
#define MD5C32 0xab9423a7u
#define MD5C33 0xfc93a039u
#define MD5C34 0x655b59c3u
#define MD5C35 0x8f0ccc92u
#define MD5C36 0xffeff47du
#define MD5C37 0x85845dd1u
#define MD5C38 0x6fa87e4fu
#define MD5C39 0xfe2ce6e0u
#define MD5C3a 0xa3014314u
#define MD5C3b 0x4e0811a1u
#define MD5C3c 0xf7537e82u
#define MD5C3d 0xbd3af235u
#define MD5C3e 0x2ad7d2bbu
#define MD5C3f 0xeb86d391u

#define MD5_F(x,y,z)    z ^ (x & (y ^ z))
#define MD5_G(x,y,z)    y ^ (z & (x ^ y))
#define MD5_H1(x,y,z)   (tmp2 = (x ^ y)) ^ z
#define MD5_H2(x,y,z)   x ^ tmp2
#define MD5_I(x,y,z)    y ^ (x | ~z)

#define MD5_Fo(x,y,z)   bitselect (z, y, x)
#define MD5_Go(x,y,z)   bitselect (y, x, z)

#define ROTL(a,n)       rotate (a, n)

#define MD5_STEP(f,a,b,c,d,x,K,s)   \
{                                   \
  a += K;                           \
  a += x;                           \
  a += f (b, c, d);                 \
  a  = ROTL (a, s);                 \
  a += b;                           \
}

#define MD5_STEP0(f,a,b,c,d,K,s)    \
{                                   \
  a += K;                           \
  a += f (b, c, d);                 \
  a  = ROTL (a, s);                 \
  a += b;                           \
}

#define MD5_STEPZ(f,a,b,c,d,x,K,s)  \
{                                   \
  a += x;                           \
  a  = ROTL (a, s);                 \
  a += b;                           \
}

