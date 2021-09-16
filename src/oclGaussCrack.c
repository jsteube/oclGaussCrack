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

#define CL_TARGET_OPENCL_VERSION 120

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <search.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <CL/cl.h>

#include "md5.c"
#include "OpenCL.c"

#define KERNEL_SRC    "src/oclGaussCrack.cl"
#define KERNEL_NAME   "oclGaussCrack"
#define POTFILE       "oclGaussCrack.pot"
#define BUILD_OPTS    "-I . -I src"
#define DEV_TYPE      CL_DEVICE_TYPE_GPU
#define MAX_LINELEN   256 + 100
#define MAX_PLATFORM  16
#define MAX_GPU       16
#define VECT_SIZE     4   // must be power of 2
#define GPU_THREADS   64  // must be power of 2
#define GPU_ACCEL     32

#define rotate(x,n)   (x << n) | (x >> (32 - n))

#define timer_set(a)  gettimeofday (a, NULL)

#define timer_get(a,r)                        \
{                                             \
  struct timeval hr_tmp;                      \
  timer_set (&hr_tmp);                        \
  r = ((hr_tmp.tv_sec  - a.tv_sec)  * 1000)   \
    + ((hr_tmp.tv_usec - a.tv_usec) / 1000);  \
}

typedef struct __block
{
  /* it is using uint4 in kernel thats why this looks a bit weird */

  cl_uint A[4];
  cl_uint B[4];
  cl_uint C[4];
  cl_uint D[4];

} block_t;

typedef struct
{
  cl_context          context;
  cl_program          program;
  cl_kernel           kernel;
  cl_command_queue    command_queue;

  cl_uint             max_compute_units;
  uint32_t            num_threads;
  uint32_t            num_elements;
  uint32_t            num_cached;
  uint32_t            num_work;

  uint8_t           **plains_buf;
  size_t             *plains_len;

  cl_mem              d_block;
  cl_mem              d_results;
  block_t            *h_block;
  uint32_t           *h_results;

  struct timeval      timer;

} gpu_ctx_t;

static void dump_hex (FILE *fd, const uint8_t *s, const size_t sz)
{
  for (size_t i = 0; i < sz; i += 8)
  {
    for (size_t j = 0; j < 8; j += 1)
    {
      const size_t pos = i + j;

      if (pos == sz) break;

      fprintf (fd, "%02x ", s[pos]);
    }

    fprintf (fd, "\n");
  }

  fprintf (fd, "\n");
}

static void calc_work (const cl_uint num_devices, gpu_ctx_t *gpu_ctxs)
{
  for (cl_uint device_id = 0; device_id < num_devices; device_id++)
  {
    gpu_ctx_t *gpu_ctx = &gpu_ctxs[device_id];

    if (gpu_ctx->num_cached == 0) continue;

    gpu_ctx->num_work = ((gpu_ctx->num_cached + (VECT_SIZE - 1)) & ~(VECT_SIZE - 1)) / VECT_SIZE;
  }
}

static void launch_kernel (const cl_uint num_devices, gpu_ctx_t *gpu_ctxs)
{
  for (cl_uint device_id = 0; device_id < num_devices; device_id++)
  {
    gpu_ctx_t *gpu_ctx = &gpu_ctxs[device_id];

    if (gpu_ctx->num_work == 0) continue;

    timer_set (&gpu_ctx->timer);

    const size_t size_block = gpu_ctx->num_work * sizeof (block_t);

    gc_clEnqueueWriteBuffer (gpu_ctx->command_queue, gpu_ctx->d_block, CL_FALSE, 0, size_block, gpu_ctx->h_block, 0, NULL, NULL);

    size_t global_work_size[3] = { gpu_ctx->num_work,    1, 1 };
    size_t local_work_size[3]  = { gpu_ctx->num_threads, 1, 1 };

    while (global_work_size[0] % local_work_size[0]) global_work_size[0]++;

    gc_clEnqueueNDRangeKernel (gpu_ctx->command_queue, gpu_ctx->kernel, 1, NULL, global_work_size, local_work_size, 0, NULL, NULL);

    gc_clFlush (gpu_ctx->command_queue);
  }
}

