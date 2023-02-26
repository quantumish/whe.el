build:	
	g++ -fPIC -c wheelmacs.cpp -std=c++17
	g++ -shared -o wheel.so wheelmacs.o xdo.o /usr/lib/x86_64-linux-gnu/libXtst.a

