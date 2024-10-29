#include <iostream>
#include <string>
#include <cstring>
#include <map>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <fstream>
using namespace std;

class Server {
    int sock;
    struct sockaddr_in server_addr, client_addr;
    map<string, pair<string, int>> clientsByName;
    map<int, string> clientsByPort;
    string gameState = "---------";
    int player1, player2;
    bool isPlayerXTurn = true;
public:
    Server(int port) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }

    void start() {
        cout << "Server is running..." << endl;
        while (true) {
            handleClient();
        }
    }

    ~Server() {
        close(sock);
    }

private:
    void handleClient() {
        char buffer[1005];
        socklen_t addr_len = sizeof(client_addr);
        memset(buffer, 0, sizeof(buffer));
        recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        processMessage(buffer);
    }

    void processMessage(const char* message) {
        char instruction = message[0];
        switch (instruction) {
            case 'n': handleLogin(message); break;
            case 'l': listClients(); break;
            case 'm': handleMessage(message); break;
            case 'b': handleBroadcast(message); break;
            case 'f': handleFileTransfer(message); break;
            case 'i': handleGameRequest(message); break;
            case 't': handlePlayTurn(message); break;
            case 'j': handleResponseToRequest(message); break;
            case 'o': handleLogout(message); break;
            default: cout << "Unknown instruction: " << instruction << endl;
        }
    }

    void handleLogin(const char* message) {
        int nicknameLength = stoi(string(message + 1, 4));
        string nickname(message + 5, nicknameLength);
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        int clientPort = ntohs(client_addr.sin_port);
        clientsByName[nickname] = { ip, clientPort };
        clientsByPort[clientPort] = nickname;
        cout << nickname << " logged in from " << ip << ":" << clientPort << endl;
    }

    string fixNumber(int number, int fixTo) {
        string numStr = to_string(number);
        if (numStr.size() < fixTo) {
            numStr.insert(0, fixTo - numStr.size(), '0');  // Relleno con ceros a la izquierda
        }
        return numStr;
    }

    void listClients() {
        string clientList;
        for (const auto& client : clientsByName) {
            clientList += client.first + " ";
        }

        int clientPort = getPortByNickname(clientsByPort[ntohs(client_addr.sin_port)]);
        if (clientPort != -1) {
            string message = "L" + fixNumber(clientList.size(), 5) + clientList;
            sendMessageToClient(applyPadding(message), clientsByPort[clientPort]);
        } else {
            cout << "Client port not found for listing clients." << endl;
        }
    }

    string applyPadding(const string& message) {
        string paddedMessage = message;
        int paddingLength = 1000 - (message.size() + 3);
        paddedMessage.append(max(0, paddingLength), '#');
        paddedMessage += fixNumber(paddingLength, 3);
        return paddedMessage;
    }

    void sendMessageToClients(const string& message) {
        string paddedMessage = applyPadding(message);
        for (const auto& client : clientsByName) {
            sendMessageToClient(paddedMessage, client.first);
        }
    }

    void sendMessageToClient(const string& message, const string& clientNickname) {
        if (clientsByName.find(clientNickname) != clientsByName.end()) {
            auto [ip, port] = clientsByName[clientNickname];
            struct sockaddr_in destinatary_addr;
            destinatary_addr.sin_family = AF_INET;
            destinatary_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &destinatary_addr.sin_addr);
            sendto(sock, message.c_str(), message.size(), 0, (struct sockaddr*)&destinatary_addr, sizeof(destinatary_addr));
            cout << "Message sent to " << clientNickname << endl;
        } else {
            cout << "Client not found: " << clientNickname << endl;
        }
    }

    void handleMessage(const string& message) {
        int nicknameLength = stoi(message.substr(1, 4));
        string senderNickname = message.substr(5, nicknameLength);
        int contentLengthIndex = 5 + nicknameLength;
        int contentLength = stoi(message.substr(contentLengthIndex, 4));
        string content = message.substr(contentLengthIndex + 4, contentLength);
        cout << "Message from " << senderNickname << ": " << content << endl;

        string response = "M" + fixNumber(senderNickname.size(), 4) + senderNickname +
                          fixNumber(content.size(), 4) + content;

        response = applyPadding(response);
        sendMessageToClient(response, senderNickname);
    }

    void handleBroadcast(const string& message) {
        int contentLength = stoi(message.substr(1, 4));
        string content = message.substr(5, contentLength);
        string senderNickname = clientsByPort[ntohs(client_addr.sin_port)];

        string response = "B" + fixNumber(senderNickname.size(), 4) + senderNickname +
                          fixNumber(content.size(), 4) + content;
        sendMessageToClients(response);
    }

    void handleFileTransfer(const char* message) {
        cout << "File transfer request received." << endl;
    }

    int getPortByNickname(const string& nickname) {
        if (clientsByName.find(nickname) != clientsByName.end()) {
            return clientsByName[nickname].second;
        }
        return -1; // Retorna -1 si el nickname no se encuentra
    }

    void handleGameRequest(const char* message) {
        int destinataryLength = stoi(string(message + 1, 4));
        string destinatary(message + 5, destinataryLength);
        int clientPort = ntohs(client_addr.sin_port);
        string senderNickname = clientsByPort[clientPort];

        // Guardar el puerto del jugador 1 usando getPortByNickname
        player1 = getPortByNickname(senderNickname);

        string response = "I" + fixNumber(senderNickname.size(), 4) + senderNickname;

        if (clientsByName.find(destinatary) != clientsByName.end()) {
            sendMessageToClient(applyPadding(response), destinatary);
            cout << "Game request sent to " << destinatary << " from " << senderNickname << endl;
        } else {
            cout << "Recipient not found: " << destinatary << endl;
        }
    }

    void handlePlayTurn(const char* message) {
        // La posición recibida en el mensaje es un string; convertir a índice entero
        int position = stoi(string(message + 1, strlen(message) - 1)) - 1;

        // Validar la posición (debe estar entre 0 y 8 y ser un espacio vacío en gameState)
        if (position < 0 || position > 8 || gameState[position] != '-') {
            cout << "Invalid move at position: " << position + 1 << endl;
            return;
        }

        // Actualizar el estado del juego
        gameState[position] = isPlayerXTurn ? 'X' : 'O';
        cout << "Turn played at position: " << position + 1 << endl;
        cout << "Current game state: " << gameState << endl;

        // Verificar si hay un ganador
        char winner = checkWinner();
        if (winner != '-') {
            // Anunciar al ganador y enviar mensaje a ambos jugadores
            string winMessage = "W" + string(1, winner); // Mensaje de victoria
            sendMessageToClient(applyPadding(winMessage), clientsByPort[player1]);
            cout << "Message sent to player1 about winner." << endl;
            sendMessageToClient(applyPadding(winMessage), clientsByPort[player2]);
            cout << "Message sent to player2 about winner." << endl;
            cout << "Player " << winner << " has won the game!" << endl;
            return; // Terminar la función si hay un ganador
        }

        // Cambiar turno al otro jugador
        isPlayerXTurn = !isPlayerXTurn;

        // Notificar a ambos jugadores sobre el estado actual del juego
        string updateMessage = "G" + gameState;
        sendMessageToClient(applyPadding(updateMessage), clientsByPort[player1]);
        cout << "Message sent to player1 about game state." << endl;
        sendMessageToClient(applyPadding(updateMessage), clientsByPort[player2]);
        cout << "Message sent to player2 about game state." << endl;
    }

    char checkWinner() {
        // Comprobaciones horizontales
        for (int i = 0; i < 3; i++) {
            if (gameState[i * 3] != '-' && gameState[i * 3] == gameState[i * 3 + 1] && gameState[i * 3] == gameState[i * 3 + 2]) {
                return gameState[i * 3]; // Retorna 'X' o 'O'
            }
        }

        // Comprobaciones verticales
        for (int i = 0; i < 3; i++) {
            if (gameState[i] != '-' && gameState[i] == gameState[i + 3] && gameState[i] == gameState[i + 6]) {
                return gameState[i]; // Retorna 'X' o 'O'
            }
        }

        // Comprobaciones diagonales
        if (gameState[0] != '-' && gameState[0] == gameState[4] && gameState[0] == gameState[8]) {
            return gameState[0]; // Retorna 'X' o 'O'
        }
        if (gameState[2] != '-' && gameState[2] == gameState[4] && gameState[2] == gameState[6]) {
            return gameState[2]; // Retorna 'X' o 'O'
        }

        return '-'; // Sin ganador
    }

    void handleResponseToRequest(const string& message) {
        char responseType = message[1];
        if (responseType == 'y') {
            cout << "Player 2 accepted." << endl;
            player2 = ntohs(client_addr.sin_port);
        } else if (responseType == 'n') {
            cout << "Player 2 rejected." << endl;
        }
    }

    void handleLogout(const char* message) {
        string nickname(message + 1, strlen(message) - 1);
        cout << nickname << " logged out." << endl;
        clientsByName.erase(nickname);
    }
};

int main() {
    int port = 8080;
    Server server(port);
    server.start();
    return 0;
}
