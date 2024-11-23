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

int main() {
    std::vector<std::string> data;
    const char* SOCKET_PATH = "./receiver_soc";
    sockaddr_un peer;
    int soc;
    peer.sun_family = AF_UNIX;
    std::strcpy(peer.sun_path, SOCKET_PATH);

    soc = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (soc < 0) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

   
   

    int seqNum = 0;
    int messageCount = getRand(1, 5);
    for (int i = 0; i < messageCount; i++) {
        data.push_back(generateMessage());
    }
    data.push_back("\0");

    for (auto &message : data) {
        message = std::to_string(seqNum) + message; // Add sequence number
        char sizeOfMessage = message.length() + '0'; // Single-byte size
        int sizeSent = sendto(soc, &sizeOfMessage, 1, 0, (sockaddr *)&peer, sizeof(peer));
        if (sizeSent < 0) {
            perror("sendto failed for size");
            close(soc);
            return 1;
        }
        sleep(1);
        const char *msg = message.c_str();
        int n = sendto(soc, msg, message.length(), 0, (sockaddr *)&peer, sizeof(peer));
        if (n < 0) {
            perror("sendto failed for message");
            close(soc);
            return 1;
        }
        sleep(1);
        seqNum++;
        std::cout << "Sent message: " << message << " (" << n << " bytes)\n";
    }

    close(soc);
    return 0;
}
