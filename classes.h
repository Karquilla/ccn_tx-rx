#pragma once

#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <random>
#include <sys/select.h>

// Utility function for cleanup
void cleanup(int soc, const char *path) {
    close(soc);          // Close the socket
    unlink(path);        // Remove the socket file from the filesystem
}

class Messenger {
public:
    // Constructor to set up the paths for sender and receiver
    Messenger(const char* senderPath, const char* receiverPath)
        : SENDER_SOCKET_PATH(senderPath), RECEIVER_SOCKET_PATH(receiverPath) {}

    // Destructor to clean up resources
    ~Messenger() {
        cleanup(sender_soc, SENDER_SOCKET_PATH);
        cleanup(receiver_soc, RECEIVER_SOCKET_PATH);
    }

    // Bind sender and receiver sockets to their respective paths
    void bindPath() {
        if (!pathSetup_) {
            // Configure sender address
            sender_addr.sun_family = AF_UNIX;
            std::strcpy(sender_addr.sun_path, SENDER_SOCKET_PATH);

            // Configure receiver address
            receiver_addr.sun_family = AF_UNIX;
            std::strcpy(receiver_addr.sun_path, RECEIVER_SOCKET_PATH);

            // Unlink old paths to avoid "address already in use" errors
            unlink(SENDER_SOCKET_PATH);
            unlink(RECEIVER_SOCKET_PATH);

            // Create and bind sender socket
            sender_soc = socket(AF_UNIX, SOCK_DGRAM, 0);
            if (sender_soc < 0) {
                perror("Socket creation failed for sender");
                exit(EXIT_FAILURE);
            }
            if (bind(sender_soc, (sockaddr*)&sender_addr, sizeof(sender_addr)) < 0) {
                perror("Bind failed for sender");
                cleanup(sender_soc, SENDER_SOCKET_PATH);
                exit(EXIT_FAILURE);
            }

            // Create and bind receiver socket
            receiver_soc = socket(AF_UNIX, SOCK_DGRAM, 0);
            if (receiver_soc < 0) {
                perror("Socket creation failed for receiver");
                exit(EXIT_FAILURE);
            }
            if (bind(receiver_soc, (sockaddr*)&receiver_addr, sizeof(receiver_addr)) < 0) {
                perror("Bind failed for receiver");
                cleanup(receiver_soc, RECEIVER_SOCKET_PATH);
                exit(EXIT_FAILURE);
            }

            pathSetup_ = true; // Mark paths as set up
        }
    }

    // Communication loop to send and receive messages
    void communicate(const char* destinationPath) {
        fd_set readfds;       // for select
        char buffer[1024];    // Buffer to store messages
        std::string message;  // Message to send

        while (true) {
            FD_ZERO(&readfds);                     // Clear the descriptor set
            FD_SET(receiver_soc, &readfds);       // Add receiver socket to the set
            FD_SET(STDIN_FILENO, &readfds);       // Add standard input (user input) to the set

            int max_fd = std::max(receiver_soc, STDIN_FILENO) + 1; // Calculate the max FD for select

            // Wait for activity on one of the file descriptors
            int activity = select(max_fd, &readfds, nullptr, nullptr, nullptr);

            if (activity < 0) {
                perror("select error");
                break;
            }

            // Check if there's data to receive
            if (FD_ISSET(receiver_soc, &readfds)) {
                ssize_t bytes_received = recvfrom(receiver_soc, buffer, sizeof(buffer) - 1, 0, nullptr, nullptr);
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0'; // Null-terminate the received data
                    std::cout << "Received: " << buffer << std::endl;
                } else {
                    perror("recvfrom error");
                }
            }

            // Check if there's user input to send
            if (FD_ISSET(STDIN_FILENO, &readfds)) {
                std::getline(std::cin, message); // Read input from the user
                if (message == "exit") {
                    std::cout << "Exiting communication loop." << std::endl;
                    break;
                }

                // Configure destination address for sending
                sockaddr_un dest_addr;
                dest_addr.sun_family = AF_UNIX;
                std::strcpy(dest_addr.sun_path, destinationPath);

                sendto(sender_soc, message.c_str(), message.size(), 0, (sockaddr*)&dest_addr, sizeof(dest_addr));
                std::cout << "Sent: " << message << std::endl;
            }
        }
    }

private:
    const char* SENDER_SOCKET_PATH; // Path for the sender socket
    const char* RECEIVER_SOCKET_PATH; // Path for the receiver socket
    sockaddr_un sender_addr; // Address for the sender
    sockaddr_un receiver_addr; // Address for the receiver
    int sender_soc = -1; // Sender socket descriptor
    int receiver_soc = -1; // Receiver socket descriptor
    bool pathSetup_ = false; // Flag to check if paths are set up
};

