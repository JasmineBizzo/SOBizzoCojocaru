master: ./src/master.c
	gcc -Wvla -Wextra -Werror ./src/master.c ./lib/ipc.o -o ./bin/master

atomo: ./src/atomo.c
	gcc -Wvla -Wextra -Werror ./src/atomo.c ./lib/ipc.o -o ./bin/atomo

attivatore: ./src/attivatore.c
	gcc -Wvla -Wextra -Werror ./src/attivatore.c ./lib/ipc.o -o ./bin/attivatore

alimentazione: ./src/alimentazione.c
	gcc -Wvla -Wextra -Werror ./src/alimentazione.c ./lib/ipc.o -o ./bin/alimentazione

	
all: ./src/master.c
	gcc -Wvla -Wextra -Werror ./src/master.c ./lib/ipc.o -o ./bin/master
	gcc -Wvla -Wextra -Werror ./src/atomo.c ./lib/ipc.o -o ./bin/atomo
	gcc -Wvla -Wextra -Werror ./src/attivatore.c ./lib/ipc.o -o ./bin/attivatore
	gcc -Wvla -Wextra -Werror ./src/alimentazione.c ./lib/ipc.o -o ./bin/alimentazione