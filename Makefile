all: pclient pserver

pclient: client.c
	gcc  -g -pthread client.c -o pclient

pserver: server.c
	gcc  -g -pthread server.c -o pserver

clean:
	-rm -f *.o *.out
	-rm -f pclient pserver
