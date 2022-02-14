CXX = g++
cc = gcc

CXXFLAGS = -Wall -Wextra -Wunused -pedantic -O1
LIB = -lm
BIN = launch-meet

INCLUDE_PATH = ./src/include
BIN_PATH= bin
SRC_S = src/single
SRC_M = src/multiple

# Generates binary in launch-meet/bin folder
build: launch-meet_single

# Generates binary and install in .local/bin
install: launch-meet_single
	mkdir -p "${HOME}/.local/bin"
	@cp -v $(BIN_PATH)/launch-meet "${HOME}/.local/bin/"
	chmod 776 "${HOME}/.local/bin/launch-meet"

# removes the program
uninstall:
	@rm -v "${HOME}/.local/bin/launch-meet"

# removes the program and its configurations
purge:
	@rm -v "${HOME}/.local/bin/launch-meet"
	@rm -rvf "${HOME}/.local/share/launch-meet"
	@rm -rvf "${HOME}/.cache/launch-meet"
	@rm -rvf "${HOME}/.config/launch-meet"

rm_cache:
	@rm -rvf "${HOME}/.cache/launch-meet"

reinstall: uninstall install

launch-meet_single: $(SRC_S)/launch-meet.cpp
	$(CXX) $(CXXFLAGS) -o $(BIN_PATH)/launch-meet $^

# install_mutiple: launch-meet_multiple

# launch-meet_multiple: one.o two.o
# 	$(CXX) $(CXXFLAGS) -o $@ $^
# one.o: launch-meet.cpp
# 	$(CXX) $(CXXFLAGS) -o $@ -c $^
# two.o: launch-meet.cpp
# 	$(CXX) $(CXXFLAGS) -o $@ -c $^

.PHONY: all install uninstall reinstall build rm_cache
