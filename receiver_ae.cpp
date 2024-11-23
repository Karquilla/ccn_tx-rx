#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>
#include <string>
#include <vector>

void cleanup(int soc, const char *path) {
    close(soc);
    unlink(path);
}

int main() {
    const char *SOCKET_PATH = "./receiver_soc";
    int soc;                     // Socket file descriptor
    sockaddr_un self, peer;      // Structs for socket addresses (self = receiver, peer = sender)
    socklen_t peer_len = sizeof(peer); // Length of the peer struct for receiving the message

    // Set up the address struct for the receiver (self)
    self.sun_family = AF_UNIX;               // Set address family to UNIX domain
    std::strcpy(self.sun_path, SOCKET_PATH); // Copy socket path to self struct

    // Create a datagram (SOCK_DGRAM) socket in the UNIX domain (AF_UNIX)
    soc = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (soc < 0) { // Check for socket creation error
        std::cerr << "Socket creation failed\n";
        return 1;  // Exit with error code
    }

    // Bind the socket to the specified path
    if (bind(soc, (sockaddr*)&self, sizeof(self)) < 0) {
        std::cerr << "Bind failed\n"; // Print error if binding fails
        cleanup(soc, SOCKET_PATH);    // Clean up resources
        return 1;
    }

    char buf[1]; // Buffer for size message
    std::vector<std::string> messages;

    while (true) {
        // Receive size
        int sizeReceived = recvfrom(soc, buf, 1, 0, (sockaddr*)&peer, &peer_len);
        if (sizeReceived < 0) {
            perror("recvfrom failed for size");
            cleanup(soc, SOCKET_PATH);
            return 1;
        }
        sleep(1);
        int sizeNum = buf[0] - '0'; // Convert size to int
        
        std::cout << sizeNum << std::endl;
        // Receive message
        char message[sizeNum + 1];
        int msgReceived = recvfrom(soc, message, sizeNum, 0, (sockaddr *)&peer, &peer_len);
        if (msgReceived < 0) {
            perror("recvfrom failed for message");
            cleanup(soc, SOCKET_PATH);
            return 1;
        }
        sleep(1);
        if (message[1] == '\0') {
            std::cout << "End-of-transmission marker received\n";
            break;
        }
        std::string msg = "";
        for(auto& chr : message) {
            msg += chr;
        }
        messages.push_back(msg);
        
    }

    for (auto &msg : messages) {
        std::cout << "Received message: " << msg << "\n";
    }
    cleanup(soc, SOCKET_PATH);
    return 0;
}