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

#include <thread>
#include <set>

using namespace std;

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

set<int> sockets = {};

void reading_thread(int socket){
    char buffer[256] = {};
    int bytes_read;
    do{
        bytes_read = read(socket,buffer,MESSAGE_LENGHT);
        if(bytes_read <= 0)break;
        buffer[MESSAGE_LENGHT-1]='\0';
        for(auto s: sockets){
            write(s,buffer,MESSAGE_LENGHT);
        }
        printf("Cliente: %s\n", buffer);
    }
    while(strcmp(buffer,"chau")!=0);
    sockets.erase(socket);
    cout << sockets.size() << endl;
    close(socket);
}

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

    while(true){
        // Accept connection
        ConnectFD = accept(SocketFD, NULL, NULL);
        thread(reading_thread,ConnectFD).detach();
        sockets.insert(ConnectFD);
        
    }
    // Close connection
    shutdown(ConnectFD, SHUT_RDWR);
    close(ConnectFD);

    close(SocketFD);
    return 0;
}