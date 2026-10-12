#
# Tomás Oliveira e Silva,  September 2026
#
# Arquiteturas de Alto Desempenho 2026/2027
#
# makefile for the practical class AAD_P05 (2026/2027)
#

clean:
	rm -f a.out hello one_to_one one_to_all
	rm -f one_to_one_deadlock one_to_one_evenodd one_to_one_sendrecv
	rm -f MandelbrotCountSeq MandelbrotCountMPI

hello:		hello_world_mpi.c
	mpicc -Wall -O2 hello_world_mpi.c -o hello

.PHONY:		run_hello
run_hello:	hello
	mpirun -np 4 ./hello

one_to_one:	one_to_one_communication_mpi.c
	mpicc -Wall -O2 one_to_one_communication_mpi.c -o one_to_one

#
# section 4: part 3 of one_to_one with MPI_Ssend() (deadlock) and the two fixes
#

one_to_one_deadlock:	one_to_one_communication_mpi.c
	mpicc -Wall -O2 -DPART3=0 one_to_one_communication_mpi.c -o one_to_one_deadlock

one_to_one_evenodd:	one_to_one_communication_mpi.c
	mpicc -Wall -O2 -DPART3=1 one_to_one_communication_mpi.c -o one_to_one_evenodd

one_to_one_sendrecv:	one_to_one_communication_mpi.c
	mpicc -Wall -O2 -DPART3=2 one_to_one_communication_mpi.c -o one_to_one_sendrecv

section4_all:	one_to_one_deadlock one_to_one_evenodd one_to_one_sendrecv

one_to_all:	one_to_all_communication_mpi.c
	mpicc -Wall -O2 one_to_all_communication_mpi.c -o one_to_all

#
# section 6: parallel MandelbrotCount with MPI
#

MandelbrotCountSeq:	MandelbrotCount.c
	cc -Wall -O2 MandelbrotCount.c -o MandelbrotCountSeq

MandelbrotCountMPI:	MandelbrotCountMPI.c
	mpicc -Wall -O2 MandelbrotCountMPI.c -o MandelbrotCountMPI

section6_all:	MandelbrotCountSeq MandelbrotCountMPI
