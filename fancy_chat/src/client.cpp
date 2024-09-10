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

//received message
void process_message(const string& message) {
    cout << "Message: " << message << endl;
}

// list of users
void process_list_of_users(const string& list) {
    if (list.length() > 2 && list.front() == '{' && list.back() == '}') {
        string users = list.substr(1, list.length() - 2);
        cout << "List of Users:\n" << users << endl;
    } else {
        cerr << "Malformed list of users" << endl;
    }
}

// error messages
void process_error(const string& error) {
    cout << "Error: " << error << endl;
}

// session
void process_acknowledgement() {
    cout << "Nickname accepted. You can now send messages and perform other actions." << endl;
}

// registration status
bool is_registered = false;

void reading_thread(int socket) {
    char buffer[MESSAGE_LENGTH] = {};
    while (true) {
        // Read the type character ('M', 'L', 'E', 'A', 'B')
        int bytes_read = read(socket, buffer, 1);
        if (bytes_read <= 0) {
            break;
        }

        char type = buffer[0];
        if (type != 'M' && type != 'L' && type != 'E' && type != 'A' && type != 'B') {
            cerr << "Unknown type: " << type << endl;
            continue;
        }

        if (type == 'A') {
            process_acknowledgement();
            is_registered = true;
            continue;
        }

        bytes_read = read(socket, buffer, 5);
        if (bytes_read != 5) {
            cerr << "Failed to read length field" << endl;
            break;
        }
        buffer[5] = '\0'; // Null-terminate
        int length = atoi(buffer);
        if (length <= 0 || length > MESSAGE_LENGTH - 1) {
            cerr << "Invalid length: " << length << endl;
            break;
        }

        // Read
        bytes_read = read(socket, buffer, length);
        if (bytes_read != length) {
            cerr << "Failed to read complete message" << endl;
            //break;
        }

        buffer[length] = '\0'; // Null-terminate
        string received_message(buffer);

        switch (type) {
            case 'M':
                process_message(received_message);
                break;
            case 'L':
                process_list_of_users(received_message);
                break;
            case 'E':
                process_error(received_message);
                break;
            case 'B':
                process_message(received_message);
                break;
        }
    }
    close(socket);
}

// Function to handle user requests
void manage_server_request(int socket, string& sender_name) {
    char request_type;
    while (true) {
        cout << "Enter request type (n: nickname, m: message, l: list, b: broadcast, o: logout): ";
        cin >> request_type;

        if (request_type == 'n') {
            cout << "Insert nickname: ";
            cin >> sender_name;

            // Ensure nickname fits within MESSAGE_LENGTH - 5 (for length prefix)
            if (sender_name.size() >= MESSAGE_LENGTH - 5) {
                cerr << "Nickname too long. Max length is " << (MESSAGE_LENGTH - 5) << " characters." << endl;
                continue;
            }

            // Format message as "n" + 5-byte length of nickname + nickname
            string message_to_server = "n" + int_to_padded_string(sender_name.size()) + sender_name;
            write(socket, message_to_server.c_str(), message_to_server.size());

        } else if (request_type == 'm') {
            if (!is_registered) {
                cerr << "You must be registered to send messages." << endl;
                continue;
            }

            string dest_nickname, message;
            cout << "Insert destination nickname: ";
            cin >> dest_nickname;
            cout << "Insert message: ";
            cin.ignore();
            getline(cin, message);

            // Ensure destination nickname and message fit within MESSAGE_LENGTH - 10 (for length prefixes)
            if (dest_nickname.size() >= MESSAGE_LENGTH - 10 || message.size() >= MESSAGE_LENGTH - 10) {
                cerr << "Nickname or message too long." << endl;
                continue;
            }

            // Format message as "m" + 5-byte length of destination nickname + destination nickname + 5-byte length of message (including sender's name) + sender's name + newline + message
            string length_str_dest = int_to_padded_string(dest_nickname.size());
            string length_str_msg = int_to_padded_string(message.size() + sender_name.size() + 1);
            string full_message = "m" + length_str_dest + dest_nickname + length_str_msg + sender_name + ":" + message;
            write(socket, full_message.c_str(), full_message.size());

        } else if (request_type == 'l') {
            if (!is_registered) {
                cerr << "You must be registered to request the list of users." << endl;
                continue;
            }

            // Send request for list of users
            write(socket, "l", 1);

        } else if (request_type == 'b') {
            if (!is_registered) {
                cerr << "You must be registered to broadcast messages." << endl;
                continue;
            }

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
            string length_str = int_to_padded_string(broadcast_message.size() + sender_name.size() + 1);
            string full_message = "b" + length_str + sender_name + ":" + broadcast_message;
            write(socket, full_message.c_str(), full_message.size());

        } else if (request_type == 'o') {
            // Send logout request
            write(socket, "o", 1);
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

    memset(&stSockAddr, 0, sizeof(stSockAddr));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    inet_pton(AF_INET, ADDRESS, &stSockAddr.sin_addr);

    // Connect
    if (connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(stSockAddr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Start reading thread
    string sender_name;
    thread(reading_thread, SocketFD).detach();

    // Handle server requests
    manage_server_request(SocketFD, sender_name);

    // Cleanup
    close(SocketFD);
    cout << "Connection closed" << endl;
    return 0;
}
