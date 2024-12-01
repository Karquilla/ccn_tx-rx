# Define the compiler
CXX = g++

# Compiler flags
CXXFLAGS = -Wall -Wextra -std=c++17

# Targets
project: sender receiver

messenger: messenger_sender messenger_receiver

messenger_sender:
	$(CXX) $(CXXFLAGS) -o sender sender_f.cpp

messenger_receiver:
	$(CXX) $(CXXFLAGS) -o receiver receiver_f.cpp

sender: sender_ae.cpp
	$(CXX) $(CXXFLAGS) -o sender sender_ae.cpp

receiver: receiver_ae.cpp
	$(CXX) $(CXXFLAGS) -o receiver receiver_ae.cpp

# Phony targets
.PHONY: clean run_sender run_receiver

# Clean the build
clean:
	rm -f sender receiver receiver_soc sender_soc p1_sender p1_receiver p2_sender p2_receiver
