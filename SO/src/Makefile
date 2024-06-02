master: master.c
	gcc -Wvla -Wextra -Werror ./master.c ../lib/ipc.o -o ../bin/master

atomo: atomo.c
	gcc -Wvla -Wextra -Werror ./atomo.c ../lib/ipc.o -o ../bin/atomo

attivatore: attivatore.c
	gcc -Wvla -Wextra -Werror ./attivatore.c ../lib/ipc.o -o ../bin/attivatore

alimentazione: alimentazione.c
	gcc -Wvla -Wextra -Werror ./alimentazione.c ../lib/ipc.o -o ../bin/alimentazione

	
all: master.c
	gcc -Wvla -Wextra -Werror ./master.c ../lib/ipc.o -o ../bin/master
	gcc -Wvla -Wextra -Werror ./atomo.c ../lib/ipc.o -o ../bin/atomo
	gcc -Wvla -Wextra -Werror ./attivatore.c ../lib/ipc.o -o ../bin/attivatore
	gcc -Wvla -Wextra -Werror ./alimentazione.c ../lib/ipc.o -o ../bin/alimentazione