# Makefile for AUbatch

CC = gcc

AUbatch: auburnbatch.o command_parser.o textPrinter.o
	$(CC) -o AUbatch -lpthread auburnbatch.o command_parser.o textPrinter.o

textPrinter.o: textPrinter.c
	${CC} -c textPrinter.c

command_parser.o: command_parser.c
	${CC} -c command_parser.c

auburnbatch.o: auburnbatch.c
	${CC} -c auburnbatch.c
