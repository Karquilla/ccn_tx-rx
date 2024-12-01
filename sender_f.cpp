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
    Messenger messenger("./p1_sender", "./p1_receiver");
    messenger.bindPath();
    messenger.communicate("./p2_receiver"); // Send messages to Process 2's receiver
    return 0;
}