static void check_results (const cl_uint num_devices, gpu_ctx_t *gpu_ctxs)
{
  static uint64_t total = 0;

  for (cl_uint device_id = 0; device_id < num_devices; device_id++)
  {
    gpu_ctx_t *gpu_ctx = &gpu_ctxs[device_id];

    if (gpu_ctx->num_cached == 0) continue;

    const size_t size_results = gpu_ctx->num_threads * sizeof (uint32_t);

    gc_clEnqueueReadBuffer (gpu_ctx->command_queue, gpu_ctx->d_results, CL_TRUE, 0, size_results, gpu_ctx->h_results, 0, NULL, NULL);

    for (uint32_t thread = 0; thread < gpu_ctx->num_threads; thread++)
    {
      if (gpu_ctx->h_results[thread] != 0xffffffff)
      {
        uint32_t gid = gpu_ctx->h_results[thread];

        fprintf (stdout, "\nGPU #%2d: ALARM! Candidate number %u cracked the hash! Hex dump following:\n\n", device_id, gid);

        dump_hex (stdout, gpu_ctx->plains_buf[gid], gpu_ctx->plains_len[gid]);

        FILE *fd = fopen (POTFILE, "ab");

        if (fd) // ignore error
        {
          fprintf (fd, "\nGPU #%2d: ALARM! Candidate number %u cracked the hash! Hex dump following:\n\n", device_id, gid);

          dump_hex (fd, gpu_ctx->plains_buf[gid], gpu_ctx->plains_len[gid]);

          fclose (fd);
        }

        exit (0);
      }
    }

    const uint32_t num_cached = gpu_ctx->num_cached;

    total += num_cached;

    float ms;

    timer_get (gpu_ctx->timer, ms);

    if (ms == 0) continue;

    float speed = (num_cached / ms) * 1000;

    printf ("GPU #%2d: %u candidates in %u ms [%u/s] - Total = %llu...\n", device_id, num_cached, (uint32_t) ms, (uint32_t) speed, (long long unsigned int) total);
  }
}

