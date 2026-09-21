#
# Tomás Oliveira e Silva,  September 2026
#
# Arquiteturas de Alto Desempenho 2026/2027
#
# makefile for the practical class AAD_P03 (2026/2027)
#

clean:
	rm -f a.out create_and_join create_and_join.s lock_and_unlock reduction tasks
	rm -f MandelbrotCountSeq MandelbrotCountOMP

create_and_join:	create_and_join.c
	cc -Wall -O2 -fopenmp create_and_join.c -o create_and_join

create_and_join.s:	create_and_join.c
	cc -Wall -O2 -S -fverbose-asm -fopenmp create_and_join.c

lock_and_unlock:	lock_and_unlock.c
	cc -Wall -O2 -fopenmp lock_and_unlock.c -o lock_and_unlock

reduction:		reduction.c
	cc -Wall -O2 -fopenmp reduction.c -o reduction

tasks:			tasks.c
	cc -Wall -O2 -fopenmp tasks.c -o tasks

#
# section 6: parallel MandelbrotCount
#

MandelbrotCountSeq:	MandelbrotCount.c
	cc -Wall -O2 MandelbrotCount.c -o MandelbrotCountSeq

MandelbrotCountOMP:	MandelbrotCountOMP.c
	cc -Wall -O2 -fopenmp MandelbrotCountOMP.c -o MandelbrotCountOMP

section6_all:	MandelbrotCountSeq MandelbrotCountOMP
