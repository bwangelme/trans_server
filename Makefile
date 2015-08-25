SRC := ./src/

.PHONY: server client

server:
	make server.o -j2 -C $(SRC)
	g++ -o server $(SRC)server.o -lpthread

client:
	make client.o -j2 -C $(SRC)
	gcc -o client $(SRC)client.o -lpthread
