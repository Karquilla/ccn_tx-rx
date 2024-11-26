#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
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
    const char* SENDER_SOCKET_PATH = "./sender_soc";
    const char* RECEIVER_SOCKET_PATH = "./receiver_soc";
    sockaddr_un sender_addr, receiver_addr;

    // Create sender socket
    int soc = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (soc < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Bind sender socket to a unique path
    sender_addr.sun_family = AF_UNIX;
    std::strcpy(sender_addr.sun_path, SENDER_SOCKET_PATH);

    if (bind(soc, (sockaddr*)&sender_addr, sizeof(sender_addr)) < 0) {
        perror("Bind failed for sender");
        cleanup(soc, SENDER_SOCKET_PATH);
        return 1;
    }

    // Set up receiver address
    receiver_addr.sun_family = AF_UNIX;
    std::strcpy(receiver_addr.sun_path, RECEIVER_SOCKET_PATH);

    // Generate random messages to send
    std::vector<std::string> data;
    int seqNum = 0;
    int messageCount = getRand(1, 5);
    for (int i = 0; i < messageCount; i++) {
        data.push_back(generateMessage());
    }
    data.push_back("END");

    // Send messages to receiver
    for (auto &message : data) {
        message = std::to_string(seqNum) + message;
        uint8_t sizeOfMessage = static_cast<uint8_t>(message.length());

        // Send size of the message
        if (sendto(soc, &sizeOfMessage, 1, 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr)) < 0) {
            perror("sendto failed for size");
            cleanup(soc, SENDER_SOCKET_PATH);
            return 1;
        }
        sleep(1);

        // Send the actual message
        if (sendto(soc, message.c_str(), message.length(), 0, (sockaddr*)&receiver_addr, sizeof(receiver_addr)) < 0) {
            perror("sendto failed for message");
            cleanup(soc, SENDER_SOCKET_PATH);
            return 1;
        }
        sleep(1);
        if (message.substr(1) == "END") {
                    break;
        }
        std::cout << "Sent message "<< seqNum << " : " << message.substr(1) << "\n";
       

        // Receive reply from receiver
        uint8_t replySize;
        sockaddr_un peer_addr;
        socklen_t peer_len = sizeof(peer_addr);
        if (recvfrom(soc, &replySize, 1, 0, (sockaddr*)&peer_addr, &peer_len) < 0) {
            perror("recvfrom failed for size");
            cleanup(soc, SENDER_SOCKET_PATH);
            return 1;
        }

        char reply[replySize + 1];
        if (recvfrom(soc, reply, replySize, 0, (sockaddr*)&peer_addr, &peer_len) < 0) {
            perror("recvfrom failed for message");
            cleanup(soc, SENDER_SOCKET_PATH);
            return 1;
        }
        reply[replySize] = '\0';
        std::string replyStr = reply;
        std::cout << "Received reply "<< replyStr.substr(0,1) << " : " << replyStr.substr(1) << "\n";

        seqNum++;
        
    }

    cleanup(soc, SENDER_SOCKET_PATH);
    return 0;
}