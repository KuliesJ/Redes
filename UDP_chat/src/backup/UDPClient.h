#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <atomic>

class UDPClient {
public:
    int udpMessageSize = 1000;
    std::string nickname;
    bool isLoggedIn;
    int port;
    std::string server_ip;
    UDPClient(const char* server_ip, int port) : server_ip(server_ip), port(port), running(true) {
        // Crear socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Error creando socket");
        }

        // Configurar dirección del servidor
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    }

    ~UDPClient() {
        running = false;
        close(sockfd);
    }

    // Función para crear un mensaje con padding
    std::string createPaddedMessage(const std::string& rawMessage, size_t totalLength) {
        // Calcular cuánto padding se necesita
        size_t paddingSize = totalLength - rawMessage.size() - 2; // 3 para el tamaño del padding
        std::string padding = std::string(paddingSize, '#'); // Rellenar con '#'

        // Crear el mensaje completo
        std::string messageWithPadding = rawMessage + padding;

        // Agregar la longitud del padding en los últimos 3 caracteres
        std::string paddingLength = std::to_string(paddingSize);
        paddingLength = std::string(3 - paddingLength.size(), '0') + paddingLength; // Rellenar a 3 caracteres
        messageWithPadding += paddingLength;

        return messageWithPadding;
    }

    void sendLoginRequest() {
        std::string nickname;
        std::cout << "Nickname: ";
        std::cin >> nickname;

        // Obtener el tamaño del nickname y rellenarlo a 4 caracteres
        std::string nicknameSize = std::to_string(nickname.size());
        nicknameSize = std::string(4 - nicknameSize.size(), '0') + nicknameSize;

        // Crear el mensaje base
        std::string rawMessage = "n" + nicknameSize + nickname;

        // Crear el UDPMessage utilizando la función de padding
        std::string UDPMessage = createPaddedMessage(rawMessage, 1000);

        // Imprimir el mensaje final
        //std::cout << "UDP Message: " << UDPMessage << std::endl;

        // Enviar el mensaje al servidor
        sendMessage(UDPMessage);
    }

    void sendLogoutRequest(){
        // Crear el mensaje base
        std::string rawMessage = "o";

        // Crear el UDPMessage utilizando la función de padding
        std::string UDPMessage = createPaddedMessage(rawMessage, 1000);

        // Imprimir el mensaje final
        //std::cout << "UDP Message: " << UDPMessage << std::endl;
    }

    void sendMessageRequest(){
        std::string destinatary;
        std::cout << "Destinatary: ";
        std::cin >> destinatary;

        std::string message;
        std::cout << "Message: ";
        std::cin.ignore(); // Ignorar el salto de línea anterior
        std::getline(std::cin, message); // Leer el mensaje completo

        // Obtener la longitud del destinatario y rellenarla a 4 caracteres
        std::string destinatarySize = std::to_string(destinatary.size());
        destinatarySize = std::string(4 - destinatarySize.size(), '0') + destinatarySize;

        // Obtener la longitud del mensaje y rellenarla a 5 caracteres
        std::string messageSize = std::to_string(message.size());
        messageSize = std::string(5 - messageSize.size(), '0') + messageSize;

        // Crear el mensaje base
        std::string rawMessage = "m" + destinatarySize + destinatary + messageSize + message;

        // Crear el UDPMessage utilizando la función de padding
        std::string UDPMessage = createPaddedMessage(rawMessage, 1000);

        // Imprimir el mensaje final
        //std::cout << "UDP Message: " << UDPMessage << std::endl;

        sendMessage(UDPMessage);
    }

    void processUI(){
        while(true){
            std::cout << "n: Login\no: Logout\nm: Message\n" << std::endl;
            char userInput;
            std::cin >> userInput;
            switch (userInput)
            {
            case 'n':
                sendLoginRequest();
                break;
            case 'o':
                if (isLoggedIn) sendLogoutRequest();
                break;
            case 'm':
                if (isLoggedIn) sendMessageRequest();
                break;
            default:
                std::cout << "Input not recognized or you are not logged in" << std::endl;
                break;
            }
        }
    }

    void start() {
        std::thread(&UDPClient::receiveResponses, this).detach();
        std::cout << "Cliente UDP listo para enviar mensajes." << std::endl;
        processUI();
    }

    void sendMessage(const std::string& message) {
        sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }

private:
    int sockfd;
    struct sockaddr_in server_addr;
    std::atomic<bool> running;
    
    void receiveResponses() {
        char buffer[1024];
        while (running) {
            socklen_t addr_len = sizeof(server_addr);
            int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&server_addr, &addr_len);
            if (n > 0) {
                buffer[n] = '\0'; // Asegurarse de que el buffer sea un string
                std::cout << "Respuesta del servidor: " << buffer << std::endl;
            }
        }
    }
};
