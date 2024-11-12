// sender.cpp
// Sender program for a datagram socket using UNIX domain in C++

#include <iostream>     // for std::cout and std::cerr
#include <cstring>      // for std::strlen and std::strcpy
#include <sys/types.h>  // basic socket types
#include <sys/socket.h> // main socket functions
#include <sys/un.h>     // UNIX domain socket structures
#include <unistd.h>     // for close()

int main() {
    // Define the path for the receiver socket file
    const char *SOCKET_PATH = "./receiver_soc";
    const char *message = "Hello there"; // Message to send
    int soc;                             // Socket file descriptor
    sockaddr_un peer;                    // Struct for the address of the receiving socket

    // Set up the address struct for the receiver (peer)
    peer.sun_family = AF_UNIX;               // Set address family to UNIX domain
    std::strcpy(peer.sun_path, SOCKET_PATH); // Copy socket path to peer struct

    // Create a datagram (SOCK_DGRAM) socket in the UNIX domain (AF_UNIX)
    soc = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (soc < 0) { // Check for socket creation error
        std::cerr << "Socket creation failed\n";
        return 1;  // Exit with error code
    }

    // Send the message to the receiver
    int n = sendto(soc, message, std::strlen(message), 0, (sockaddr*)&peer, sizeof(peer));
    if (n < 0) { // Check for sending error
        std::cerr << "sendto failed\n";
        close(soc); // Close the socket
        return 1;
    }

    // Output confirmation of message sent with byte count
    std::cout << "Sent message: " << message << " (" << n << " bytes)\n";
    close(soc); // Close the socket
    return 0;   // Successful execution
}
