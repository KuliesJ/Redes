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

//Constants
int SocketFD = -1;
int MESSAGE_LENGHT = 256;

//Functions
void handle_sigint(int sig) {
    if (SocketFD != -1) {
        close(SocketFD);
    }
    printf("Server shutting down\n");
    exit(EXIT_SUCCESS);
}

using namespace std;

//Main function
int main(){
    printf("Server started, waiting for message\n");

    //SETUP
    struct sockaddr_in stSockAddr;
    int ConnectFD;

    // Set up signal handler for SIGINT
    signal(SIGINT, handle_sigint);

    //Create socket
    SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    //Socket thingies
    memset(&stSockAddr, 0, sizeof(stSockAddr));
    stSockAddr.sin_family = AF_INET;
    stSockAddr.sin_port = htons(1100);
    stSockAddr.sin_addr.s_addr = INADDR_ANY;

    //Bind socket
    bind(SocketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr));

    //Listen socket
    listen(SocketFD, 10);

    //Message buffer
    char buffer[256];

    while(true){
        // Accept connection
        ConnectFD = accept(SocketFD, NULL, NULL);

        do{
            //Message received
            read(ConnectFD,buffer,MESSAGE_LENGHT);
            printf("Pepito: %s\n", buffer);

            if(strcmp(buffer,"chau")==0){
                break;
            }

            //Message to send
            cout << "Josesito: "; cin.getline(buffer,MESSAGE_LENGHT);
            buffer[MESSAGE_LENGHT-1] = '\0';
            write(ConnectFD,buffer,MESSAGE_LENGHT);

        }while(strcmp(buffer,"chau")!=0);
        printf("Ended a connection\n");
    }
    // Close connection
    shutdown(ConnectFD, SHUT_RDWR);
    close(ConnectFD);

    close(SocketFD);
    return 0;
}