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
#include <map>
#include <mutex>
using namespace std;

#define PORT 45003
const int MESSAGE_LENGTH = 256;

// Map to hold connected nicknames and their sockets
map<string, int> sockets;
int SocketFD = -1;


// Mutex for thread-safe access to sockets map
mutex mtx;

// Function to handle SIGINT signal
void handle_sigint(int sig) {
    if (SocketFD != -1) {
        close(SocketFD);
    }
    printf("Server shutting down\n");
    exit(EXIT_SUCCESS);
}

void manage_requests(int socket) {
    bool session_available = true;
    while (session_available) {
        char request;
        ssize_t bytes_read = read(socket, &request, 1);
        if (bytes_read <= 0) {
            // Error or connection closed
            break;
        }

        if (request == 'n') {
            char nickname_size_str[6] = {}; // 5 characters + null terminator
            ssize_t size_bytes_read = read(socket, nickname_size_str, 5);
            if (size_bytes_read <= 0) {
                // Error or connection closed
                session_available = false;
                break;
            }
            nickname_size_str[5] = '\0'; // Ensure null-termination

            int nickname_size = atoi(nickname_size_str);
            
            if (nickname_size <= 0 || nickname_size > MESSAGE_LENGTH - 1) {
                // Invalid nickname size
                const string error_message = "Invalid nickname size";
                write(socket, "E", 1); // Error code
                write(socket, to_string(error_message.size()).c_str(), to_string(error_message.size()).size());
                write(socket, error_message.c_str(), error_message.size());
                //session_available = false;
                break;
            }

            char nickname[MESSAGE_LENGTH] = {};
            ssize_t nickname_bytes_read = read(socket, nickname, nickname_size);
            if (nickname_bytes_read <= 0) {
                // Error or connection closed
                //session_available = false;
                break;
            }
            nickname[nickname_size] = '\0'; // Null-terminate the nickname
            
            {
                lock_guard<mutex> guard(mtx);
                if (sockets.find(nickname) != sockets.end()) {
                    // Nickname already exists
                    const string error_message = "User already exists";
                    write(socket, "E", 1); // Error code
                    write(socket, to_string(error_message.size()).c_str(), to_string(error_message.size()).size());
                    write(socket, error_message.c_str(), error_message.size());
                    //session_available = false;
                } else {
                    // Add nickname and socket to the map
                    sockets[nickname] = socket;
                    cout << socket << " logged in as " << nickname << endl;
                    //write(socket, "A", 1); // Acknowledgment for successful login
                }
            }
        } else {
            if(request!='\n')cerr << "Unknown request \"" << request  << "\"" << endl;
            //session_available = false;
        }
    }

    close(socket);
    cout << "Connection closed" << endl;
}

// Main function
int main() {
    printf("Server started, waiting for connections\n");

    // Setup

    {
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
    }

    while (true) {
        // Accept connection
        int client_socket = accept(SocketFD, NULL, NULL);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        // Add the new socket to the map and start new threads
        {
            lock_guard<mutex> guard(mtx);
            printf("Someone is trying to join\n");
        }

        thread(manage_requests, client_socket).detach();
    }

    // Close the main socket (unreachable in this loop, but good practice)
    close(SocketFD);
    return 0;
}
