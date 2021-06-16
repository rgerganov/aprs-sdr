CXXFLAGS = -O2 -Wall -Iinclude -std=c++11

all: aprs

aprs: aprs.cpp ax25.cpp dsp.cpp

clean:
	rm -f aprs *.o
