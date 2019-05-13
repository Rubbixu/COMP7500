# Makefile for pwordcount

CC = gcc

pwordcount: pwordcount.o wordcount.o inputBuff.o
	$(CC) -o pwordcount pwordcount.o wordcount.o inputBuff.o

pwordcount.o: pwordcount.c
	${CC} -c pwordcount.c

wordcount.o: wordcount.c
	${CC} -c wordcount.c

inputBuff.o: inputBuff.c
	${CC} -c inputBuff.c
