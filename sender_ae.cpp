#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <random>

// Function to generate a random number between begin and end
int getRand(int begin, int end) {
    std::random_device rd; // Used to seed the random number generator
    std::mt19937 gen(rd()); // Standard random number generator
    std::uniform_int_distribution<> dis(begin, end); // Distribution range
    return dis(gen); // Generate random number
}

// Function to generate a random numeric string of random length (3 to 10 characters)
std::string generateMessage() {
    int length = getRand(3, 10); // Get random length for the message
    std::string randMsg = "";
    for (int i = 0; i < length; i++) {
        randMsg += std::to_string(getRand(0, 9)); // Append random digits
    }
    return randMsg; // Return the generated message
}

void cleanup(int soc, const char *path) {
    close(soc);
    unlink(path); 
}

// Function to send a message with up to 3 retries
bool sendWithRetry(int soc, const void *message, size_t length, int flags, const sockaddr *receiver_addr, socklen_t addrlen) {
    int retries = 2; // Number of retries
    while (retries >= 0) {
        // Try sending the message
        if (sendto(soc, message, length, flags, receiver_addr, addrlen) >= 0) {
            return true; // Success
        }
        perror("sendto failed");
        retries--; // Decrease retry count
        sleep(1); 
    }
    return false; // Return false if all retries fail
}

int main() {
    // File paths for the sender and receiver sockets
    const char* SENDER_SOCKET_PATH = "./sender_soc";
    const char* RECEIVER_SOCKET_PATH = "./receiver_soc";
    sockaddr_un sender_addr, receiver_addr;

    // Create a socket for the sender
    int soc = socket(AF_UNIX, SOCK_DGRAM, 0); // Create a datagram socket
    // Check for errors
    if (soc < 0) { 
        perror("Socket creation failed");
        return 1; 
    }

    // Bind the sender socket to a unique file path
    sender_addr.sun_family = AF_UNIX; // Set socket family to UNIX
    std::strcpy(sender_addr.sun_path, SENDER_SOCKET_PATH); // Set file path
    if (bind(soc, (sockaddr*)&sender_addr, sizeof(sender_addr)) < 0) { // Bind the socket
        perror("Bind failed for sender");
        cleanup(soc, SENDER_SOCKET_PATH); // Clean up on failure
        return 1; 
    }

    // Set up the receiver's address
    receiver_addr.sun_family = AF_UNIX; // Set socket family to UNIX
    std::strcpy(receiver_addr.sun_path, RECEIVER_SOCKET_PATH); // Set file path

    // Generate random messages to send
    std::vector<std::string> data; // Vector to store messages
    int seqNum = 0; // Sequence number for messages
    int messageCount = getRand(1, 5); // Random number of messages to send

    // Generate and store each message
    for (int i = 0; i < messageCount; i++) {
        data.push_back(generateMessage()); 
    }
    data.push_back("END");

    // Send each message to the receiver
    for (auto &message : data) {
        message = std::to_string(seqNum) + message; // Add sequence number to the message
        uint8_t sizeOfMessage = static_cast<uint8_t>(message.length()); // Get the size of the message

        // Send the size of the message
        if (!sendWithRetry(soc, &sizeOfMessage, 1, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr))) {
            cleanup(soc, SENDER_SOCKET_PATH); 
            return 1;
        }
        sleep(1);

        // Send the actual message content
        if (!sendWithRetry(soc, message.c_str(), message.length(), 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr))) {
            cleanup(soc, SENDER_SOCKET_PATH); 
            return 1;
        }
        sleep(1); // Wait a moment after sending

        // Check if it's the end message
        if (message.substr(1) == "END") { 
            break;
        }

        std::cout << "Sent message " << seqNum << " : " << message.substr(1) << "\n";

        // Receive a reply from the receiver
        uint8_t replySize; // Variable to store the size of the reply
        sockaddr_un peer_addr; // Address of the peer
        socklen_t peer_len = sizeof(peer_addr); // Length of the peer address

        // Receive the size of the reply
        if (recvfrom(soc, &replySize, 1, 0, (sockaddr*)&peer_addr, &peer_len) < 0) {
            perror("recvfrom failed for size");
            cleanup(soc, SENDER_SOCKET_PATH);
            return 1;
        }

        // Receive the actual reply content
        char reply[replySize + 1]; // Buffer for the reply
        if (recvfrom(soc, reply, replySize, 0, (sockaddr*)&peer_addr, &peer_len) < 0) {
            perror("recvfrom failed for message");
            cleanup(soc, SENDER_SOCKET_PATH);
            return 1;
        }
        reply[replySize] = '\0'; // terminate the reply
        std::string replyStr = reply; // Convert reply to a string
        std::cout << "Received reply " << replyStr.substr(0, 1) << " : " << replyStr.substr(1) << "\n"; // Log the reply

        seqNum++; // Increment the sequence number
    }

    cleanup(soc, SENDER_SOCKET_PATH);
    return 0;
}
