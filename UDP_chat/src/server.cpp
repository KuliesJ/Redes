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

    void handleMessage(const string& message) {
    try {
        // Asegúrate de que el mensaje tiene la longitud suficiente
        if (message.size() < 14) { // Mínimo esperado: 'm' + 4 + nombre + 4 + contenido
            throw std::invalid_argument("Message too short");
        }

        // Obtener el nickname del remitente
        int nicknameLength = stoi(message.substr(1, 4)); // Longitud del nickname
        if (nicknameLength <= 0 || nicknameLength + 5 > message.size()) {
            throw std::invalid_argument("Invalid nickname length");
        }
        string senderNickname = message.substr(5, nicknameLength); // Nombre del remitente

        // Obtener el contenido del mensaje
        int contentLengthIndex = 5 + nicknameLength;
        int contentLength = stoi(message.substr(contentLengthIndex, 4)); // Longitud del contenido
        if (contentLength <= 0 || contentLength + contentLengthIndex + 4 > message.size()) {
            throw std::invalid_argument("Invalid content length");
        }
        string content = message.substr(contentLengthIndex + 4, contentLength); // Contenido del mensaje

        // Imprimir para depurar
        cout << "Nickname: " << senderNickname << " | Content: " << content << endl;

        // Construir el mensaje para enviar
        string response = "M" + string(4 - to_string(senderNickname.size()).size(), '0') + to_string(senderNickname.size()) +
                          string(4 - to_string(content.size()).size(), '0') + to_string(content.size()) + content;

        cout << "Message from " << senderNickname << ": " << content << endl;

        // Aquí puedes enviar el mensaje a otros clientes o gestionar la lógica adicional
    } catch (const std::invalid_argument& e) {
        cerr << "Invalid argument: " << e.what() << " | Message: " << message << endl;
    } catch (const std::out_of_range& e) {
        cerr << "Out of range: " << e.what() << " | Message: " << message << endl;
    } catch (const std::exception& e) {
        cerr << "An error occurred: " << e.what() << " | Message: " << message << endl;
    }
}

    void handleBroadcast(const char* message) {
        string content(message + 5, stoi(string(message + 1, 4))); // Extraer contenido

        // Implementar la lógica para enviar un mensaje a todos los clientes
        cout << "Broadcast message: " << content << endl;
    }

    void handleFileTransfer(const char* message) {
        // Aquí implementas la lógica para manejar la transferencia de archivos
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
