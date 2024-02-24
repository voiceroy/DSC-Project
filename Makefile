server: server.c linkedlist.c | build
	gcc -o build/server server.c linkedlist.c -I.

client: client.c linkedlist.c | build
	gcc -o build/client client.c linkedlist.c -I.

build:
	mkdir build
