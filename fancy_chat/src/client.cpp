// Basic includes
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <thread>
#include <iomanip>
#include <sstream>

using namespace std;

#define ADDRESS "127.0.0.1"
#define PORT 45001
#define MESSAGE_LENGTH 256

// Function to convert integer to fixed-width 5-character string
string int_to_padded_string(int num) {
    stringstream ss;
    ss << setw(5) << setfill('0') << num;
    return ss.str();
}

// Function to extract the actual message from the received message
string extract_message(const string& received_message) {
    size_t first_newline_pos = received_message.find('\n');
    if (first_newline_pos != string::npos) {
        return received_message.substr(first_newline_pos + 1);
    }
    return received_message;
}

void reading_thread(int socket) {
    char buffer[MESSAGE_LENGTH] = {};
    int bytes_read;
    while (true) {
        bytes_read = read(socket, buffer, MESSAGE_LENGTH - 1);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        string received_message(buffer);
        cout << "Received: " << extract_message(received_message) << endl;
    }
    close(socket);
}
string nickname;
void manage_server_request(int socket) {
    string sender_name;

    char request_type;
    while (true) {
        cout << "Enter request type (n: nickname, m: message, l: list, b: broadcast, o: logout): ";
        cin >> request_type;

        if (request_type == 'n') {
            cout << "Insert nickname: ";
            cin >> nickname;

            // Ensure nickname fits within MESSAGE_LENGTH - 5 (for length prefix)
            if (nickname.size() >= MESSAGE_LENGTH - 5) {
                cerr << "Nickname too long. Max length is " << (MESSAGE_LENGTH - 5) << " characters." << endl;
                continue;
            }

            // Format message as "n" + 5-byte length of nickname + nickname
            string message_to_server = "n";
            string length_str = int_to_padded_string(nickname.size());
            string full_message = message_to_server + length_str + nickname;
            write(socket, full_message.c_str(), full_message.size());
        } else if (request_type == 'm') {
            string dest_nickname;
            string message;
            cout << "Insert destination nickname: ";
            cin >> dest_nickname;
            cout << "Insert message: ";
            cin.ignore();  // Ignore leftover newline from previous input
            getline(cin, message);

            // Ensure destination nickname and message fit within MESSAGE_LENGTH - 10 (for length prefixes)
            if (dest_nickname.size() >= MESSAGE_LENGTH - 10 || message.size() >= MESSAGE_LENGTH - 10) {
                cerr << "Nickname or message too long." << endl;
                continue;
            }

            // Format message as "m" + 5-byte length of destination nickname + destination nickname + 5-byte length of message (including sender's name) + sender's name + newline + message
            string message_to_server = "m";
            string length_str_dest = int_to_padded_string(dest_nickname.size());
            string length_str_msg = int_to_padded_string(message.size() + sender_name.size() + nickname.size() + 1); 
            string full_message = message_to_server + length_str_dest + dest_nickname + length_str_msg + sender_name + nickname + ":" + message;
            cout << full_message << endl;
            write(socket, full_message.c_str(), full_message.size());
        } else if (request_type == 'l') {
            // Send request for list of users
            string message_to_server = "l";
            write(socket, message_to_server.c_str(), message_to_server.size());
        } else if (request_type == 'b') {
            string broadcast_message;
            cout << "Insert broadcast message: ";
            cin.ignore();  // Ignore leftover newline from previous input
            getline(cin, broadcast_message);

            // Ensure broadcast message fits within MESSAGE_LENGTH - 5 (for length prefix)
            if (broadcast_message.size() >= MESSAGE_LENGTH - 5) {
                cerr << "Broadcast message too long." << endl;
                continue;
            }

            // Format message as "b" + 5-byte length of broadcast message (including sender's name) + sender's name + newline + broadcast message
            string message_to_server = "b";
            string length_str = int_to_padded_string(broadcast_message.size() + sender_name.size() + 1 + nickname.size());
            string full_message = message_to_server + length_str + sender_name + nickname + ":" + broadcast_message;
            write(socket, full_message.c_str(), full_message.size());
        } else if (request_type == 'o') {
            // Send logout request
            string message_to_server = "o";
            write(socket, message_to_server.c_str(), message_to_server.size());
            break;
        } else {
            cerr << "Unknown request type" << endl;
        }
    }
}


int main() {
    // Socket setup
    struct sockaddr_in stSockAddr;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    inet_pton(AF_INET, ADDRESS, &stSockAddr.sin_addr);

    // Connect
    if (connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Start reading thread
    thread(reading_thread, SocketFD).detach();

    // Handle server requests
    manage_server_request(SocketFD);

    // Cleanup
    close(SocketFD);
    printf("Connection closed\n");
    return 0;
}
