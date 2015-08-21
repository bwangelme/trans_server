SRC := ./src/

.PHONY: server client

server:
	make server.o -j2 -C $(SRC)
	g++ -o server $(SRC)server.o

client:
	make client.o -j2 -C $(SRC)
	g++ -o client $(SRC)client.o
