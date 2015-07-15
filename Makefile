all: client server
client: client.c	
	gcc -Wall -lpthread scrlib.c client.c -o client
server: server.c	
	gcc -Wall -lpthread scrlib.c server.c -o server
.PHONY: clean
clean:
	rm client server
