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
            case 'n':
                handleLogin(message);
                break;
            case 'l':
                listClients();
                break;
            case 'm':
                handleMessage(message);
                break;
            case 'b':
                handleBroadcast(message);
                break;
            case 'f':
                handleFileTransfer(message);
                break;
            case 'i':
                handleGameRequest(message);
                break;
            case 't':
                handlePlayTurn(message);
                break;
            case 'j':
                handleResponseToRequest(message);
                break;
            case 'o':
                handleLogout(message);
                break;
            default:
                cout << "Unknown instruction: " << instruction << endl;
        }
    }

    void handleLogin(const char* message) {
        string lengthString(message + 1, 4);
        int nicknameLength = stoi(lengthString);
        string nickname(message + 5, nicknameLength);
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        int clientPort = ntohs(client_addr.sin_port);
        clientsByName[nickname] = { ip, clientPort };
        clientsByPort[clientPort] = nickname;
        cout << nickname << " logged in from " << ip << ":" << clientPort << endl;
    }

    void listClients() {
        string clientList;
        for (const auto& client : clientsByName) {
            clientList += client.first + " from " + client.second.first + ":" + to_string(client.second.second) + "\n";
        }
        if (clientList.empty()) {
            clientList = "No clients connected.";
        }
        cout << "Connected clients:\n" << clientList << endl;
    }

    string applyPadding(const string& message) {
        string paddedMessage = message;
        int paddingLength = 1000 - (message.size() + 3);
        if (paddingLength < 0) paddingLength = 0;
        paddedMessage.append(paddingLength, '#');
        paddedMessage += to_string(paddingLength);
        return paddedMessage;
    }

    void sendMessageToClients(const string& message) {
        string paddedMessage = applyPadding(message);
        for (const auto& client : clientsByName) {
            string nickname = client.first;
            string ip = client.second.first;
            int port = client.second.second;
            struct sockaddr_in dest_addr;
            dest_addr.sin_family = AF_INET;
            dest_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &dest_addr.sin_addr);
            sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
        }
    }

    void sendMessageToClient(const string& message, const string& clientNickname) {
        string paddedMessage = applyPadding(message);
        if (clientsByName.find(clientNickname) != clientsByName.end()) {
            auto [ip, port] = clientsByName[clientNickname];
            struct sockaddr_in destinatary_addr;
            destinatary_addr.sin_family = AF_INET;
            destinatary_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &destinatary_addr.sin_addr);
            sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&destinatary_addr, sizeof(destinatary_addr));
            cout << "Message sent to " << clientNickname << endl;
        } else {
            cout << "Client not found: " << clientNickname << endl;
        }
    }

    void handleMessage(const string& message) {
        if (message.size() < 14) {
            return;
        }
        int nicknameLength = stoi(message.substr(1, 4));
        string senderNickname = message.substr(5, nicknameLength);
        int contentLengthIndex = 5 + nicknameLength;
        int contentLength = stoi(message.substr(contentLengthIndex, 4));
        string content = message.substr(contentLengthIndex + 4, contentLength);
        cout << "Message from " << senderNickname << ": " << content << endl;

        // Send the response only to the sender
        string response = "M" + string(4 - to_string(senderNickname.size()).size(), '0') + to_string(senderNickname.size()) + senderNickname +
                          string(4 - to_string(content.size()).size(), '0') + to_string(content.size()) + content;
        sendMessageToClient(response, senderNickname);
    }

    void handleBroadcast(const string& message) {
        int contentLength = stoi(message.substr(1, 4));
        string content = message.substr(5, contentLength);
        string senderNickname;
        int clientPort = ntohs(client_addr.sin_port);
        if (clientsByPort.find(clientPort) != clientsByPort.end()) {
            senderNickname = clientsByPort[clientPort];
        } else {
            cout << "Sender not found in clientsByPort." << endl;
            return;
        }
        string response = "B" + string(4 - to_string(senderNickname.size()).size(), '0') + to_string(senderNickname.size()) +
                        senderNickname +
                        string(4 - to_string(content.size()).size(), '0') + to_string(content.size()) + content;
        sendMessageToClients(response);
    }

    void handleFileTransfer(const char* message) {
        cout << "File transfer request received." << endl;
    }

    void handleGameRequest(const char* message) {
        int destinataryLength = stoi(string(message + 1, 4));
        string destinatary(message + 5, destinataryLength);
        string senderNickname;
        int clientPort = ntohs(client_addr.sin_port);
        if (clientsByPort.find(clientPort) != clientsByPort.end()) {
            senderNickname = clientsByPort[clientPort];
        } else {
            cout << "Sender not found in clientsByPort." << endl;
            return;
        }
        string response = "I" + string(4 - to_string(senderNickname.size()).size(), '0') + to_string(senderNickname.size()) +
                        senderNickname;
        if (clientsByName.find(destinatary) != clientsByName.end()) {
            auto [ip, port] = clientsByName[destinatary];
            response = applyPadding(response);
            struct sockaddr_in destinatary_addr;
            memset(&destinatary_addr, 0, sizeof(destinatary_addr));
            destinatary_addr.sin_family = AF_INET;
            destinatary_addr.sin_port = htons(port);
            inet_pton(AF_INET, ip.c_str(), &destinatary_addr.sin_addr);
            sendto(sock, response.c_str(), response.size(), 0, (struct sockaddr*)&destinatary_addr, sizeof(destinatary_addr));
            cout << "Game request sent to " << destinatary << " from " << senderNickname << endl;
        } else {
            cout << "Recipient not found: " << destinatary << endl;
        }
    }

    void handlePlayTurn(const char* message) {
        string position(message + 1, strlen(message) - 1);
        cout << "Turn played at position: " << position << endl;
    }

    void handleResponseToRequest(const string& message) {
    if (message.size() < 3) return;

    char responseType = message[1]; // 'y' o 'n'

    if (responseType == 'y') {
        cout << "Jugador 2 aceptado." << endl;

        int clientPort = ntohs(client_addr.sin_port);
        string player1Nickname = clientsByPort[clientPort];

        if (!player1Nickname.empty()) {
            auto it = clientsByName.find(player1Nickname);
            if (it != clientsByName.end()) {
                string player2Ip = it->second.first; // Almacenar la IP como cadena
                cout << "Jugador 2 asociado con IP: " << player2Ip << endl; // Imprimir la IP correctamente
                player2 = clientPort; // Asocia el puerto del jugador 2
            }
        }
    } else if (responseType == 'n') {
        cout << "Jugador 2 rechazado." << endl;
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
