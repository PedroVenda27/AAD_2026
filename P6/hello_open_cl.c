//
// Tomás Oliveira e Silva,  September 2026
//
// Arquiteturas de Alto Desempenho 2026/2027
//
// hello world program in OpenCL --- low level
//
// what it does:
//   the OpenCL kernel initializes an array with the string "Hello, world!"
//   each "thread" initializes only one array position
//

#include <stdio.h>
#include <stdlib.h>


//
// get the CL_CALL iand CL_CALL_ALT macros
//

#include "open_cl_util.h"


//
// main program
//

int main(void)
{
  int i;

  //
  // read the OpenCL kernel source code (this could be a string in our source code, but it is easier during code development to read it from a file)
  //
  char open_cl_source_code[8192];
  size_t open_cl_source_code_size;
  FILE *fp;

  fp = fopen("hello_open_cl.cl","r");
  if(fp == NULL)
  {
    perror("fopen()");
    exit(1);
  }
  open_cl_source_code_size = fread((void *)&open_cl_source_code[0],sizeof(char),sizeof(open_cl_source_code),fp);
  if(open_cl_source_code_size < (size_t)1 || open_cl_source_code_size >= sizeof(open_cl_source_code))
  {
    fprintf(stderr,"fread(): the OpenCL kernel code is either too small or too large\n");
    exit(1);
  }
  fclose(fp);
  //
  // get the first OpenCL platform ID
  //
  cl_uint num_platforms;
  cl_platform_id platform_id[1];

  CL_CALL( clGetPlatformIDs , (1,&platform_id[0],&num_platforms) );
  if(num_platforms < 1)
  {
    fprintf(stderr,"No OpenCL platform\n");
    exit(1);
  }
  if(num_platforms > 1)
    fprintf(stderr,"Warning: more than one OpenCL platform found (using the first one)\n");
  //
  // get information about the OpenCL platform (not truly needed, but this information may be useful)
  //
  char info_data[256];

  printf("OpenCL platform information\n");
  CL_CALL( clGetPlatformInfo , (platform_id[0],CL_PLATFORM_PROFILE,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  profile ................... %s\n",info_data);
  CL_CALL( clGetPlatformInfo , (platform_id[0],CL_PLATFORM_VERSION,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  version ................... %s\n",info_data);
  CL_CALL( clGetPlatformInfo , (platform_id[0],CL_PLATFORM_NAME,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  name ...................... %s\n",info_data);
  CL_CALL( clGetPlatformInfo , (platform_id[0],CL_PLATFORM_VENDOR,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  vendor .................... %s\n",info_data);
#if 0
  CL_CALL( clGetPlatformInfo , (platform_id[0],CL_PLATFORM_EXTENSIONS,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  extensions ................ %s\n",info_data);
#endif
  //
  // get information about the first OpenCL device (use CL_DEVICE_TYPE_CPU or CL_DEVICE_TYPE_GPU to force a specific device type)
  //
  cl_uint num_devices;
  cl_device_id device_id[1];

  CL_CALL( clGetDeviceIDs , (platform_id[0],CL_DEVICE_TYPE_DEFAULT,1,&device_id[0],&num_devices) );
  if(num_devices < 1)
  {
    fprintf(stderr,"No OpenCL device\n");
    exit(1);
  }
  if(num_devices > 1)
    fprintf(stderr,"Warning: more than one OpenCL device found (using the first one)\n");
  //
  // get information about the OpenCL device we have chosen (not truly needed, but this information is useful)
  //
  cl_device_type device_type;
  cl_uint n_compute_units,n_dimensions;
  size_t work_group_limits[4],local_index_limits[4]; // 4 is more than enough for current hardware
  size_t max_local_size;
  cl_ulong device_mem_size,device_global_cache_size,device_local_mem_size;

  printf("OpenCL device information\n");
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_NAME,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  name ...................... %s\n",info_data);
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_VENDOR,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  vendor .................... %s\n",info_data);
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DRIVER_VERSION,sizeof(info_data),(void *)&info_data[0],NULL) );
  printf("  driver version ............ %s\n",info_data);
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_TYPE,sizeof(device_type),(void *)&device_type,NULL) );
  printf("  type ...................... 0x%08X\n",(int)device_type);
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_MAX_COMPUTE_UNITS,sizeof(n_compute_units),(void *)&n_compute_units,NULL) );
  printf("  number of compute units ... %u\n",(unsigned int)n_compute_units);
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,sizeof(n_dimensions),(void *)&n_dimensions,NULL) );
  printf("  number of indices ......... %u%s\n",(unsigned int)n_dimensions,((int)n_dimensions > 3) ? " (more than three!)" : "");
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_MAX_WORK_ITEM_SIZES,sizeof(work_group_limits),(void *)&work_group_limits[0],NULL) );
  printf("  work group limits .........");
  for(i = 0;i < (int)n_dimensions;i++)
    printf(" %d",(int)work_group_limits[i]);
  printf(" # N.B. these are not the total limits!\n");
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_MAX_WORK_GROUP_SIZE,sizeof(local_index_limits),(void *)&local_index_limits[0],NULL) );
  printf("  local index limits ........");
  for(i = 0;i < (int)n_dimensions;i++)
    printf(" %d",(int)local_index_limits[i]);
  printf("\n");
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_MAX_WORK_GROUP_SIZE,sizeof(max_local_size),(void *)&max_local_size,NULL) );
  printf("  max local threads ......... %u\n",(unsigned int)max_local_size);
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_GLOBAL_MEM_SIZE,sizeof(device_mem_size),(void *)&device_mem_size,NULL) );
  printf("  device memory ............. %.3fGiB\n",(double)device_mem_size / (double)(1 << 30));
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_GLOBAL_MEM_CACHE_SIZE,sizeof(device_global_cache_size),(void *)&device_global_cache_size,NULL) );
  printf("  cache memory .............. %.3fKiB\n",(double)device_global_cache_size / (double)(1 << 10));
  CL_CALL( clGetDeviceInfo , (device_id[0],CL_DEVICE_LOCAL_MEM_SIZE,sizeof(device_local_mem_size),(void *)&device_local_mem_size,NULL) );
  printf("  local memory .............. %.3fKiB\n",(double)device_local_mem_size / (double)(1 << 10));
  //
  // create an OpenCL context
  //
  cl_context context;

  CL_CALL_ALT( context = clCreateContext , (NULL,1,&device_id[0],NULL,NULL,&e) );
  //
  // create an OpenCL command queue
  //
  cl_command_queue command_queue;

