.PHONY:run-client run-server clean tmp

all:server client

server:server.cpp common.h func.h constants.h
	g++ -Wall -std=c++11 server.cpp -o server -lpthread -ggdb

client:client.cpp common.h func.h constants.h
	g++ -Wall -std=c++11 client.cpp -o client -lpthread -ggdb

clean:
	rm server client

run-server:server client
	./server

run-client:server client
	./client

tmp:
	echo $(CFILES)
	echo $(filter,./front/front-main.c,$(CFILES))
