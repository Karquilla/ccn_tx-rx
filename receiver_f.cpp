#include <iostream>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <random>

#include "classes.h"


int main() {
    Messenger messenger("./p2_sender", "./p2_receiver");
    messenger.bindPath();
    messenger.communicate("./p1_receiver"); // Send messages to Process 1's receiver
    return 0;
}