static void md5_transform (uint32_t *W, const uint32_t len, uint32_t digest[4])
{
  // prepare some stuff

  digest[0] = MD5M_A;
  digest[1] = MD5M_B;
  digest[2] = MD5M_C;
  digest[3] = MD5M_D;

  uint32_t len_pad = (len + 0x3f) & ~0x3f;

  uint8_t *ptr = (uint8_t *) W;

  memset (ptr + len, 0, len_pad - len);

  ptr[len] = 0x80;

  W[((len_pad - 64) / 4) + 14] = len * 8;

  // loop that

  for (uint32_t i = len_pad; i >= 55; i -= 64, W += 16)
  {
    uint32_t a = digest[0];
    uint32_t b = digest[1];
    uint32_t c = digest[2];
    uint32_t d = digest[3];

    uint32_t tmp2;

    MD5_STEP (MD5_F , a, b, c, d, W[ 0], MD5C00, MD5S00);
    MD5_STEP (MD5_F , d, a, b, c, W[ 1], MD5C01, MD5S01);
    MD5_STEP (MD5_F , c, d, a, b, W[ 2], MD5C02, MD5S02);
    MD5_STEP (MD5_F , b, c, d, a, W[ 3], MD5C03, MD5S03);
    MD5_STEP (MD5_F , a, b, c, d, W[ 4], MD5C04, MD5S00);
    MD5_STEP (MD5_F , d, a, b, c, W[ 5], MD5C05, MD5S01);
    MD5_STEP (MD5_F , c, d, a, b, W[ 6], MD5C06, MD5S02);
    MD5_STEP (MD5_F , b, c, d, a, W[ 7], MD5C07, MD5S03);
    MD5_STEP (MD5_F , a, b, c, d, W[ 8], MD5C08, MD5S00);
    MD5_STEP (MD5_F , d, a, b, c, W[ 9], MD5C09, MD5S01);
    MD5_STEP (MD5_F , c, d, a, b, W[10], MD5C0a, MD5S02);
    MD5_STEP (MD5_F , b, c, d, a, W[11], MD5C0b, MD5S03);
    MD5_STEP (MD5_F , a, b, c, d, W[12], MD5C0c, MD5S00);
    MD5_STEP (MD5_F , d, a, b, c, W[13], MD5C0d, MD5S01);
    MD5_STEP (MD5_F , c, d, a, b, W[14], MD5C0e, MD5S02);
    MD5_STEP (MD5_F , b, c, d, a, W[15], MD5C0f, MD5S03);

    MD5_STEP (MD5_G , a, b, c, d, W[ 1], MD5C10, MD5S10);
    MD5_STEP (MD5_G , d, a, b, c, W[ 6], MD5C11, MD5S11);
    MD5_STEP (MD5_G , c, d, a, b, W[11], MD5C12, MD5S12);
    MD5_STEP (MD5_G , b, c, d, a, W[ 0], MD5C13, MD5S13);
    MD5_STEP (MD5_G , a, b, c, d, W[ 5], MD5C14, MD5S10);
    MD5_STEP (MD5_G , d, a, b, c, W[10], MD5C15, MD5S11);
    MD5_STEP (MD5_G , c, d, a, b, W[15], MD5C16, MD5S12);
    MD5_STEP (MD5_G , b, c, d, a, W[ 4], MD5C17, MD5S13);
    MD5_STEP (MD5_G , a, b, c, d, W[ 9], MD5C18, MD5S10);
    MD5_STEP (MD5_G , d, a, b, c, W[14], MD5C19, MD5S11);
    MD5_STEP (MD5_G , c, d, a, b, W[ 3], MD5C1a, MD5S12);
    MD5_STEP (MD5_G , b, c, d, a, W[ 8], MD5C1b, MD5S13);
    MD5_STEP (MD5_G , a, b, c, d, W[13], MD5C1c, MD5S10);
    MD5_STEP (MD5_G , d, a, b, c, W[ 2], MD5C1d, MD5S11);
    MD5_STEP (MD5_G , c, d, a, b, W[ 7], MD5C1e, MD5S12);
    MD5_STEP (MD5_G , b, c, d, a, W[12], MD5C1f, MD5S13);

    MD5_STEP (MD5_H1, a, b, c, d, W[ 5], MD5C20, MD5S20);
    MD5_STEP (MD5_H2, d, a, b, c, W[ 8], MD5C21, MD5S21);
    MD5_STEP (MD5_H1, c, d, a, b, W[11], MD5C22, MD5S22);
    MD5_STEP (MD5_H2, b, c, d, a, W[14], MD5C23, MD5S23);
    MD5_STEP (MD5_H1, a, b, c, d, W[ 1], MD5C24, MD5S20);
    MD5_STEP (MD5_H2, d, a, b, c, W[ 4], MD5C25, MD5S21);
    MD5_STEP (MD5_H1, c, d, a, b, W[ 7], MD5C26, MD5S22);
    MD5_STEP (MD5_H2, b, c, d, a, W[10], MD5C27, MD5S23);
    MD5_STEP (MD5_H1, a, b, c, d, W[13], MD5C28, MD5S20);
    MD5_STEP (MD5_H2, d, a, b, c, W[ 0], MD5C29, MD5S21);
    MD5_STEP (MD5_H1, c, d, a, b, W[ 3], MD5C2a, MD5S22);
    MD5_STEP (MD5_H2, b, c, d, a, W[ 6], MD5C2b, MD5S23);
    MD5_STEP (MD5_H1, a, b, c, d, W[ 9], MD5C2c, MD5S20);
    MD5_STEP (MD5_H2, d, a, b, c, W[12], MD5C2d, MD5S21);
    MD5_STEP (MD5_H1, c, d, a, b, W[15], MD5C2e, MD5S22);
    MD5_STEP (MD5_H2, b, c, d, a, W[ 2], MD5C2f, MD5S23);

    MD5_STEP (MD5_I , a, b, c, d, W[ 0], MD5C30, MD5S30);
    MD5_STEP (MD5_I , d, a, b, c, W[ 7], MD5C31, MD5S31);
    MD5_STEP (MD5_I , c, d, a, b, W[14], MD5C32, MD5S32);
    MD5_STEP (MD5_I , b, c, d, a, W[ 5], MD5C33, MD5S33);
    MD5_STEP (MD5_I , a, b, c, d, W[12], MD5C34, MD5S30);
    MD5_STEP (MD5_I , d, a, b, c, W[ 3], MD5C35, MD5S31);
    MD5_STEP (MD5_I , c, d, a, b, W[10], MD5C36, MD5S32);
    MD5_STEP (MD5_I , b, c, d, a, W[ 1], MD5C37, MD5S33);
    MD5_STEP (MD5_I , a, b, c, d, W[ 8], MD5C38, MD5S30);
    MD5_STEP (MD5_I , d, a, b, c, W[15], MD5C39, MD5S31);
    MD5_STEP (MD5_I , c, d, a, b, W[ 6], MD5C3a, MD5S32);
    MD5_STEP (MD5_I , b, c, d, a, W[13], MD5C3b, MD5S33);
    MD5_STEP (MD5_I , a, b, c, d, W[ 4], MD5C3c, MD5S30);
    MD5_STEP (MD5_I , d, a, b, c, W[11], MD5C3d, MD5S31);
    MD5_STEP (MD5_I , c, d, a, b, W[ 2], MD5C3e, MD5S32);
    MD5_STEP (MD5_I , b, c, d, a, W[ 9], MD5C3f, MD5S33);

    digest[0] += a;
    digest[1] += b;
    digest[2] += c;
    digest[3] += d;
  }
}

