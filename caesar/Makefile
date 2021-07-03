# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul 
PORT = 12345

# Adresa IP a serverului 
IP_SERVER = 127.0.0.1

all: server client

# Compileaza server.cpp
server: server.cpp

# Compileaza client.cpp
client: client.cpp

.PHONY: clean run_server run_client

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_client:
	./client ${IP_SERVER} ${PORT}

clean:
	rm -f server client
