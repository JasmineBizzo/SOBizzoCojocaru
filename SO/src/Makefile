CC = gcc
#flags per la compilazione
CFLAGS = -std=c89 -Wpedantic
# target, ovvero nome dell'eseguibile che si intende produrre
TARGET = master
DEPS = utils.h ../lib/ipc.h
#object file necessari per produrre l'eseguibile
OBJ = master.o attivatore.o alimentazione.o atomo.o ../lib/ipc.o

# si sfrutta la regola implicita per la produzione dei file oggetto n OBJ
#all: master alimentazione atomo attivatore

# per produrre l'eseguibile "master" servono i file object
# master.o attivatore.o alimentazione.o atomo.o
#$(TARGET): $(OBJ)
#	$(CC) $(OBJ) $(CFLAGS) -o $(TARGET)

%.o: %.c $(DEPS)
		$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJ)
		$(CC) $(CFLAGS) -o $@ $<

#master: master.c utils.h
#	$(CC) $(CFLAGS) $^ -o $@

#attivatore: attivatore.c utils.h
#	$(CC) $(CFLAGS) $^ -o $@

#alimentazione: alimentazione.c utils.h
#	$(CC) $(CFLAGS) $^ -o $@

#atomo: atomo.c utils.h
#	$(CC) $(CFLAGS) $^ -o $@


run: $(TARGET)
	./$(TARGET)

clean:
	rm -f *.o $(TARGET) *~