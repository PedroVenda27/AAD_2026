#
# Tomás Oliveira e Silva,  September 2026
#
# Arquiteturas de Alto Desempenho 2026/2027
#
# makefile for the practical class AAD_P02 (2026/2027)
#

clean:
	rm -f a.out create_and_join lock_and_unlock producer_consumer
	rm -f MandelbrotCountSeq MandelbrotCountVert MandelbrotCountHoriz MandelbrotCountCrit MandelbrotCountProdCons

create_and_join:	create_and_join.c
	cc -Wall -O2 -pthread create_and_join.c -o create_and_join

lock_and_unlock:	lock_and_unlock.c
	cc -Wall -O2 -pthread lock_and_unlock.c -o lock_and_unlock

producer_consumer:	producer_consumer.c
	cc -Wall -O2 -pthread producer_consumer.c -o producer_consumer

#
# section 7: parallel MandelbrotCount
#

MandelbrotCountSeq:	MandelbrotCount.c
	cc -Wall -O2 MandelbrotCount.c -o MandelbrotCountSeq

MandelbrotCountVert:	MandelbrotCountVert.c
	cc -Wall -O2 -pthread MandelbrotCountVert.c -o MandelbrotCountVert

MandelbrotCountHoriz:	MandelbrotCountHoriz.c
	cc -Wall -O2 -pthread MandelbrotCountHoriz.c -o MandelbrotCountHoriz

MandelbrotCountCrit:	MandelbrotCountCrit.c
	cc -Wall -O2 -pthread MandelbrotCountCrit.c -o MandelbrotCountCrit

MandelbrotCountProdCons:	MandelbrotCountProdCons.c
	cc -Wall -O2 -pthread MandelbrotCountProdCons.c -o MandelbrotCountProdCons

section7_all:	MandelbrotCountSeq MandelbrotCountVert MandelbrotCountHoriz MandelbrotCountCrit MandelbrotCountProdCons
