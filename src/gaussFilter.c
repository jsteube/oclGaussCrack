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
  FILE *fd;

  if (argc != 2)
  {
    fprintf (stderr, "usage: %s file\n", argv[0]);

    return (-1);
  }

  if ((fd = fopen (argv[1], "rb")) == NULL)
  {
    fprintf (stderr, "%s: %s\n", argv[1], strerror (errno));

    return (-1);
  }

  while (!feof (fd))
  {
    /* Get new password candidate from stdin */

    uint16_t line_buf[BUFSIZ];

    int line_len = 0;

    int i;

    for (i = 0; i < BUFSIZ - 1; i++)
    {
      int c1 = fgetc (fd);

      if (c1 == EOF) break;

      int c2 = fgetc (fd);

      if (c2 == EOF) break;

      uint16_t c = (c1 << 0) | (c2 << 8);

      line_buf[line_len] = c;

      line_len++;

      if (c == '\n') break;
    }

    if (line_len == 0) continue;

    if (line_buf[0] <= 0x7a) continue;

    fwrite ((char *) line_buf, sizeof (uint16_t), line_len, stdout);
  }

  fclose (fd);

  return 0;
}