//CL_CALL_ALT( command_queue = clCreateCommandQueueWithProperties, (context,device_id[0],NULL,&e) );
  CL_CALL_ALT( command_queue = clCreateCommandQueue, (context,device_id[0],0,&e) );
  //
  // create a memory area in device memory where the "Hello, world!" string will be placed
  //
  char host_buffer[128];
  cl_mem device_buffer;
  int buffer_size;

  buffer_size = (int)sizeof(host_buffer);
  CL_CALL_ALT( device_buffer = clCreateBuffer, (context,CL_MEM_READ_WRITE,(size_t)buffer_size,NULL,&e) );
  //
  // transfer the OpenCL code to the OpenCL context
  //
  char *program_lines[1];
  size_t program_line_lengths[1];
  cl_program program;

  program_lines[0] = &open_cl_source_code[0];
  program_line_lengths[0] = open_cl_source_code_size;
  CL_CALL_ALT( program = clCreateProgramWithSource, (context,1,(const char **)&program_lines[0],&program_line_lengths[0],&e) );
  //
  // compile the OpenCL code and get the hello_kernel() handle
  //
  cl_kernel kernel;
  size_t simd_width,max_group_size,compiled_group_size[3];
  char build_log[1024];

  CL_CALL( clBuildProgram , (program,1,&device_id[0],NULL,NULL,NULL) );
  CL_CALL_ALT( kernel = clCreateKernel , (program,"hello_kernel",&e) );
  printf("kernel information\n");
  CL_CALL( clGetKernelWorkGroupInfo , (kernel,device_id[0],CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,sizeof(simd_width),(void *)&simd_width,NULL) );
  printf("  simd width ................ %d\n",(int)simd_width);
  CL_CALL( clGetKernelWorkGroupInfo , (kernel,device_id[0],CL_KERNEL_WORK_GROUP_SIZE,sizeof(max_group_size),(void *)&max_group_size,NULL) );
  printf("  max group size ............ %d\n",(int)max_group_size);
  CL_CALL( clGetKernelWorkGroupInfo , (kernel,device_id[0],CL_KERNEL_COMPILE_WORK_GROUP_SIZE,sizeof(compiled_group_size),(void *)&compiled_group_size[0],NULL) );
  printf("  compiled group size .......");
  for(i = 0;i < 3;i++)
    printf(" %d",(int)compiled_group_size[i]);
  printf("\n");
  CL_CALL( clGetProgramBuildInfo , (program,device_id[0],CL_PROGRAM_BUILD_LOG,sizeof(build_log),(void *)&build_log[0],NULL) );
  printf("  build log ................. %s\n",build_log);
  //
  // run the kernel (set its arguments and launch grid first)
  //
  // we are launching sizeof(host_buffer) threads here; each thread initializes only one byte of the device_buffer array
  // as we will be launching a single kernel, there is no need to specify the events this kernel has to wait for
  // total_work_size[0] equal to local_work_size[0] means that we will be launching only one work group (equivalent to a CUDA block)
  //
  size_t total_work_size[1],local_work_size[1]; // number of threads (we will one only one dimension, so the arrays have 1 element each)
  cl_event hello_kernel_done[1];

  CL_CALL( clSetKernelArg , (kernel,0,sizeof(cl_mem),(void *)&device_buffer) );
  CL_CALL( clSetKernelArg , (kernel,1,sizeof(int),&buffer_size) );
  total_work_size[0] = (size_t)buffer_size; // the total number of threads (one dimension)
  local_work_size[0] = (size_t)buffer_size; // the number of threads in each work group (this should match the value of the work_group_size_hint in the .cl file)
  CL_CALL( clEnqueueNDRangeKernel , (command_queue,kernel,1,NULL,&total_work_size[0],&local_work_size[0],0,NULL,&hello_kernel_done[0]) );
  //
  // copy the buffer form device memory to CPU memory (copy only after the kernel has finished and block host execution until the copy is completed)
  //
  CL_CALL( clEnqueueReadBuffer , (command_queue,device_buffer,CL_TRUE,0,(size_t)buffer_size,(void *)host_buffer,1,&hello_kernel_done[0],NULL) );
  //
  // print the host_buffer contents
  //
  for(i = 0;i < buffer_size;i++)
    printf("%3d %02X %c\n",i,(int)host_buffer[i] & 0xFF,((int)host_buffer[i] >= 32 && (int)host_buffer[i] < 127) ? host_buffer[i] : '_');
  //
  // clean up (optional)
  //
  CL_CALL( clFlush , (command_queue) );
  CL_CALL( clFinish , (command_queue) );
  CL_CALL( clReleaseKernel , (kernel) );
  CL_CALL( clReleaseProgram , (program) );
  CL_CALL( clReleaseMemObject , (device_buffer) );
  CL_CALL( clReleaseCommandQueue , (command_queue) );
  CL_CALL( clReleaseContext , (context) );
  //
  // all done!
  //
  return 0;
}
