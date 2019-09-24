capture: main.cpp tcpserver.cpp
	g++ -o camcap main.cpp tcpserver.cpp -lpthread -std=c++11

clean:
	rm camcap
