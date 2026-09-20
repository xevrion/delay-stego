CXX := g++
CXXFLAGS := -O2 -std=c++17 -Wall -Wextra -pthread

all: sender receiver

sender: sender.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

receiver: receiver.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

run-receiver: receiver
	./receiver

run-sender: sender
	./sender

clean:
	rm -f sender receiver

.PHONY: all clean run-receiver run-sender
