//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// CUDA hello world example, using NVIDIA extensions to the C++ language and nvcc (runtime API, high level)
//


#include <stdio.h>


//
// CPU code
//

void cpu_hello(void)
{
  printf("hello from the cpu\n");
}


//
// CUDA code, note the __global__
//

__global__ void cuda_hello(void)
{
  printf("  hello from cuda_hello block(%2u,%2u) thread(%2u,%2u)\n",blockIdx.x,blockIdx.y,threadIdx.x,threadIdx.y);
}


//
// main program (CPU code)
//

int main(void)
{
  //
  // call the cpu_hello function, making sure that the printf output is sent to stdout
  //
  cpu_hello();
  fflush(stdout);
  //
  // call the GPU code using a launch grid with 1 block, with 32 threads per block
  //
  printf("first cuda grid\n");
  fflush(stdout);
  cuda_hello<<<1,32>>>(); // n_blocks=1, n_threads_per_block=32 (x coordinates)
  //
  // make sure that the printf() calls done in the GPU code are sent to stdout
  //
  cudaDeviceSynchronize();
  //
  // call the GPU code using a launch grid with 8 block (4x2), with 32 threads per block (2x16)
  //
  printf("second cuda grid\n");
  dim3 grid(4,2,1);
  dim3 block(2,16,1);
  cuda_hello<<<grid,block>>>();
  cudaDeviceSynchronize();
  //
  // done
  //
  printf("done\n");
  fflush(stdout);
  cudaDeviceReset();
  return 0;
}
