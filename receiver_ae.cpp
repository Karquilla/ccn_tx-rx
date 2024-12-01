#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h>
#include <string>
#include <vector>
#include <random>
#include <sys/time.h>

// Function to generate a random number between begin and end
int getRand(int begin, int end) {
    std::random_device rd; // Seed for random number generator
    std::mt19937 gen(rd()); // Mersenne Twister random number generator
    std::uniform_int_distribution<> dis(begin, end); // Define range for uniform distribution
    return dis(gen); // Generate and return the random number
}

// Function to generate a random numeric string of random length (3 to 10 characters)
std::string generateMessage() {
    int length = getRand(3, 10); // Generate random length for the message
    std::string randMsg = ""; 
    for (int i = 0; i < length; i++) {
        randMsg += std::to_string(getRand(0, 9)); // Append a random digit to the string
    }
    return randMsg; // Return the generated message
}

void cleanup(int soc, const char *path) {
    close(soc);
    unlink(path);
}

int main() {
    const char *SOCKET_PATH = "./receiver_soc"; // Path to the receiver's socket file
    int soc; // Socket file descriptor
    sockaddr_un self, peer; // Structures for the receiver's address and peer's address
    socklen_t peer_len = sizeof(peer); // Length of the peer address structure

    self.sun_family = AF_UNIX; // Set the address family to UNIX domain
    std::strcpy(self.sun_path, SOCKET_PATH); // Copy the socket file path into the address structure

    // Create a datagram socket in the UNIX domain
    soc = socket(AF_UNIX, SOCK_DGRAM, 0);

    if (soc < 0) {
        perror("Socket creation failed"); // Print error message if socket creation fails
        return 1;
    }

    std::cout << "Socket created successfully.\n"; // Debug message to confirm socket creation

    // Bind the socket to the receiver's address
    if (bind(soc, (sockaddr*)&self, sizeof(self)) < 0) {
        perror("Bind failed"); // Print error message if bind fails
        cleanup(soc, SOCKET_PATH); // Clean up resources on failure
        return 1;
    }

    std::cout << "Socket bound to address.\n"; // Debug message to confirm successful binding

    // Loop to continuously wait for and handle incoming messages
    while (true) {
        peer_len = sizeof(peer); // Reset peer length for each iteration

        fd_set readfds; // File descriptor set for monitoring activity on the socket
        FD_ZERO(&readfds); // Clear the file descriptor set
        FD_SET(soc, &readfds); // Add the socket to the set

        struct timeval timeout; // Timeout structure for the select function
        timeout.tv_sec = 5; // Set timeout to 5 seconds
        timeout.tv_usec = 0; // Set microseconds to 0

        std::cout << "Waiting for activity...\n"; // Debug message indicating the program is waiting for activity

        // Monitor the socket for incoming activity
        int activity = select(soc + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0) {
            perror("select error"); // Print error message if select fails
            cleanup(soc, SOCKET_PATH); // Clean up resources on failure
            return 1;
        }

        if (activity == 0) {
            std::cout << "Timeout occurred! No data received in 5 seconds.\n"; // Debug message for timeout
            cleanup(soc, SOCKET_PATH); // Clean up resources after timeout
            return 1;
        }

        std::cout << "Activity detected!\n"; // Debug message indicating activity was detected

        uint8_t sizeNum; // Variable to store the size of the incoming message

        // Receive the size of the incoming message
        if (recvfrom(soc, &sizeNum, 1, 0, (sockaddr*)&peer, &peer_len) < 0) {
            perror("recvfrom failed for size"); // Print error message if size reception fails
            cleanup(soc, SOCKET_PATH); // Clean up resources on failure
            return 1;
        }

        char message[sizeNum + 1]; // Buffer to store the incoming message content

        // Receive the actual message content
        if (recvfrom(soc, message, sizeNum, 0, (sockaddr*)&peer, &peer_len) < 0) {
            perror("recvfrom failed for message"); // Print error message if message reception fails
            cleanup(soc, SOCKET_PATH); // Clean up resources on failure
            return 1;
        }

        message[sizeNum] = '\0'; // Null-terminate the received message

        std::string msg(message); // Convert the message to a string for processing

        // Check if the received message signals the end of transmission
        if (msg.substr(1) == "END") {
            std::cout << "End-of-transmission\n"; // Debug message indicating the end of transmission
            break; // Exit the loop
        }

        int seqNum = std::stoi(msg.substr(0, 1)); // Extract the sequence number from the message
        std::cout << "Received message " << seqNum << " : " << msg.substr(1) << "\n"; // Log the received message

        std::string reply = std::to_string(seqNum) + generateMessage(); // Generate a reply message
        uint8_t replySize = static_cast<uint8_t>(reply.length()); // Get the size of the reply message

        // Send the size of the reply message
        if (sendto(soc, &replySize, 1, 0, (sockaddr*)&peer, peer_len) < 0) {
            perror("sendto failed for size"); // Print error message if reply size sending fails
            cleanup(soc, SOCKET_PATH); // Clean up resources on failure
            return 1;
        }

        // Send the actual reply message content
        if (sendto(soc, reply.c_str(), reply.length(), 0, (sockaddr*)&peer, peer_len) < 0) {
            perror("sendto failed for message"); // Print error message if reply message sending fails
            cleanup(soc, SOCKET_PATH); // Clean up resources on failure
            return 1;
        }

        std::cout << "Sent reply " << reply.substr(0,1) << " : " << reply.substr(1) << "\n"; // Log the sent reply
    }

    cleanup(soc, SOCKET_PATH); // Clean up resources before exiting
    return 0;
}