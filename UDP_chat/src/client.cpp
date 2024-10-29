#include <iostream>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <fstream>
using namespace std;

class Client {
    string nickname;
    int port;
    string address;
    int sock;
    struct sockaddr_in server_addr;
    bool loggedIn; // Variable para verificar si el cliente está loggeado

public:
    // Constructor sin nickname
    Client(int _port, string _address) 
        : port(_port), address(_address), loggedIn(false) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        inet_pton(AF_INET, address.c_str(), &server_addr.sin_addr);
    }

    // Método para enviar mensajes con padding
    void sendMessage(const string& message) {
        string paddedMessage = message;

        // Calcular el padding necesario
        int paddingLength = 1000 - (message.size() + 3); // 3 para la longitud del padding
        if (paddingLength < 0) paddingLength = 0; // Evitar padding negativo

        // Añadir padding
        paddedMessage.append(paddingLength, '#'); // Añadir padding de '#'
        paddedMessage += to_string(paddingLength); // Añadir la longitud del padding

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
    }

    // Método para recibir mensajes y guardarlos en un string
    void receiveMessages() {
        char buffer[1005];
        socklen_t addr_len = sizeof(server_addr);
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            recvfrom(sock, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&server_addr, &addr_len);
            
            // Convertir el mensaje recibido a string
            string receivedMessage(buffer);

            // Guardar o procesar el mensaje
            //cout << "Received message as string: " << receivedMessage << endl;
            processReceivedMessage(receivedMessage);
            
        }
    }

    void processReceivedMessage(const string& message) {
        switch (message[0]) {
            case 'M':  // Mensaje directo
                handleMessage(message);
                break;
            case 'L':  // Lista de clientes
                handleListClients(message);
                break;
            case 'B':  // Mensaje de difusión (broadcast)
                handleBroadcast(message);
                break;
            case 'I':  // Invitación a juego
                handleGameInvitation(message);
                break;
            case 'T':  // Turno de juego
                handlePlayTurn(message);
                break;
            case 'O':  // Logout de un usuario
                handleLogout(message);
                break;
            case 'G':  // Logout de un usuario
                handleGamestate(message);
                break;
            case 'W':  // Logout de un usuario
                handleWin(message);
                break;
            default:
                cout << "Mensaje no reconocido: " << message << endl;
                break;
        }
    }

    void handleWin(const string& message) {
        cout << "Player with " << message[1] << " wins\n";
    }

    void handleGamestate(const string& message) {
        // Extraer el estado del juego, que son los caracteres de posición
        string gameState = message.substr(1, 9); // Obtener los 9 caracteres del estado

        // Imprimir el tablero 3x3
        cout << "Tablero de juego:" << endl;
        for (int i = 0; i < 3; ++i) {
            cout << " " << gameState[i * 3] << " | " << gameState[i * 3 + 1] << " | " << gameState[i * 3 + 2] << endl;
            if (i < 2) {
                cout << "---|---|---" << endl; // Separador entre filas
            }
        }
    }


    void handleMessage(const string& message) {
        // Lee la longitud del nombre del remitente
        int nameLength = stoi(message.substr(1, 4));
        
        // Extrae el nombre del remitente
        string remitent = message.substr(5, nameLength);
        
        // Lee la longitud del mensaje
        int messageLength = stoi(message.substr(5 + nameLength, 5));
        
        // Extrae el mensaje
        string msg = message.substr(9 + nameLength, messageLength);

        // Imprime el remitente y el mensaje
        cout << remitent << " envía: " << msg << endl;
        
        // Placeholder para procesar mensaje directo
        // Aquí puedes agregar la lógica para manejar el mensaje según sea necesario
    }


    void handleListClients(const string& message) {
        cout << "Lista de clientes recibida: " << endl;
        // Placeholder para procesar la lista de clientes
    }

    void handleBroadcast(const string& message) {
        cout << "Mensaje de difusión recibido: " << endl;
        // Placeholder para procesar mensaje de broadcast
    }

    void handleGameInvitation(const string& message) {
        cout << "Invitación a juego recibida: " << endl;
        // Placeholder para procesar invitación de juego
    }

    void handlePlayTurn(const string& message) {
        cout << "Turno de juego recibido: " << message << endl;
        // Placeholder para procesar turno de juego
    }

    void handleLogout(const string& message) {
        cout << "Logout recibido: " << message << endl;
        // Placeholder para procesar logout
    }
    
    void processInput() {
        string command;

        // Verificar si el cliente está loggeado
        if (!loggedIn) {
            cout << "Please log in first." << endl;
            return;
        }
        
        while (true) {
            cout << "Enter command:" << endl;
            cout << "  n - Login" << endl;
            cout << "  l - List connected clients" << endl;
            cout << "  m - Send message to a client" << endl;
            cout << "  b - Broadcast message to all clients" << endl;
            cout << "  f - Send a file to a client" << endl;
            cout << "  i - Send a game request" << endl;
            cout << "  j - Respond to game request" << endl;
            cout << "  t - Play your turn" << endl;
            cout << "  o - Logout" << endl;

            cin >> command;
            cin.ignore();

            switch (command[0]) {
                case 'n':
                    login();
                    break;
                case 'l':
                    listClients();
                    break;
                case 'm':
                    sendMessage(); // Placeholder para enviar un mensaje
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

    // Método para iniciar sesión
    void login() {
        char instruction = 'n';
        string nickname;

        cout << "Enter your nickname: ";
        cin >> nickname;

        string nicknameLength = to_string(nickname.size());
        nicknameLength = string(4 - nicknameLength.size(), '0') + nicknameLength; // Asegura que tenga 4 caracteres

        string message = instruction + nicknameLength + nickname; // Crea el mensaje sin padding
        string paddedMessage = applyPadding(message); // Aplica padding
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        loggedIn = true;
        
    }

    // Iniciar cliente
    void start() {
        thread receiver(&Client::receiveMessages, this);
        receiver.detach();
        login(); // Llamar al método de login al iniciar
        processInput();
    }

    // Destructor para cerrar el socket
    ~Client() {
        close(sock);
    }

private:
    string applyPadding(const string& message) {
        string paddedMessage = message;

        // Calcular el padding necesario
        int paddingLength = 1000 - (message.size() + 3); // 3 para la longitud del padding
        if (paddingLength < 0) paddingLength = 0; // Evitar padding negativo

        // Añadir padding
        paddedMessage.append(paddingLength, '#'); // Añadir padding de '#'
        paddedMessage += to_string(paddingLength); // Añadir la longitud del padding

        return paddedMessage; // Retorna el mensaje con padding
    }

    void listClients() {
        string message = "l";
        string paddedMessage = applyPadding(message);
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }

    void sendMessage() {
        char instruction = 'm';
        string destinatary;

        cout << "Destinatary: ";
        cin >> destinatary;
        cin.ignore();

        string content;
        cout << "Content: ";
        getline(cin, content);

        string destinataryLength = to_string(destinatary.size());
        destinataryLength = string(4 - destinataryLength.size(), '0') + destinataryLength;

        string contentLength = to_string(content.size());
        contentLength = string(4 - contentLength.size(), '0') + contentLength;

        string message = instruction + destinataryLength + destinatary + contentLength + content;

        string paddedMessage = applyPadding(message);
        
        cout << "Padded Message: " << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }


    void broadcastMessage() {
        char instruction = 'b';

        string content;
        cout << "Content: ";
        getline(cin, content);

        string contentLength = to_string(content.size());
        contentLength = string(4 - contentLength.size(), '0') + contentLength;

        string paddedMessage = applyPadding(instruction + contentLength + content);
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }

    void sendFile() {
        char instruction = 'f';

        string filename;
        cout << "Filename: ";
        getline(cin, filename);

        string filenameLength = to_string(filename.size());
        filenameLength = string(4 - filenameLength.size(), '0') + filenameLength;

        ifstream file(filename, ios::binary);
        if (!file) {
            cout << "Error opening file." << endl;
            return;
        }

        string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
        string contentLength = to_string(content.size());
        contentLength = string(4 - contentLength.size(), '0') + contentLength;

        string paddedMessage = applyPadding(instruction + filenameLength + filename + contentLength + content);
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }


    void sendGameRequest() {
        char instruction = 'i';

        string destinatary;
        cout << "Destinatary: ";
        getline(cin, destinatary);

        string destinataryLength = to_string(destinatary.size());
        destinataryLength = string(4 - destinataryLength.size(), '0') + destinataryLength;

        string paddedMessage = applyPadding(instruction + destinataryLength + destinatary);
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }

    void playTurn() {
        char instruction = 't';

        string position;
        cout << "Position (1-9): ";
        getline(cin, position);

        string paddedMessage = applyPadding(instruction + position);
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }

    void respondToRequest() {
        char instruction = 'j';

        string position;
        cout << "Want to play? (y/n): ";
        getline(cin, position);

        string paddedMessage = applyPadding(instruction + position);
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }

    void logout() {
        string paddedMessage = applyPadding("o");
        cout << paddedMessage << endl;

        sendto(sock, paddedMessage.c_str(), paddedMessage.size(), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

    }
};

int main() {
    int port = 8080;
    string address = "127.0.0.1";

    Client client(port, address);
    client.start();

    return 0;
}
