//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/202R7
//
// hello world program in CUDA using the driver API --- low level, nvcc used only to compile the CUDA kernel
//
// what it does:
//   the CUDA kernel initializes an array with the string "Hello, world!"
//   each "thread" initializes only one array position
//

#include <stdio.h>
#include <stdlib.h>


//
// get the CU_CALL macro
//

#include "cuda_driver_api_util.h"


//
// main program
//

int main(void)
{
  int i;

  //
  // initialize the driver API interface
  //
  CU_CALL( cuInit , (0) );
  //
  // open the first CUDA device
  //
  int device_number;
  CUdevice cu_device;

  device_number = 0;
  CU_CALL( cuDeviceGet , (&cu_device,device_number) );
  //
  // get information about the CUDA device
  //
  char device_name[256];

  CU_CALL( cuDeviceGetName , (device_name,(int)sizeof(device_name) - 1,cu_device) );
  printf("CUDA code running on a %s (device %d)\n\n",device_name,device_number);
  //
  // create a context
  //
  CUcontext cu_context;

  CU_CALL( cuCtxCreate , (&cu_context,CU_CTX_SCHED_YIELD,cu_device) ); // CU_CTX_SCHED_SPIN may be slightly faster
  CU_CALL( cuCtxSetCacheConfig , (CU_FUNC_CACHE_PREFER_L1) );
  //
  // load a precompiled module (a module may have more than one kernel)
  //
  CUmodule cu_module;

  CU_CALL( cuModuleLoad , (&cu_module,"./hello_cuda_driver_api.cubin") );
  //
  // create a memory area in device memory where the "Hello, world!" string will be placed
  //
  char host_buffer[128];
  CUdeviceptr device_buffer;
  int buffer_size; // we use an int here because a device int is the same as a host int

  buffer_size = (int)sizeof(host_buffer);
  CU_CALL( cuMemAlloc , (&device_buffer,(size_t)buffer_size) );
  //
  // get the kernel function pointer
  //
  CUfunction cu_kernel;

  CU_CALL( cuModuleGetFunction, (&cu_kernel,cu_module,"hello_kernel") ); // unmangled function name, that's why we placed extern "C" in hello_cuda_driver_api.cu
  //
  // run the kernel (set its arguments first)
  //
  // we are launching sizeof(host_buffer) threads here; each thread initializes only one byte of the device_buffer array
  // the cuStreamSynchronize() call is not really needed, because cuMemcpyDtoH() will block until the kernel finished execution
  // launch parameters:
  //   cu_kernel is the kernel to launch
  //   1,1,1 means gridDimX=1, gridDimY=1, gridDimZ=1 (one block)
  //   buffer_size,1,1 means blockDimX=buffer_size, blockDimY=1, blockDimZ=1 (buffer-se threads per block, using only the x dimension)
  //   0 means 0 bytes of shared memory
  //   (CUstream)0 is the stream identifier (it is possible to use multiple execution streams, but we will not do so in AAD)
  //   &cu_params[0] is the pointer to the beginning of the array holding the pointers to the actual kernel arguments
  //
  void *cu_params[2];

  cu_params[0] = &device_buffer;
  cu_params[1] = &buffer_size;
  CU_CALL( cuLaunchKernel , (cu_kernel,1,1,1,buffer_size,1,1,0,(CUstream)0,&cu_params[0],NULL) );
  CU_CALL( cuStreamSynchronize , (0) );
  //
  // copy the buffer form device memory to CPU memory (copy only after the kernel has finished and block host execution until the copy is completed)
  //
  CU_CALL( cuMemcpyDtoH , (&host_buffer,device_buffer,(size_t)buffer_size) );
  //
  // print the host_buffer contents
  //
  for(i = 0;i < buffer_size;i++)
    printf("%3d %02X %c\n",i,(int)host_buffer[i] & 0xFF,((int)host_buffer[i] >= 32 && (int)host_buffer[i] < 127) ? host_buffer[i] : '_');
  //
  // clean up (optional)
  //
  CU_CALL( cuMemFree , (device_buffer) );
  CU_CALL( cuModuleUnload , (cu_module) );
  CU_CALL( cuCtxDestroy , (cu_context) );
  //
  // all done!
  //
  return 0;
}
