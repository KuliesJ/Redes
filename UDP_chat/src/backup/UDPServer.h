#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <algorithm>

using namespace std;
class UDPServer {
public:
    int port;
    unordered_map<string, int> clients; // Mapa para mantener los clientes conectados
    unordered_map<int, string> clients_r;
    UDPServer(int port) : port(port), running(true) {
        // Crear socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Error creando socket");
        }

        // Configurar dirección del servidor
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        // Bind del socket
        if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Error en bind");
        }
    }

    ~UDPServer() {
        running = false;
        close(sockfd);
    }

    void start() {
        std::thread(&UDPServer::receiveMessages, this).detach();
        std::cout << "Servidor UDP escuchando en el puerto " << port << std::endl;
        while (true) {
            continue;
        }
    }

    void processRequestReceived(char* buffer, struct sockaddr_in& client_addr, socklen_t addr_len) {
        switch (buffer[0]) {
        case 'n':
            handleLogin(buffer, client_addr, addr_len);
            break;
        case 'o':
            handleLogout(buffer, client_addr, addr_len);
            break;
        case 'm':
            handleMessage(buffer, client_addr, addr_len);
            break;
        default:
            std::cout << "Comando no reconocido." << std::endl;
            break;
        }
    }

private:
    int sockfd;
    struct sockaddr_in server_addr;
    std::atomic<bool> running;

    void receiveMessages() {
        char buffer[1005];
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        while (true) {
            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
            if (n > 0) {
                buffer[n] = '\0'; // Asegurarse de que el buffer sea un string
                std::cout << "Mensaje recibido: " << buffer << std::endl;
                processRequestReceived(buffer, client_addr, addr_len);
            }
        }
    }

    void handleLogin(char* buffer, struct sockaddr_in& client_addr, socklen_t addr_len) {
        // Extraer el tamaño del nickname (4 caracteres) y el nickname
        std::string nicknameSize(buffer + 1, 4); // Extraer el tamaño del nickname
        int size = std::stoi(nicknameSize); // Convertir a entero
        std::string nickname(buffer + 5, size); // Extraer el nickname

        // Crear un identificador único para el cliente usando su IP y puerto
        int clientID = client_addr.sin_addr.s_addr ^ client_addr.sin_port;

        // Añadir al cliente en ambos mapas
        clients[nickname] = clientID;
        clients_r[clientID] = nickname;

        std::cout << "Usuario conectado: " << nickname << std::endl;

        // Responder al cliente
        const char* response = "Login exitoso";
        sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)&client_addr, addr_len);
    }


    void handleLogout(char* buffer, struct sockaddr_in& client_addr, socklen_t addr_len) {
    // Identificar al cliente por su dirección en lugar del nickname.
    auto it = std::find_if(clients.begin(), clients.end(), [&](const auto& pair) {
        return clients_r[pair.second] == inet_ntoa(client_addr.sin_addr);
    });

    if (it != clients.end()) {
        int clientID = it->second;
        clients.erase(it);  // Borrar del mapa `clients`
        clients_r.erase(clientID);  // Borrar del mapa `clients_r`

        std::cout << "Usuario desconectado: " << it->first << std::endl;

        // Responder al cliente
        const char* response = "Logout exitoso";
        sendto(sockfd, response, strlen(response), 0, (struct sockaddr*)&client_addr, addr_len);
    } else {
        std::cout << "Usuario no encontrado para desconectar." << std::endl;
    }
}

void handleMessage(char* buffer, struct sockaddr_in& client_addr, socklen_t addr_len) {
    // Extraer el tamaño del destinatario (4 caracteres)
    std::string destinatarySize(buffer + 1, 4);
    int destSize = std::stoi(destinatarySize);

    // Extraer el nombre del destinatario (según `destSize`)
    std::string destinatary(buffer + 5, destSize);

    // Extraer el tamaño del mensaje (4 caracteres)
    std::string messageSize(buffer + 5 + destSize, 4);
    int messageLen = std::stoi(messageSize);

    // Extraer el contenido del mensaje (según `messageLen`)
    std::string message(buffer + 9 + destSize, messageLen);

    // Verificar si el destinatario está conectado
    auto it = clients.find(destinatary);
    if (it != clients.end()) {
        int destinataryID = it->second;
        auto destinataryIt = clients_r.find(destinataryID);

        if (destinataryIt != clients_r.end()) {
            struct sockaddr_in destinataryAddr;
            destinataryAddr.sin_family = AF_INET;
            destinataryAddr.sin_addr.s_addr = inet_addr(destinataryIt->second.c_str());
            destinataryAddr.sin_port = htons(destinataryID);

            // Construir el mensaje para el destinatario
            std::string response = "M" + destinatarySize + destinatary + messageSize + message;

            // Enviar el mensaje al destinatario
            sendto(sockfd, response.c_str(), response.size(), 0, (struct sockaddr*)&destinataryAddr, sizeof(destinataryAddr));
        } else {
            const char* errorResponse = "Destinatario no encontrado";
            sendto(sockfd, errorResponse, strlen(errorResponse), 0, (struct sockaddr*)&client_addr, addr_len);
        }
    } else {
        const char* errorResponse = "Destinatario no encontrado";
        sendto(sockfd, errorResponse, strlen(errorResponse), 0, (struct sockaddr*)&client_addr, addr_len);
    }
}

};
