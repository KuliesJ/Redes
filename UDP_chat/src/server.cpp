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
    map<string, pair<string, int>> clientsByName; // Almacena información de clientes
    map<int, string> clientsByPort; // Almacena información de clientes por puerto

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
        
        // Recibir mensaje del cliente
        recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client_addr, &addr_len);
        
        // Procesar el mensaje recibido
        processMessage(buffer);
    }

    void processMessage(const char* message) {
        char instruction = message[0]; // La primera letra indica la acción

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
        string lengthString(message + 1, 4); // Obtener la longitud del nickname
        int nicknameLength = stoi(lengthString);
        string nickname(message + 5, nicknameLength); // Obtener el nickname

        // Guardar la información del cliente
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
        int paddingLength = 1000 - (message.size() + 3); // 3 para la longitud del padding
        if (paddingLength < 0) paddingLength = 0; // Evitar padding negativo
        paddedMessage.append(paddingLength, '#'); // Añadir padding de '#'
        paddedMessage += to_string(paddingLength); // Añadir la longitud del padding
        return paddedMessage; // Retorna el mensaje con padding
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

        string response = "M" + string(4 - to_string(senderNickname.size()).size(), '0') + to_string(senderNickname.size()) + senderNickname +
                          string(4 - to_string(content.size()).size(), '0') + to_string(content.size()) + content;

        sendMessageToClients(response);
    }

    void handleBroadcast(const string& message) {
        // Extraer la longitud del contenido del mensaje
        int contentLength = stoi(message.substr(1, 4)); // Longitud del mensaje
        string content = message.substr(5, contentLength); // Mensaje

        // Obtener el nombre del remitente (nombre1) a partir del contexto actual (usando clientsByPort)
        string senderNickname; // Nombre del cliente que envía el mensaje
        int clientPort = ntohs(client_addr.sin_port); // Obtener el puerto del cliente actual
        if (clientsByPort.find(clientPort) != clientsByPort.end()) {
            senderNickname = clientsByPort[clientPort]; // Obtener el nombre del remitente
        } else {
            cout << "Sender not found in clientsByPort." << endl;
            return; // Salir si no se encuentra el remitente
        }

        // Formatear el mensaje para enviar a todos los clientes
        string response = "B" + string(4 - to_string(senderNickname.size()).size(), '0') + to_string(senderNickname.size()) +
                        senderNickname +
                        string(4 - to_string(content.size()).size(), '0') + to_string(content.size()) + content;

        // Enviar el mensaje a todos los clientes
        sendMessageToClients(response); // Asumiendo que sendMessageToClients envía el mensaje a la dirección correspondiente
    }



    void handleFileTransfer(const char* message) {
        cout << "File transfer request received." << endl;
    }

    void handleGameRequest(const char* message) {
        string destinatary(message + 5, stoi(string(message + 1, 4))); // Extraer destinatario
        cout << "Game request sent to: " << destinatary << endl;
    }

    void handlePlayTurn(const char* message) {
        string position(message + 1, strlen(message) - 1); // Extraer la posición
        cout << "Turn played at position: " << position << endl;
    }

    void handleResponseToRequest(const char* message) {
        string response(message + 1, strlen(message) - 1); // Extraer la respuesta
        cout << "Response to game request: " << response << endl;
    }

    void handleLogout(const char* message) {
        string nickname(message + 1, strlen(message) - 1); // Extraer el nickname
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
