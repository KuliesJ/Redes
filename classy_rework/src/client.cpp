#include <iostream>
#include <string>
#include <cstring>      // For memset
#include <unistd.h>    // For close(), read(), write()
#include <arpa/inet.h> // For socket functions
#include <thread>      // For std::thread
#include <atomic>      // For std::atomic

using namespace std;

class Client {
public:
    Client(const string& serverIP, const string& port) 
        : serverIP(serverIP), port(port), socketFD(-1), running(false) {}

    bool connectToServer() {
        socketFD = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFD < 0) {
            cerr << "Failed to create socket." << endl;
            return false;
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(stoi(port));
        inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

        if (connect(socketFD, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Failed to connect to server." << endl;
            close(socketFD);
            return false;
        }

        running = true;
        thread(&Client::readFromServer, this).detach(); // Start reading thread
        return true;
    }

    void run() {
        string nickname;
        cout << "Enter your nickname: ";
        cin >> nickname;
        registerUser(nickname); // Register the user after connection

        string command;
        while (true) {
            cout << "Enter command:" << endl;
            cout << "  l - List connected clients" << endl;
            cout << "  m - Send message to a client" << endl;
            cout << "  b - Broadcast message to all clients" << endl;
            cout << "  f - Send a file to a client" << endl;
            cout << "  i - Send a game request" << endl;
            cout << "  j - Respond to game request" << endl;
            cout << "  t - Play your turn" << endl;
            cout << "  o - Logout" << endl;

            cin >> command;

            switch (command[0]) {
                case 'l':
                    listClients();
                    break;
                case 'm':
                    sendMessage();
                    break;
                case 'b':
                    broadcastMessage();
                    break;
                case 'f':
                    sendFile();
                    break;
                case 'i':
                    sendGameRequest();
                    break;
                case 't':
                    playTurn();
                    break;
                case 'j':
                    respondToRequest();
                    break;
                case 'o':
                    logout();
                    return; // Exit the loop
                default:
                    cout << "Unknown command." << endl;
            }
        }
    }

private:
    string serverIP;
    string port;
    int socketFD;
    atomic<bool> running; // Controla el estado del hilo de lectura

    void readFromServer() {
    char buffer;
    while (running) {
        ssize_t bytesRead = read(socketFD, &buffer, 1);
        switch (buffer) {
        case 'T': {
            char game[9];
            char turn;
            read(socketFD, game, 9);
            read(socketFD, &turn, 1);
            cout << turn << "'s turn" << endl;
            cout << game << endl;
            break;
        }
        case 'G': {
            // Leer la longitud del nombre del usuario
            char lengthBuffer[5]; // Para almacenar los 4 dígitos + 1 para el terminador
            read(socketFD, lengthBuffer, 4);
            lengthBuffer[4] = '\0'; // Terminar la cadena

            // Convertir a número entero
            int userLength = atoi(lengthBuffer);
            
            // Leer el nombre del usuario
            char* usernameBuffer = new char[userLength + 1]; // +1 para el terminador
            read(socketFD, usernameBuffer, userLength);
            usernameBuffer[userLength] = '\0'; // Terminar la cadena

            // Imprimir el mensaje
            cout << "Game request from: " << usernameBuffer << endl;

            delete[] usernameBuffer; // Liberar la memoria
            break;
        }
        case 'W': {
            char result;
            read(socketFD, &result, 1); // Leer el resultado de la victoria
            if (result == '1') {
                cout << "You win!" << endl; // Mensaje para el ganador
            } else if (result == '0') {
                cout << "You lose!" << endl; // Mensaje para el perdedor
            }
            break;
        }
        default:
            break;
        }
    }
}


    void registerUser(const string& nickname) {
        string lengthStr = to_string(nickname.size());
        lengthStr.insert(lengthStr.begin(), 4 - lengthStr.length(), '0'); // Padded to 4 bytes
        string command = "n" + lengthStr + nickname; // Create the registration string
        cout << "Sending: " << command << endl;
        write(socketFD, command.c_str(), command.size());
    }

    void sendMessage() {
        string targetNickname, message;
        cout << "Enter target nickname: ";
        cin >> targetNickname;
        cout << "Enter your message: ";
        cin.ignore(); // Limpiar el buffer
        getline(cin, message);
        string targetLengthStr = to_string(targetNickname.size());
        targetLengthStr.insert(targetLengthStr.begin(), 4 - targetLengthStr.length(), '0');

        string messageLengthStr = to_string(message.size());
        messageLengthStr.insert(messageLengthStr.begin(), 5 - messageLengthStr.length(), '0');
        string command = "m" + targetLengthStr + targetNickname + messageLengthStr + message;
        cout << "Sending: " << command << endl;
        write(socketFD, command.c_str(), command.size());
    }

    void listClients() {
        write(socketFD, "l", 1); // Command to list clients
    }

    void broadcastMessage() {
        string message;
        cout << "Enter your broadcast message: ";
        cin.ignore(); // Limpiar el buffer
        getline(cin, message);
        string messageLengthStr = to_string(message.size());
        messageLengthStr.insert(messageLengthStr.begin(), 5 - messageLengthStr.length(), '0');
        string command = "b" + messageLengthStr + message;
        cout << "Sending: " << command << endl;
        write(socketFD, command.c_str(), command.size());
    }

    void sendFile() {
        string filename, sender;
        cout << "Enter filename: ";
        cin >> filename;
        cout << "Enter your name: ";
        cin >> sender;

        // Placeholder for file sending
        string command = "f" + to_string(filename.size()) + filename; // Adjust accordingly
        cout << "Sending: " << command << endl;
        write(socketFD, command.c_str(), command.size());
    }

    void sendGameRequest() {
        string targetNickname;
        cout << "Enter target nickname for game request: ";
        cin >> targetNickname;

        string targetLengthStr = to_string(targetNickname.size());
        targetLengthStr.insert(targetLengthStr.begin(), 4 - targetLengthStr.length(), '0');
        string command = "i" + targetLengthStr + targetNickname; // Game request
        cout << "Sending: " << command << endl;
        write(socketFD, command.c_str(), command.size());
    }

    void playTurn() {
        string gameData;
        cout << "Enter your turn data: ";
        cin >> gameData;

        string command = "t" + gameData; // Adjust for actual game data
        cout << "Sending: " << command << endl;
        write(socketFD, command.c_str(), command.size());
    }

    void respondToRequest() {
        string responseData;
        cout << "Enter your response data: ";
        cin >> responseData;

        string command = "j" + responseData; // Adjust for actual response data
        cout << "Sending: " << command << endl;
        write(socketFD, command.c_str(), command.size());
    }

    void logout() {
        running = false;
        write(socketFD, "o", 1); // Comando de logout
        readResponse(); // Leer respuesta del servidor
        close(socketFD);
    }

    void readResponse() {
        char command;
        ssize_t bytesRead = read(socketFD, &command, 1);
        switch (command)
        {
        case 'T':
            char tablero[9];
            read(socketFD, &tablero, 9);
            char turn;
            read(socketFD, &turn, 1);
            cout << turn << "'s turn" << endl;
            for(int i=0;i<9;i++){
                cout << tablero[i];
                if((i+1)%3==0){
                    cout << endl;
                }
            }
            break;
        
        default:
            break;
        }
    }
};

int main() {
    Client client("127.0.0.1", "12346"); // IP del servidor y puerto
    if (!client.connectToServer()) {
        return 1; // Error al conectar
    }

    client.run(); // Ejecutar el bucle de comandos

    return 0;
}
