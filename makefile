all: user server worker

user: client.cpp
	g++ -std=c++11 -o user client.cpp

server: server.cpp
	g++ -std=c++11 -o server server.cpp

worker: worker.cpp
	g++ -std=c++11 -o worker worker.cpp -lcrypt -pthread
