#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

class Server {
public:
    Server(const string& port) : port(port), running(false) {}

    void start() {
        running = true;
        setupServerSocket();
        thread acceptThread(&Server::acceptConnections, this);
        acceptThread.detach();
    }

    void stop() {
        running = false;
        close(serverSocket);
    }

private:
    string port;
    int serverSocket;
    map<int, string> users; // Mapa de descriptores de socket a apodos
    mutex userMutex;
    bool running;

    std::map<int, string*> games; // Mapa para almacenar el estado del tablero de cada juego
    std::map<int, char> playerSymbols; // Mapa para almacenar el símbolo de cada jugador
    std::mutex gameMutex; // Mutex para sincronizar el acceso al juego
    std::map<int, int> rival;

    void setupServerSocket() {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            cerr << "Failed to create socket." << endl;
            exit(EXIT_FAILURE);
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(stoi(port));

        if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
            cerr << "Failed to bind socket." << endl;
            exit(EXIT_FAILURE);
        }

        listen(serverSocket, 5);
    }

    void acceptConnections() {
        while (running) {
            int clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket >= 0) {
                thread(&Server::handleClient, this, clientSocket).detach();
            }
        }
    }

    void handleClient(int clientSocket) {
        while (running) {
            char command;
            ssize_t bytesRead = read(clientSocket, &command, 1);
            if (bytesRead <= 0) {
                break; // Cliente desconectado
            }

            switch (command) {
                case 'n':
                    loginClient(clientSocket);
                    break;
                case 'l':
                    listClients(clientSocket);
                    break;
                case 'o':
                    logoutClient(clientSocket);
                    break;
                case 'i':
                    sendGameRequest(clientSocket);
                    break;
                case 'j':
                    respondToRequest(clientSocket);
                    break;
                case 't':
                    playTurn(clientSocket);
                    break;
                default:
                    sendResponse(clientSocket, "Unknown command.");
                    break;
            }
        }

        {
            lock_guard<mutex> lock(userMutex);
            users.erase(clientSocket);
        }

        close(clientSocket);
    }

    void sendResponse(int clientSocket, const string& message) {
        write(clientSocket, message.c_str(), message.size());
    }

    void listClients(int clientSocket) {
        lock_guard<mutex> lock(userMutex);
        
        string userCountStr = to_string(users.size());
        userCountStr.insert(userCountStr.begin(), 5 - userCountStr.length(), '0'); // Rellenar a 5 bytes

        string clientList = "L" + userCountStr; // Comando 'L' para lista de clientes
        for (const auto& user : users) {
            clientList += user.second; // Agregar cada apodo
        }

        sendResponse(clientSocket, clientList);
    }

    void loginClient(int clientSocket) {
        char lengthBuffer[4];
        read(clientSocket, lengthBuffer, 4);
        int nicknameLength = stoi(string(lengthBuffer, 4));

        string nickname(nicknameLength, '\0');
        read(clientSocket, &nickname[0], nicknameLength);

        {
            lock_guard<mutex> lock(userMutex);
            users[clientSocket] = nickname; // Almacenar el apodo
        }

        sendResponse(clientSocket, "You are now logged in as " + nickname);
    }

    void logoutClient(int clientSocket) {
        {
            lock_guard<mutex> lock(userMutex);
            users.erase(clientSocket); // Eliminar el usuario del mapa
        }

        sendResponse(clientSocket, "You have logged out.");
        close(clientSocket); // Cerrar el socket del cliente
    }

    void sendGameRequest(int clientSocket) {
        char lengthBuffer[4];
        read(clientSocket, lengthBuffer, 4);
        int targetNicknameLength = stoi(string(lengthBuffer, 4));

        string targetNickname(targetNicknameLength, '\0');
        read(clientSocket, &targetNickname[0], targetNicknameLength);

        int targetSocket = -1;
        {
            lock_guard<mutex> lock(userMutex);
            for (const auto& user : users) {
                if (user.second == targetNickname) {
                    targetSocket = user.first;
                    break;
                }
            }
        }

        if (targetSocket == -1) {
            sendResponse(clientSocket, "Target player not found.");
            return;
        }

        // Calcular la longitud del nombre del usuario
        size_t userLength = users[clientSocket].length();

        // Crear el mensaje de solicitud con la longitud formateada
        string gameRequestMessage = "G" + std::to_string(userLength).insert(0, 4 - std::to_string(userLength).length(), '0') + users[clientSocket];

        // Enviar la solicitud al jugador objetivo
        sendResponse(targetSocket, gameRequestMessage);
    }

    void respondToRequest(int clientSocket) {
        char response;
        read(clientSocket, &response, 1); // Leer respuesta (y/n)

        if (response == 'y') {
            startGame(clientSocket);
        } else {
            sendResponse(clientSocket, "Game request rejected.");
        }
    }

    void startGame(int challengerSocket) {
        int opponentSocket = -1;

        // Busca al oponente
        {
            std::lock_guard<std::mutex> lock(userMutex);
            for (const auto& user : users) {
                if (user.first != challengerSocket) {
                    opponentSocket = user.first;
                    break;
                }
            }
        }

        if (opponentSocket == -1) {
            sendResponse(challengerSocket, "No available opponent.");
            return;
        }

        // Iniciar el juego
        string* initialBoard = new string("---------"); // Usar puntero normal
        games[challengerSocket] = initialBoard;
        games[opponentSocket] = initialBoard;  // Asegúrate de que ambos apunten al mismo tablero
        rival[challengerSocket] = opponentSocket;
        rival[opponentSocket] = challengerSocket;
        playerSymbols[challengerSocket] = 'X'; // X para el primer jugador
        playerSymbols[opponentSocket] = 'O'; // O para el segundo jugador

        sendResponse(challengerSocket, "T" + *initialBoard + 'X');
        sendResponse(opponentSocket, "T" + *initialBoard + 'O');
    }

    void playTurn(int clientSocket) {
    char positionBuffer[1];
    ssize_t bytesRead = read(clientSocket, positionBuffer, 1);
    if (bytesRead <= 0) {
        sendResponse(clientSocket, "Error reading turn data.");
        return;
    }

    int position = positionBuffer[0] - '1'; // Convertir a índice
    if (position < 0 || position >= 9) {
        sendResponse(clientSocket, "Invalid position. Please enter a number between 1 and 9.");
        return;
    }

    std::lock_guard<std::mutex> lock(gameMutex);
    auto it = games.find(clientSocket);
    if (it == games.end()) {
        sendResponse(clientSocket, "No active game found.");
        return;
    }

    string* board = it->second; // Puntero al tablero actual
    int opponentSocket = rival[clientSocket]; // Obtener el socket del oponente

    // Verificar que la posición esté vacía
    if ((*board)[position] == '-') { // Si la posición está vacía
        (*board)[position] = playerSymbols[clientSocket]; // Asignar símbolo del jugador
        cout << "Updated board: " << *board << endl;

        // Verificar si hay un ganador
        if (checkWinner(*board, playerSymbols[clientSocket])) {
            sendResponse(clientSocket, "W1"); // El jugador ha ganado
            sendResponse(opponentSocket, "W0"); // El oponente pierde
        } else {
            // Enviar el nuevo estado del juego a ambos jugadores
            sendResponse(opponentSocket, "T" + *board + string(1, playerSymbols[clientSocket])); // Agregar símbolo del jugador al mensaje
        }
    } else {
        sendResponse(clientSocket, "Invalid move. The position is already taken. Try again.");
    }
}

// Función para verificar si hay un ganador
bool checkWinner(const string& board, char symbol) {
    const int winningCombinations[8][3] = {
        {0, 1, 2}, // Fila 1
        {3, 4, 5}, // Fila 2
        {6, 7, 8}, // Fila 3
        {0, 3, 6}, // Columna 1
        {1, 4, 7}, // Columna 2
        {2, 5, 8}, // Columna 3
        {0, 4, 8}, // Diagonal
        {2, 4, 6}  // Diagonal
    };

    for (const auto& combination : winningCombinations) {
        if (board[combination[0]] == symbol && 
            board[combination[1]] == symbol && 
            board[combination[2]] == symbol) {
            return true;
        }
    }
    return false;
}

};

int main() {
    Server server("12346");
    server.start();

    cout << "Server is running on port 12346..." << endl;

    string command;
    while (getline(cin, command)) {
        if (command == "exit") {
            server.stop();
            break;
        }
    }

    return 0;
}
