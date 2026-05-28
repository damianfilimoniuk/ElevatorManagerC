# Kompilator i flagi
CC = gcc
CFLAGS = -Wall -Wextra -pthread

# Domyślny cel 
all: winda_cond winda_sem

# Reguła budowania Wariantu 1
winda_cond: winda_cond.c winda_utils.c winda_utils.h
	$(CC) $(CFLAGS) -o winda_cond winda_cond.c winda_utils.c

# Reguła budowania Wariantu 2 
winda_sem: winda_sem.c winda_utils.c winda_utils.h
	$(CC) $(CFLAGS) -o winda_sem winda_sem.c winda_utils.c

clean:
	rm -f winda_cond winda_sem