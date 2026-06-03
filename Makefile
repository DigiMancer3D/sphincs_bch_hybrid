CC = gcc
CFLAGS = -Wall -Wextra -O2 -Wno-format-truncation
LIBS = -loqs -ljansson -lcrypto -lm

TARGET = bch_pqc_hybrid_single

.PHONY: all clean prog1 prog2 all-progs

all: $(TARGET)

$(TARGET): bch_pqc_hybrid_single.c
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -f $(TARGET)

# Build prog1
prog1:
	$(MAKE) -C prog1 clean
	$(MAKE) -C prog1

# Build prog2
prog2:
	$(MAKE) -C prog2 clean
	$(MAKE) -C prog2

all-progs: prog1 prog2
