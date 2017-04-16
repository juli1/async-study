CC=/usr/bin/gcc
NREQUESTS=1

all: sync epoll

sync:
	$(CC) -DNREQUESTS=$(NREQUESTS) -o sync -lpthread sync.c

epoll:
	$(CC) -DNREQUESTS=$(NREQUESTS) -o epoll -lrt epoll.c


clean:
	rm -f *~ epoll sync
