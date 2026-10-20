//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// 3x3 median filter example in CUDA for a 2049x2049 gray level image
//


#include <time.h>
#include <stdio.h>
#include <stdlib.h>


#define WIDTH   2049
#define HEIGHT  2049

//
// CUDA code
//

__global__ void median(unsigned int *gray_image_in,unsigned int *gray_image_out)
{
  int x = (int)threadIdx.x + (int)blockDim.x * (int)blockIdx.x; // x coordinate
  int y = (int)threadIdx.y + (int)blockDim.y * (int)blockIdx.y; // y coordinate
  //
  // do the work only when the coordinates are valid (we have to launch more threads because 2049 is not a multiple of 32...)
  //
  if(x < WIDTH && y < HEIGHT)
  {
    //
    // get the gray levels in a 3x3 square, centered at (x,y)
    //
    unsigned int g0,g1,g2,g3,g4,g5,g6,g7,g8;
#   define GET_GRAY_LEVEL(g,x,y)                                   \
    do /*  define macro to get the gray level at position (x,y) */ \
    {                                                              \
      int local_x = max(0,min(WIDTH  - 1,x));                      \
      int local_y = max(0,min(HEIGHT - 1,y));                      \
      g = gray_image_in[local_x + WIDTH * local_y];                \
    }                                                              \
    while(0)
    GET_GRAY_LEVEL(g0,x - 1,y - 1); GET_GRAY_LEVEL(g1,x,y - 1); GET_GRAY_LEVEL(g2,x + 1,y - 1);
    GET_GRAY_LEVEL(g3,x - 1,y    ); GET_GRAY_LEVEL(g4,x,y    ); GET_GRAY_LEVEL(g5,x + 1,y    );
    GET_GRAY_LEVEL(g6,x - 1,y + 1); GET_GRAY_LEVEL(g7,x,y + 1); GET_GRAY_LEVEL(g8,x + 1,y + 1);
#   undef GET_GRAY_LEVEL
    //
    // sort the gray levels; data for optimally sorting 9 integers from https://bertdobbelaere.github.io/sorting_networks.html
    //
#   define SORT2(u,v) /* sort g_u and g_v in place */ \
    do                                                \
    {                                                 \
      unsigned int lo = min(g ## u,g ## v);           \
      unsigned int hi = max(g ## u,g ## v);           \
      g ## u = lo;                                    \
      g ## v = hi;                                    \
    }                                                 \
    while(0)
    SORT2(0,3); SORT2(1,7); SORT2(2,5); SORT2(4,8);
    SORT2(0,7); SORT2(2,4); SORT2(3,8); SORT2(5,6);
    SORT2(0,2); SORT2(1,3); SORT2(4,5); SORT2(7,8);
    SORT2(1,4); SORT2(3,6); SORT2(5,7);
    SORT2(0,1); SORT2(2,4); SORT2(3,5); SORT2(6,8);
    SORT2(2,3); SORT2(4,5); SORT2(6,7);
    SORT2(1,2); SORT2(3,4); SORT2(5,6);
#   undef SORT2
    //
    // store the median in the output array
    //
    // it is not safe to overwrite gray_image_in[] because other threads may still need the original data, so we have to use another array
    //
    gray_image_out[x + WIDTH * y] = g4;
  }
}


//
// read and write a WIDTHxHEIGHT pgm image
//

static void read_pgm_file(const char *file_name,unsigned int image[HEIGHT * WIDTH])
{
  int width,height,i;
  FILE *fp;

  fp = fopen(file_name,"r");
  if(fp == NULL)
  {
    fprintf(stderr,"read_pgm_image: unable to open file %s\n",file_name);
    exit(1);
  }
  if(fscanf(fp,"P2 %d %d %d",&width,&height,&i) != 3 || width != WIDTH || height != HEIGHT || i != 255)
  {
    fclose(fp);
    fprintf(stderr,"read_pgm_image: bad header in file %s\n",file_name);
    exit(1);
  }
  for(i = 0;i < HEIGHT * WIDTH;i++)
    if(fscanf(fp,"%d",&image[i]) != 1 || image[i] > 255u)
    {
      fclose(fp);
      fprintf(stderr,"read_pgm_image: bad gray level at i=%d in file %s\n",i,file_name);
      exit(1);
    }
  fclose(fp);
}

static void write_pgm_file(const char *file_name,unsigned int image[HEIGHT * WIDTH])
{
  FILE *fp;
  int i;

  fp = fopen(file_name,"w");
  if(fp == NULL)
  {
    fprintf(stderr,"write_pgm_image: unable to create file %s\n",file_name);
    exit(1);
  }
  fprintf(fp,"P2 %d %d 255\n",WIDTH,HEIGHT);
  for(i = 0;i < HEIGHT * WIDTH;i++)
    fprintf(fp,"%u\n",image[i]);
  fclose(fp);
}


//
// main program (CPU code)
//

int main(int argc,char **argv)
{
  //
  // block size (adapted: given in the command line, to make it easy to try other block sizes; default 32x32)
  //
  int block_x = (argc > 2) ? atoi(argv[1]) : 32;
  int block_y = (argc > 2) ? atoi(argv[2]) : 32;
  if(block_x < 1 || block_y < 1 || block_x * block_y > 1024)
  {
    fprintf(stderr,"usage: %s [block_x block_y]  # block_x * block_y <= 1024\n",argv[0]);
    return 1;
  }
  //
  // allocate memory for the input and output images (accessible in the host and in the device)
  //
  unsigned int *image_in,*image_out;
  cudaMallocManaged((void **)&image_in,(size_t)(HEIGHT * WIDTH) * sizeof(int));
  cudaMallocManaged((void **)&image_out,(size_t)(HEIGHT * WIDTH) * sizeof(int));
  //
  // read image
  //
  read_pgm_file("MandelbrotSet.pgm",image_in);
  //
  // optional: prefetch the data in the device (will speeds memory reads in the device!)
  //           use nvprof to time median() 
  //
  // due to a nvprof bug, to profile this code use "nvprof --unified-memory-profiling off ./median_filter_cuda"
  //
# if 0
  int device = -1;
  cudaGetDevice(&device);
  cudaError_t e = cudaMemPrefetchAsync(image_in,(size_t)(HEIGHT * WIDTH) * sizeof(int),device);
  if(e != cudaSuccess) // adapted: on WSL/Windows prefetching is not supported (concurrentManagedAccess=0)
    printf("cudaMemPrefetchAsync: %s\n",cudaGetErrorString(e));
# endif
  //
  // call the GPU median kernel using a launch grid with block_x x block_y x 1 threads per block,
  //   and a grid with ceil(WIDTH/block_x) x ceil(HEIGHT/block_y) x 1 blocks
  //
  // adapted: nvprof does not support compute capability >= 8.0 (RTX 30xx), so the kernel is timed with CUDA events
  //
  cudaEvent_t start,stop;
  float kernel_ms;
  cudaEventCreate(&start);
  cudaEventCreate(&stop);
  dim3 grid((WIDTH + block_x - 1) / block_x,(HEIGHT + block_y - 1) / block_y,1);
  dim3 block(block_x,block_y,1);
  cudaEventRecord(start);
  median<<<grid,block>>>(image_in,image_out);
  cudaEventRecord(stop);
  //
  // adapted: wait for the kernel to finish before the host touches the managed memory (without this the host may read
  // pixels not yet computed; on WSL/Windows, where concurrentManagedAccess=0, the program crashes with a segmentation fault)
  //
  cudaDeviceSynchronize();
  cudaEventElapsedTime(&kernel_ms,start,stop);
  printf("median kernel (%dx%d blocks): %.3f ms (1st run, includes moving the images to the device)\n",block_x,block_y,kernel_ms);
  //
  // adapted: run it again, now with the data already in device memory, to get the time of the kernel alone
  //
  cudaEventRecord(start);
  median<<<grid,block>>>(image_in,image_out);
  cudaEventRecord(stop);
  cudaDeviceSynchronize();
  cudaEventElapsedTime(&kernel_ms,start,stop);
  printf("median kernel (%dx%d blocks): %.3f ms (2nd run, kernel only)\n",block_x,block_y,kernel_ms);
  //
  // write new image
  //
  write_pgm_file("MandelbrotSetMedian.pgm",image_out);
  //
  // done
  //
  cudaFree(image_in);
  cudaFree(image_out);
  cudaDeviceReset();
  printf("main: done\n");
  return 0;
}
