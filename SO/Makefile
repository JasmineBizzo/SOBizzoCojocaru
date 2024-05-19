master: master.c
	gcc -std=c89 -Wpedantic ./src/master.c ./lib/ipc.o -o ./src/master

atomo: atomo.c
	gcc -std=c89 -Wpedantic ./src/atomo.c ./lib/ipc.o -o ./src/atomo

attivatore: attivatore.c
	gcc -std=c89 -Wpedantic ./src/attivatore.c ./lib/ipc.o -o ./src/attivatore

alimentazione: alimentazione.c
	gcc -std=c89 -Wpedantic ./src/alimentazione.c ./lib/ipc.o -o ./src/alimentazione

	
all: ./SO/src/master.c
	gcc -std=c89 -Wpedantic ./src/master.c ./lib/ipc.o -o ./src/master
	gcc -std=c89 -Wpedantic ./src/atomo.c ./lib/ipc.o -o ./src/atomo
	gcc -std=c89 -Wpedantic ./src/attivatore.c ./lib/ipc.o -o ./src/attivatore
	gcc -std=c89 -Wpedantic ./src/alimentazione.c ./lib/ipc.o -o ./src/alimentazione
