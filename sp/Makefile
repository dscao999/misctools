.PHONY:	all clean release

CFLAGS += -g -pthread
LDFLAGS = -pthread

all: sp

sp:	fprimes.o node.o sp.o
	$(LINK.o) $^ -lgmp -lnuma -o $@

clean:
	rm -f *.o
	rm -f sp

release: CFLAGS = -pthread -O2 -Wall -D_GNU_SOURCE --std=c99
release: LDFLAGS = -O -pthread

release: sp
