#
# Tomás Oliveira e Silva,  September 2026
#
# Arquiteturas de Alto Desempenho 2026/2027
#
# makefile for the practical class AAD_P06 (2026/2027)
#

#
# CUDA installation directory
# (adapted: the Ubuntu nvidia-cuda-toolkit package installs CUDA in /usr; on WSL the real libcuda.so is in /usr/lib/wsl/lib)
#
CUDA_DIR = /usr
CUDA_LIB_DIR = /usr/lib/wsl/lib

#
# OpenCL library directory (adapted: on WSL there is no NVidia OpenCL, PoCL (OpenCL on the CPU) is used instead)
#
OPENCL_LIB_DIR = /usr/lib/x86_64-linux-gnu

#
# CUDA device architecture (sm_75 for a GeForce GTX 1660 Ti, sm_86 for a GeForce RTX 3050 Laptop GPU)
#
CUDA_ARCH = sm_86


#
# clean up
#
clean:
	rm -f a.out hello
	rm -f hello_cuda_driver_api hello_cuda_driver_api.cubin
	rm -f hello_open_cl
	rm -f median_filter_cuda MandelbrotSetMedian.pgm
	rm -f MandelbrotCountSeq MandelbrotCountCUDA


#
# compilation of the hello.cu program using the nvcc "high-level" compiler
#
hello:				hello.cu
	nvcc -arch=$(CUDA_ARCH) -O2 hello.cu -o hello


#
# compilation of the hello_cuda_driver_api program
#
hello_cuda_driver_api:		hello_cuda_driver_api.c hello_cuda_driver_api.cubin
	cc -Wall -O2 -std=c99 -I$(CUDA_DIR)/include hello_cuda_driver_api.c -o hello_cuda_driver_api -L$(CUDA_LIB_DIR) -lcuda

hello_cuda_driver_api.cubin:	hello_cuda_driver_api.cu
	nvcc -arch=$(CUDA_ARCH) --compiler-options -O2,-Wall -I$(CUDA_DIR)/include --cubin hello_cuda_driver_api.cu -o hello_cuda_driver_api.cubin


#
# compilation of the hello_open_cl program
# note: the hello_open_cl.cl is loaded at run time and compiled on the fly
#
hello_open_cl:			hello_open_cl.c
	cc -Wall -O2 -std=c99 -DCL_TARGET_OPENCL_VERSION=300 hello_open_cl.c -o hello_open_cl -L$(OPENCL_LIB_DIR) -lOpenCL


#
# compilation of the median_filter_cuda program
#
median_filter_cuda:		median_filter_cuda.cu
	nvcc -arch=$(CUDA_ARCH) -O2 median_filter_cuda.cu -o median_filter_cuda


#
# section 7: MandelbrotCount using CUDA
# (-fmad=false: do not merge a multiplication and an addition into a single fused multiply-add instruction, so that
#  the GPU computes exactly the same thing as the CPU; try also without it)
#
MandelbrotCountSeq:		MandelbrotCount.c
	cc -Wall -O2 MandelbrotCount.c -o MandelbrotCountSeq

MandelbrotCountCUDA:		MandelbrotCountCUDA.cu
	nvcc -arch=$(CUDA_ARCH) -O2 -fmad=false MandelbrotCountCUDA.cu -o MandelbrotCountCUDA

section7_all:			MandelbrotCountSeq MandelbrotCountCUDA