int main (int argc, char *argv[])
{
  uint64_t skip =  0;
  uint64_t left = -1;

  if (argc >= 2) skip = atoll (argv[1]);
  if (argc >= 3) left = atoll (argv[2]);

  printf ("Loading Kernel...\n");

  const char *filename = KERNEL_SRC;

  struct stat s;

  if (stat (filename, &s) == -1)
  {
    fprintf (stderr, "%s: %s in line %d\n", filename, strerror (errno), __LINE__);

    return (-1);
  }

  FILE *fp = fopen (filename, "rb");

  if (fp == NULL)
  {
    fprintf (stderr, "%s: %s in line %d\n", filename, strerror (errno), __LINE__);

    return (-1);
  }

  char *source_buf = (char *) malloc (s.st_size + 1);

  if (!fread (source_buf, sizeof (char), s.st_size, fp))
  {
    fprintf (stderr, "%s: %s in line %d\n", filename, strerror (errno), __LINE__);

    return (-1);
  }

  source_buf[s.st_size] = 0;

  fclose (fp);

  const char *sourceBuf[] = { source_buf };

  const size_t sourceLen[] = { s.st_size + 1 };

  printf ("Initializing OpenCL...\n");

  cl_platform_id platform;

  cl_uint num_devices = 0;

  cl_device_id devices[MAX_PLATFORM];

  gc_clGetPlatformIDs (1, &platform, NULL);

  gc_clGetDeviceIDs (platform, DEV_TYPE, MAX_PLATFORM, devices, &num_devices);

  gpu_ctx_t gpu_ctxs[MAX_GPU];

  memset (gpu_ctxs, 0, sizeof (gpu_ctxs));

  for (cl_uint device_id = 0; device_id < num_devices; device_id++)
  {
    cl_device_id device = devices[device_id];

    cl_context context = gc_clCreateContext (NULL, 1, &device, NULL, NULL);

    cl_program program = gc_clCreateProgramWithSource (context, 1, sourceBuf, sourceLen);

    gc_clBuildProgram (program, 1, &device, BUILD_OPTS, NULL, NULL);

    cl_kernel kernel = gc_clCreateKernel (program, KERNEL_NAME);

    cl_command_queue command_queue = gc_clCreateCommandQueue (context, device, 0);

    cl_uint max_compute_units;

    gc_clGetDeviceInfo (device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof (max_compute_units), &max_compute_units, NULL);

    char device_name[BUFSIZ];

    memset (device_name, 0, sizeof (device_name));

    gc_clGetDeviceInfo (device, CL_DEVICE_NAME, sizeof (device_name), &device_name, NULL);

    printf ("Found new device #%2d: %s, %u compute units\n", device_id, device_name, max_compute_units);

    const int num_threads  = GPU_THREADS;
    const int num_elements = max_compute_units * num_threads * GPU_ACCEL;

    /**
     * GPU memory
     */

    const size_t size_block   = num_elements * sizeof (block_t);
    const size_t size_results = num_threads  * sizeof (uint32_t);

    cl_mem d_block = gc_clCreateBuffer (context, CL_MEM_READ_ONLY, size_block, NULL);

    cl_mem d_results = gc_clCreateBuffer (context, CL_MEM_WRITE_ONLY, size_results, NULL);

    gc_clSetKernelArg (kernel, 0, sizeof (cl_mem), (void *) &d_block);
    gc_clSetKernelArg (kernel, 1, sizeof (cl_mem), (void *) &d_results);

    /**
     * Host memory
     */

    block_t *h_block = (block_t *) malloc (size_block);

    uint32_t *h_results = (uint32_t *) malloc (size_results);

    memset (h_results, 0xff, size_results);

    gc_clEnqueueWriteBuffer (command_queue, d_results, CL_TRUE, 0, size_results, h_results, 0, NULL, NULL);

    /**
     * Buffers for candidates
     */

    uint8_t **plains_buf = (uint8_t **) calloc (num_elements * VECT_SIZE, sizeof (uint8_t *));

    for (int i = 0; i < num_elements * VECT_SIZE; i++)
    {
      /* Agreed, this is not nice. But who cares nowadays? */

      plains_buf[i] = (uint8_t *) malloc (MAX_LINELEN);
    }

    size_t *plains_len = (size_t *) calloc (num_elements * VECT_SIZE, sizeof (size_t));

    gpu_ctx_t *gpu_ctx = &gpu_ctxs[device_id];

    gpu_ctx->context           = context;
    gpu_ctx->program           = program;
    gpu_ctx->kernel            = kernel;
    gpu_ctx->command_queue     = command_queue;
    gpu_ctx->max_compute_units = max_compute_units;
    gpu_ctx->d_block           = d_block;
    gpu_ctx->d_results         = d_results;
    gpu_ctx->h_block           = h_block;
    gpu_ctx->h_results         = h_results;
    gpu_ctx->num_threads       = num_threads;
    gpu_ctx->num_elements      = num_elements;
    gpu_ctx->plains_buf        = plains_buf;
    gpu_ctx->plains_len        = plains_len;
  }

  /* static salt */

  const uint8_t salt_buf[16] =
  {
    0x97, 0x48, 0x6C, 0xAA,
    0x22, 0x5F, 0xE8, 0x77,
    0xC0, 0x35, 0xCC, 0x03,
    0x73, 0x23, 0x6D, 0x51
  };

  const size_t salt_len = sizeof (salt_buf);

  /* main loop */

  printf ("Initialization done, accepting candidates from stdin...\n\n");

  cl_uint cur_device_id = 0;

  while (!feof (stdin))
  {
    /* Get new password candidate from stdin */

    uint8_t line_buf[MAX_LINELEN];

    int cur_c = 0;

    int prev_c = 0;

    size_t line_len = 0;

    for (size_t i = 0; i < MAX_LINELEN - 100; i++) // - 100 = we need some space for salt and padding
    {
      cur_c = getchar ();

      if (cur_c == EOF) break;

      if ((prev_c == '\n') && (cur_c == '\0'))
      {
        line_len--;

        break;
      }

      line_buf[line_len] = cur_c;

      line_len++;

      prev_c = cur_c;
    }

    /* chop \r if it exists for some reason (in case user used a dictionary) */

    if (line_len >= 2)
    {
      if ((prev_c == '\r') && (cur_c == '\0')) line_len -= 2;
    }

    /* skip empty lines */

    if (line_len == 0) continue;

    /* The following enables distributed computing / resume work */

    if (skip)
    {
      skip--;

      continue;
    }

    if (left)
    {
      left--;
    }
    else
    {
      break;
    }

    /* Append constant salt */

    memcpy (line_buf + line_len, salt_buf, salt_len);

    line_len += salt_len;

    /* Generate digest out of it */

    uint32_t digest[4];

    md5_transform ((uint32_t *) line_buf, (uint32_t) line_len, digest);

    /* Next garanteed free GPU */

    gpu_ctx_t *gpu_ctx = &gpu_ctxs[cur_device_id];

    /* Save original buffer in case it cracks it */

    memcpy (gpu_ctx->plains_buf[gpu_ctx->num_cached], line_buf, line_len - salt_len);

    gpu_ctx->plains_len[gpu_ctx->num_cached] = line_len - salt_len;

    /* Next garanteed free memory element on that GPU */

    const uint32_t element_div = gpu_ctx->num_cached / 4;
    const uint32_t element_mod = gpu_ctx->num_cached % 4;

    /* Copy new digest */

    gpu_ctx->h_block[element_div].A[element_mod] = digest[0];
    gpu_ctx->h_block[element_div].B[element_mod] = digest[1];
    gpu_ctx->h_block[element_div].C[element_mod] = digest[2];
    gpu_ctx->h_block[element_div].D[element_mod] = digest[3];

    gpu_ctx->num_cached++;

    /* If memory elements on that GPU are full, switch to the next GPU */

    if ((gpu_ctx->num_cached / VECT_SIZE) < gpu_ctx->num_elements) continue;

    cur_device_id++;

    /* If there is no more GPU left, run the calculation */

    if (cur_device_id < num_devices) continue;

    /* Fire! */

    calc_work (num_devices, gpu_ctxs);

    launch_kernel (num_devices, gpu_ctxs);

    /* Collecting data has a blocking effect */

    check_results (num_devices, gpu_ctxs);

    /* Reset buffer state */

    for (cl_uint device_id = 0; device_id < num_devices; device_id++)
    {
      gpu_ctx_t *gpu_ctx = &gpu_ctxs[device_id];

      gpu_ctx->num_cached = 0;
    }

    cur_device_id = 0;
  }

  /* Final calculation of leftovers */

  calc_work (num_devices, gpu_ctxs);

  launch_kernel (num_devices, gpu_ctxs);

  check_results (num_devices, gpu_ctxs);

  return -1;
}
