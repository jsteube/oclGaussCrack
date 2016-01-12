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

#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#define __MSVCRT_VERSION__ 0x0700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>

// NOTE! iconv your input file before using it
//
// iconv -c -f utf8 -t utf16le < in.txt > out.txt

int main (int argc, char *argv[])
{
  FILE *fd1;
  FILE *fd2;

  uint16_t buf1[BUFSIZ];
  uint16_t buf2[BUFSIZ];

  size_t len1;
  size_t len2;

  uint16_t out[BUFSIZ];

  uint16_t *out1 = out;
  uint16_t *out2 = NULL;

  if (argc != 3)
  {
    fprintf (stderr, "usage: %s file1 file2\n", argv[0]);

    return (-1);
  }

  if ((fd1 = fopen (argv[1], "rb")) == NULL)
  {
    fprintf (stderr, "%s: %s\n", argv[1], strerror (errno));

    return (-1);
  }

  if ((fd2 = fopen (argv[2], "rb")) == NULL)
  {
    fprintf (stderr, "%s: %s\n", argv[1], strerror (errno));

    return (-1);
  }

  while (!feof (fd1))
  {
    int i;

    for (i = 0, len1 = 0; i < BUFSIZ - 1; i++)
    {
      int c1 = fgetc (fd1);

      if (c1 == EOF) break;

      int c2 = fgetc (fd1);

      if (c2 == EOF) break;

      uint16_t c = (c1 << 0) | (c2 << 8);

      if (c == '\n') break;

      buf1[len1] = c;

      len1++;
    }

    memcpy (out1, buf1, len1 * sizeof (uint16_t));

    out2 = out1 + len1;

    while (!feof (fd2))
    {
      for (i = 0, len2 = 0; i < BUFSIZ - 1; i++)
      {
        int c1 = fgetc (fd2);

        if (c1 == EOF) break;

        int c2 = fgetc (fd2);

        if (c2 == EOF) break;

        uint16_t c = (c1 << 0) | (c2 << 8);

        if (c == '\n') break;

        buf2[len2] = c;

        len2++;
      }

      memcpy (out2, buf2, len2 * sizeof (uint16_t));

      fwrite ((char *) out, sizeof (uint16_t), len1 + len2, stdout);

      fputc ('\n', stdout);
      fputc ('\0', stdout);
    }

    rewind (fd2);
  }

  fclose (fd2);
  fclose (fd1);

  return 0;
}
