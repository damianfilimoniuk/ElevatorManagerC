CC = gcc
CFLAGS = -Wall -Wextra -pthread

all: winda_zmienne winda_semafory

winda_zmienne: winda_zmienne.o winda_utils.o
	$(CC) $(CFLAGS) -o winda_zmienne winda_zmienne.o winda_utils.o

winda_semafory: winda_semafory.o winda_utils.o
	$(CC) $(CFLAGS) -o winda_semafory winda_semafory.o winda_utils.o

winda_zmienne.o: winda_zmienne.c winda_utils.h
	$(CC) $(CFLAGS) -c winda_zmienne.c

winda_semafory.o: winda_semafory.c winda_utils.h
	$(CC) $(CFLAGS) -c winda_semafory.c

winda_utils.o: winda_utils.c winda_utils.h
	$(CC) $(CFLAGS) -c winda_utils.c

clean:
	rm -f *.o winda_zmienne winda_semafory

run-zmienne: winda_zmienne
	./winda_zmienne 3 8 3 | python3 wizualizacja.py

run-semafory: winda_semafory
	./winda_semafory 3 8 3 | python3 wizualizacja.py
