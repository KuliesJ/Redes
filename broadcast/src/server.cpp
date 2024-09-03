#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <thread>
#include <set>
#include <mutex>

using namespace std;

#define PORT 45002
// Constants
int SocketFD = -1;
const int MESSAGE_LENGTH = 256;

// Mutex for thread-safe access to sockets set
mutex mtx;

// Function to handle SIGINT signal
void handle_sigint(int sig) {
    if (SocketFD != -1) {
        close(SocketFD);
    }
    printf("Server shutting down\n");
    exit(EXIT_SUCCESS);
}

// Set to hold connected sockets
set<int> sockets;

// Function to handle reading and broadcasting messages
void reading_thread(int socket) {
    char buffer[MESSAGE_LENGTH] = {};
    int bytes_read;

    while (true) {
        bytes_read = read(socket, buffer, MESSAGE_LENGTH - 1); // Ensure null-termination space
        if (bytes_read <= 0) {
            // Error or connection closed
            break;
        }
        buffer[bytes_read] = '\0'; // Null-terminate the buffer

        // Lock mutex for thread-safe access to sockets
        {
            lock_guard<mutex> guard(mtx);
            for (int s : sockets) {
                if (s != socket) { // Avoid sending to the same socket
                    write(s, buffer, bytes_read);
                }
            }
        }
    }

    // Remove the socket from the set and close it
    {
        lock_guard<mutex> guard(mtx);
        sockets.erase(socket);
    }
    close(socket);
    cout << "Connection closed" << endl;
}

// Main function
int main() {
    printf("Server started, waiting for connections\n");

    // Set up signal handler for SIGINT
    signal(SIGINT, handle_sigint);

    // Create socket
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (SocketFD < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up sockaddr_in structure
    struct sockaddr_in stSockAddr;
    memset(&stSockAddr, 0, sizeof(stSockAddr));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(PORT);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind socket
    if (bind(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr)) < 0) {
        perror("bind failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    // Listen on socket
    if (listen(SocketFD, 10) < 0) {
        perror("listen failed");
        close(SocketFD);
        exit(EXIT_FAILURE);
    }

    while (true) {
        // Accept connection
        int ConnectFD = accept(SocketFD, NULL, NULL);
        if (ConnectFD < 0) {
            perror("accept failed");
            continue;
        }

        // Add the new socket to the set and start a new thread
        {
            lock_guard<mutex> guard(mtx);
            sockets.insert(ConnectFD);
        }
        thread(reading_thread, ConnectFD).detach();
    }

    // Close the main socket (unreachable in this loop, but good practice)
    close(SocketFD);
    return 0;
}
