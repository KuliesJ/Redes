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
#include <iomanip>  // Para std::setw y std::setfill
#include <sstream>  // Para std::stringstream

using namespace std;

#define PORT 45001
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

void handle_nickname_request(int socket) {
    // Read nickname size
    char nickname_size_str[6] = {}; // 5 characters + null terminator
    ssize_t size_bytes_read = read(socket, nickname_size_str, 5);
    if (size_bytes_read <= 0) {
        close(socket);
        return;
    }
    nickname_size_str[5] = '\0'; // Ensure null-termination

    int nickname_size = atoi(nickname_size_str);
    if (nickname_size <= 0 || nickname_size >= MESSAGE_LENGTH) {
        const string error_message = "Invalid nickname size";
        write(socket, "E", 1); // Error code
        write(socket, to_string(error_message.size()).c_str(), to_string(error_message.size()).size());
        write(socket, error_message.c_str(), error_message.size());
        close(socket);
        return;
    }

    // Read nickname
    char nickname[MESSAGE_LENGTH] = {};
    ssize_t nickname_bytes_read = read(socket, nickname, nickname_size);
    if (nickname_bytes_read <= 0) {
        close(socket);
        return;
    }
    nickname[nickname_size] = '\0'; // Null-terminate the nickname

    lock_guard<mutex> guard(mtx);
    if (sockets.find(nickname) != sockets.end()) {
        const string error_message = "User already exists";
        write(socket, "E", 1); // Error code
        write(socket, to_string(error_message.size()).c_str(), to_string(error_message.size()).size());
        write(socket, error_message.c_str(), error_message.size());
    } else {
        sockets[nickname] = socket;
        cout << socket << " logged in as " << nickname << endl;
        write(socket, "A", 1); // Acknowledgment for successful login
    }
}

void handle_message_request(int socket) {
    // Read destination nickname size
    char dest_nickname_size_str[6] = {}; // 5 characters + null terminator
    ssize_t size_bytes_read = read(socket, dest_nickname_size_str, 5);
    if (size_bytes_read <= 0) {
        close(socket);
        return;
    }
    dest_nickname_size_str[5] = '\0'; // Ensure null-termination

    int dest_nickname_size = atoi(dest_nickname_size_str);
    char dest_nickname[MESSAGE_LENGTH] = {};
    ssize_t dest_nickname_bytes_read = read(socket, dest_nickname, dest_nickname_size);
    if (dest_nickname_bytes_read <= 0) {
        close(socket);
        return;
    }
    dest_nickname[dest_nickname_size] = '\0'; // Null-terminate the destination nickname

    // Read message size
    char message_size_str[6] = {}; // 5 characters + null terminator
    ssize_t message_size_bytes_read = read(socket, message_size_str, 5);
    if (message_size_bytes_read <= 0) {
        close(socket);
        return;
    }
    message_size_str[5] = '\0'; // Ensure null-termination

    int message_size = atoi(message_size_str);
    char message[MESSAGE_LENGTH] = {};
    ssize_t message_bytes_read = read(socket, message, message_size);
    if (message_bytes_read <= 0) {
        close(socket);
        return;
    }
    message[message_size] = '\0'; // Null-terminate the message

    // Prepare message in format: "M" + 5-byte length + message
    stringstream ss;
    ss << setw(5) << setfill('0') << message_size;
    string length_str = ss.str();
    
    string full_message = "M" + length_str + string(message, message_size);

    lock_guard<mutex> guard(mtx);
    auto it = sockets.find(dest_nickname);
    if (it != sockets.end()) {
        int dest_socket = it->second;
        write(dest_socket, full_message.c_str(), full_message.size());
    } else {
        const string error_message = "User not found";
        write(socket, "E", 1); // Error code
        write(socket, to_string(error_message.size()).c_str(), to_string(error_message.size()).size());
        write(socket, error_message.c_str(), error_message.size());
    }
}


