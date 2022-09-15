CXX=g++
CXXFLAGS=-std=c++17 -g -pedantic -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer
LDLIBS=-lpthread -lrt

# 0 for output in autograder, 1 for no output in autograder
OUT=1


SRCS=server.cpp client.cpp
DEPS=common.cpp RequestChannel.cpp FIFORequestChannel.cpp  MQRequestChannel.cpp  SHMRequestChannel.cpp
BINS=$(SRCS:%.cpp=%.exe)
OBJS=$(DEPS:%.cpp=%.o)


all: clean $(BINS)

%.o: %.cpp %.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.exe: %.cpp $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(patsubst %.exe,%,$@) $^ $(LDLIBS)


.PHONY: clean print-var test

clean:
	rm -f server client fifo* data*_* *.tst *.o *.csv received/*

print-var:
	echo $(OUT)

test: all
	chmod u+x pa4-tests.sh
	./pa4-tests.sh
