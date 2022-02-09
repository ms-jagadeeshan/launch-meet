CXX = g++
cc = gcc

CXXFLAGS = -Wall -Wextra -Wunused -pedantic
LIB = -lm
BIN = launch-meet

INCLUDE_PATH = ./src/include
BIN_PATH= ./bin
SRC_S = ./src/single
SRC_M = ./src/multiple

install: launch-meet_single

launch-meet_single: $(SRC_S)/launch-meet.cpp
	$(CXX) $(CXXFLAGS) -o $(BIN_PATH)/launch-meet $^
	@cp -v $(BIN_PATH)/launch-meet "${HOME}/.local/bin/launch-meet"

install_mutiple: launch-meet_multiple

launch-meet_multiple: one.o two.o
	$(CXX) $(CXXFLAGS) -o $@ $^
one.o: launch-meet.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $^
two.o: launch-meet.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $^
clean:
	rm "${HOME}/.local/bin/launch-meet"
	rm -rf "${HOME}/.local/share/launch-meet"
	rm -rf "${HOME}/.cache/launch-meet"
	rm -rf "${HOME}/.config/launch-meet"




.PHONY: all clean install
