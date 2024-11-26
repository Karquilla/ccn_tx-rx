#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>     // For fcntl() and F_GETFL, F_SETFL, O_NONBLOCK
#include <sys/un.h>
#include <string>
#include <vector>
#include <random>

int getRand(int begin, int end) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(begin, end);
    return dis(gen);
}

std::string generateMessage() {
    int length = getRand(3, 10);
    std::string randMsg = "";
    for (int i = 0; i < length; i++) {
        randMsg += std::to_string(getRand(0, 9));
    }
    return randMsg;
}

void cleanup(int soc, const char *path) {
    close(soc);
    unlink(path);
}

int main() {
    const char *SOCKET_PATH = "./receiver_soc";
    int soc;
    sockaddr_un self, peer;
    socklen_t peer_len = sizeof(peer);

    // Set up the receiver address
    memset(&self, 0, sizeof(self));
    self.sun_family = AF_UNIX;
    std::strcpy(self.sun_path, SOCKET_PATH);

    // Create receiver socket
    soc = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (soc < 0) {
        perror("Socket creation failed");
        return 1;
    }

    //int flags = fcntl(soc, F_GETFL, 0);
    //fcntl(soc, F_SETFL, flags | O_NONBLOCK);
    // Bind the socket to the address
    if (bind(soc, (sockaddr*)&self, sizeof(self)) < 0) {
        perror("Bind failed");
        cleanup(soc, SOCKET_PATH);
        return 1;
    }

    while (true) {
        memset(&peer, 0, sizeof(peer));
        peer_len = sizeof(peer);

        uint8_t sizeNum;
        if (recvfrom(soc, &sizeNum, 1, 0, (sockaddr*)&peer, &peer_len) < 0) {
            perror("recvfrom failed for size");
            cleanup(soc, SOCKET_PATH);
            return 1;
        }

        char message[sizeNum + 1];
        if (recvfrom(soc, message, sizeNum, 0, (sockaddr*)&peer, &peer_len) < 0) {
            perror("recvfrom failed for message");
            cleanup(soc, SOCKET_PATH);
            return 1;
        }
        message[sizeNum] = '\0';

        std::string msg(message);
        if (msg.substr(1) == "END") {
            std::cout << "End-of-transmission\n";
            break;
        }

        int seqNum = std::stoi(msg.substr(0, 1));
        std::cout << "Message number: " << seqNum << " Received message: " << msg.substr(1) << "\n";

        std::string reply = std::to_string(seqNum) + generateMessage();
        uint8_t replySize = static_cast<uint8_t>(reply.length());

        // Send reply size
        if (sendto(soc, &replySize, 1, 0, (sockaddr*)&peer, peer_len) < 0) {
            perror("sendto failed for size");
            cleanup(soc, SOCKET_PATH);
            return 1;
        }

        // Send reply message
        if (sendto(soc, reply.c_str(), reply.length(), 0, (sockaddr*)&peer, peer_len) < 0) {
            perror("sendto failed for message");
            cleanup(soc, SOCKET_PATH);
            return 1;
        }
        std::cout << "Sent reply: " << reply << "\n";
    }

    cleanup(soc, SOCKET_PATH);
    return 0;
}
