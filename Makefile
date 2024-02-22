server: server.c linkedlist.c
	gcc -o build/server server.c linkedlist.c -I.

client: client.c linkedlist.c
	gcc -o build/client client.c linkedlist.c -I.
