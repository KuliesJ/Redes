//Basic includes
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
#include <string>

#include <thread>

//Constants
#define ADDRESS "127.0.0.1"
#define MESSAGE_LENGTH 256



using namespace std;

void reading_thread(int socket) {
    char buffer[MESSAGE_LENGTH] = {};
    int bytes_read;
    while (true) {
        bytes_read = read(socket, buffer, MESSAGE_LENGTH - 1); 
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        printf("Received: %s\n", buffer);
    }
    close(socket);
}

char send_nickname = 'n';

void manage_server_request(int socket) {
    char action_requested_by_server;
    read(socket, &action_requested_by_server, 1);
    string nickname;
    string message_to_server;
    string full_message;
    switch (action_requested_by_server) {
        case 'N':
            message_to_server = "n";
            cout << "Insert nickname: ";
            cin >> nickname;

            full_message = message_to_server + nickname + '\0';
            write(socket, full_message.c_str(), full_message.size());

            break;

        default:
            std::cerr << "Unknown request " << action_requested_by_server << std::endl;
            break;
    }
}

//Main function
int main(){

    //Socket
    struct sockaddr_in stSockAddr;
    int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(45002);

    // Convert IPv4 and IPv6 addresses from text to binary form
    inet_pton(AF_INET, ADDRESS, &stSockAddr.sin_addr);

    //Connection
    connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in));

    //Message variables
    string message_to_send;
    char buffer[256];
    
    do {
        thread(manage_server_request, SocketFD).detach();
        //Message to send
        cin.getline(buffer, MESSAGE_LENGTH);

        buffer[MESSAGE_LENGTH - 1] = '\0';

        

        // Enviar el mensaje
        write(SocketFD, buffer, strlen(buffer) + 1);

        if(strcmp(buffer,"chau")==0){
            break;
        }

        
    } while (strcmp(buffer, "chau") != 0);
    printf("Ended connection\n");
    return 0;
}