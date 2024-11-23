# Define the compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17

# Targets
all: sender receiver

# Compile and link sender
sender: sender_ae.cpp
	$(CXX) $(CXXFLAGS) -o sender sender_ae.cpp

# Compile and link receiver
receiver: receiver_ae.cpp
	$(CXX) $(CXXFLAGS) -o receiver receiver_ae.cpp

# Phony targets
.PHONY: clean run_sender run_receiver

# Clean the build
clean:
	rm -f sender receiver

# Run sender
run_sender: sender
	./sender test

# Run receiver
run_receiver: receiver
	./receiver