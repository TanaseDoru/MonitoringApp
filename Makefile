all: run

run: main.c
	gcc -o main main.c utils.c menuHandler.c processHandler.c showSpecs.c setup.c -lncurses