//
// Arquiteturas de Alto Desempenho 2026/2027 -- AAD_P6, section 7
//
// Mandelbrot set count, CUDA version (runtime API)
//
// usage: ./MandelbrotCountCUDA [float|double] [block_x block_y] [check]
//
//   float (default) or double: precision used in the computation; the guide asks for float, because the double
//                              precision performance of almost all graphics cards is very poor (try double to see it)
//   block_x block_y          : block size (default 32 x 8 = 256 threads)
//   check                    : also do the computation on the CPU (one core, same precision) and compare the counts
//
// each CUDA "thread" handles one point c of the 8193 x 8193 grid:
//   x coordinate of the thread -> im_idx (consecutive threads of a warp get consecutive values of c_im)
//   y coordinate of the thread -> re_idx
// the points of a block are counted with __syncthreads_count(), and each block adds its count to the global count
// with a single atomicAdd()
//

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define n_samples  8193
#define max_iter   256

static double wall_time(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME,&ts);
  return (double)ts.tv_sec + 1.0e-9 * (double)ts.tv_nsec;
}


//
// the iterations for one point (same code for the host and for the device, for float or for double)
//

template <typename T>
__host__ __device__ static inline int mandelbrot_point(int re_idx,int im_idx)
{
  const T c_re_min = (T)-2.05,c_re_max = (T)+0.55;
  const T c_im_min = (T)-1.30,c_im_max = (T)+1.30;
  T c_re = c_re_min + (T)re_idx * ((c_re_max - c_re_min) / (T)n_samples);
  T c_im = c_im_min + (T)im_idx * ((c_im_max - c_im_min) / (T)n_samples);
  T z_re = (T)0,z_im = (T)0;
  int n_iter = 0;

  while(n_iter < max_iter && z_re * z_re + z_im * z_im <= (T)4)
  {
    T tmp = z_re * z_re - z_im * z_im;
    z_im = (T)2 * z_re * z_im + c_im;
    z_re = tmp + c_re;
    n_iter++;
  }
  return (n_iter < max_iter) ? 1 : 0;
}


//
// CUDA kernel: one thread per point
//

template <typename T>
__global__ void mandelbrot_kernel(unsigned int *count)
{
  int im_idx = (int)threadIdx.x + (int)blockDim.x * (int)blockIdx.x;
  int re_idx = (int)threadIdx.y + (int)blockDim.y * (int)blockIdx.y;
  int in_set = 0;

  if(re_idx < n_samples && im_idx < n_samples) // the grid is larger than needed, because 8193 is not a multiple of the block size
    in_set = mandelbrot_point<T>(re_idx,im_idx);
  // count the points of the block (all threads of the block must call __syncthreads_count(), even those outside the grid)
  int block_count = __syncthreads_count(in_set);
  if(threadIdx.x == 0 && threadIdx.y == 0)
    atomicAdd(count,(unsigned int)block_count);
}


//
// run the kernel and return the count; the time of the kernel (in seconds) is placed in *kernel_time
//

template <typename T>
static unsigned int run_on_gpu(int block_x,int block_y,double *kernel_time)
{
  unsigned int *device_count,host_count = 0u;
  cudaEvent_t start,stop;
  float ms;
  cudaError_t e;

  cudaMalloc((void **)&device_count,sizeof(unsigned int));
  cudaMemset(device_count,0,sizeof(unsigned int));
  cudaEventCreate(&start);
  cudaEventCreate(&stop);
  dim3 grid((n_samples + block_x - 1) / block_x,(n_samples + block_y - 1) / block_y,1);
  dim3 block(block_x,block_y,1);
  cudaEventRecord(start);
  mandelbrot_kernel<T><<<grid,block>>>(device_count);
  cudaEventRecord(stop);
  e = cudaDeviceSynchronize();
  if(e == cudaSuccess)
    e = cudaGetLastError();
  if(e != cudaSuccess)
  {
    fprintf(stderr,"CUDA error: %s\n",cudaGetErrorString(e));
    exit(1);
  }
  cudaEventElapsedTime(&ms,start,stop);
  *kernel_time = 1.0e-3 * (double)ms;
  cudaMemcpy(&host_count,device_count,sizeof(unsigned int),cudaMemcpyDeviceToHost);
  cudaEventDestroy(start);
  cudaEventDestroy(stop);
  cudaFree(device_count);
  return host_count;
}


//
// the same computation on the CPU (one core), for comparison
//

template <typename T>
static unsigned int run_on_cpu(double *cpu_time)
{
  unsigned int count = 0u;
  double t0 = wall_time();

  for(int re_idx = 0;re_idx < n_samples;re_idx++)
    for(int im_idx = 0;im_idx < n_samples;im_idx++)
      count += (unsigned int)mandelbrot_point<T>(re_idx,im_idx);
  *cpu_time = wall_time() - t0;
  return count;
}


int main(int argc,char **argv)
{
  int use_double = 0,block_x = 32,block_y = 8,check = 0;
  unsigned int gpu_count,cpu_count;
  double gpu_time,cpu_time;
  const char *precision;

  //
  // command line stuff
  //
  for(int i = 1;i < argc;i++)
    if(strcmp(argv[i],"double") == 0)
      use_double = 1;
    else if(strcmp(argv[i],"float") == 0)
      use_double = 0;
    else if(strcmp(argv[i],"check") == 0)
      check = 1;
    else if(i + 1 < argc)
    {
      block_x = atoi(argv[i]);
      block_y = atoi(argv[++i]);
    }
  if(block_x < 1 || block_y < 1 || block_x * block_y > 1024)
  {
    fprintf(stderr,"usage: %s [float|double] [block_x block_y] [check]  # block_x * block_y <= 1024\n",argv[0]);
    return 1;
  }
  precision = (use_double != 0) ? "double" : "float";
  //
  // warm up the GPU (the first CUDA call creates the CUDA context, which takes some time)
  //
  cudaFree(0);
  //
  // GPU computation
  //
  gpu_count = (use_double != 0) ? run_on_gpu<double>(block_x,block_y,&gpu_time) : run_on_gpu<float>(block_x,block_y,&gpu_time);
  printf("%u -- %7.3f (GPU, %s, %dx%d blocks)\n",gpu_count,gpu_time,precision,block_x,block_y);
  //
  // optional CPU computation, with the same precision
  //
  if(check != 0)
  {
    cpu_count = (use_double != 0) ? run_on_cpu<double>(&cpu_time) : run_on_cpu<float>(&cpu_time);
    printf("%u -- %7.3f (CPU, %s, one core)\n",cpu_count,cpu_time,precision);
    printf("%s, GPU is %.1f times faster\n",(gpu_count == cpu_count) ? "same count" : "DIFFERENT counts",cpu_time / gpu_time);
  }
  cudaDeviceReset();
  return 0;
}