void handle_list_request(int socket) {
    lock_guard<mutex> guard(mtx);

    // Prepare the list of connected users
    string user_list = "{";
    for (const auto& entry : sockets) {
        user_list += entry.first + ", ";
    }
    if (!user_list.empty()) {
        user_list.pop_back(); // Remove the last space
        user_list.pop_back(); // Remove the last comma
    }
    user_list += "}";

    // Format the length as a 5-byte string
    stringstream ss;
    ss << setw(5) << setfill('0') << user_list.size();
    string length_str = ss.str();

    // Send the list to the requesting client
    write(socket, "L", 1); // Code for list of users
    write(socket, length_str.c_str(), length_str.size()); // Send length (5 bytes)
    write(socket, user_list.c_str(), user_list.size()); // Send the user list
}


void handle_broadcast_request(int socket) {
    // Read broadcast message size
    char broadcast_message_size_str[6] = {}; // 5 characters + null terminator
    ssize_t size_bytes_read = read(socket, broadcast_message_size_str, 5);
    if (size_bytes_read <= 0) {
        close(socket);
        return;
    }
    broadcast_message_size_str[5] = '\0'; // Ensure null-termination

    int broadcast_message_size = atoi(broadcast_message_size_str);
    char broadcast_message[MESSAGE_LENGTH] = {};
    ssize_t broadcast_message_bytes_read = read(socket, broadcast_message, broadcast_message_size);
    if (broadcast_message_bytes_read <= 0) {
        close(socket);
        return;
    }
    broadcast_message[broadcast_message_size] = '\0'; // Null-terminate the message

    // Prepare broadcast message in format: "B" + 5-byte length + broadcast message
    stringstream ss;
    ss << setw(5) << setfill('0') << broadcast_message_size;
    string length_str = ss.str();
    
    string full_message = "B" + length_str + string(broadcast_message, broadcast_message_size);

    lock_guard<mutex> guard(mtx);
    for (const auto& entry : sockets) {
        int user_socket = entry.second;
        if (user_socket != socket) { // Avoid sending the message back to the sender
            write(user_socket, full_message.c_str(), full_message.size());
        }
    }
}


void handle_logout_request(int socket) {

    // Find the nickname associated with this socket
    string nickname_to_remove;
    {
        lock_guard<mutex> guard(mtx);
        // Find the nickname associated with this socket
        for (auto it = sockets.begin(); it != sockets.end(); ++it) {
            if (it->second == socket) {
                nickname_to_remove = it->first;
                break;
            }
        }
        if (!nickname_to_remove.empty()) {
            sockets.erase(nickname_to_remove);
            cout << nickname_to_remove << " has logged out." << endl;
        }
    }

    close(socket);
}


void manage_requests(int socket) {
    bool session_available = true;
    while (session_available) {
        char request_type;
        ssize_t bytes_read = read(socket, &request_type, 1);
        if (bytes_read <= 0) {
            break;
        }

        switch (request_type) {
            case 'n':
                handle_nickname_request(socket);
                break;

            case 'm':
                handle_message_request(socket);
                break;

            case 'l':
                handle_list_request(socket);
                break;

            case 'b':
                handle_broadcast_request(socket);
                break;

            case 'o':
                handle_logout_request(socket);
                session_available = false; // End session after logout
                break;

            default:
                cerr << "Unknown request \"" << request_type << "\"" << endl;
                break;
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
        signal(SIGINT, handle_sigint);

        SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (SocketFD < 0) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        struct sockaddr_in stSockAddr;
        memset(&stSockAddr, 0, sizeof(stSockAddr));
        stSockAddr.sin_family = AF_INET;
        stSockAddr.sin_port = htons(PORT);
        stSockAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr)) < 0) {
            perror("bind failed");
            close(SocketFD);
            exit(EXIT_FAILURE);
        }

        if (listen(SocketFD, 10) < 0) {
            perror("listen failed");
            close(SocketFD);
            exit(EXIT_FAILURE);
        }
    }

    while (true) {
        int client_socket = accept(SocketFD, NULL, NULL);
        if (client_socket < 0) {
            perror("accept failed");
            continue;
        }

        {
            lock_guard<mutex> guard(mtx);
            printf("Someone is trying to join\n");
        }

        thread(manage_requests, client_socket).detach();
    }

    close(SocketFD);
    return 0;
}
