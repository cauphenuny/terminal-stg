CXX = g++
CXXFLAGS = -Wall -std=c++11 -g
CPPFLAGS = 
LDFLAGS = -pthread

.PHONY:run-client run-server clean

all:server client

server:server.cpp common.h func.h constants.h server.h makefile
	$(CXX) $(CXXFLAGS)$(CPPFLAGS) server.cpp -o server $(LDFLAGS) -O3

client:client.cpp common.h func.h constants.h makefile
	$(CXX) $(CXXFLAGS)$(CPPFLAGS) client.cpp -o client $(LDFLAGS)

clean:
	rm server client

run-server:server client
	./server

run-client:server client
	./client
