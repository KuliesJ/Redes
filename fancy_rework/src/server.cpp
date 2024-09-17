#include <iostream>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <cstring>
#include <map>
#include <iomanip>
#include <fstream>
#include <sstream>

#define SERVER_PORT 25001
#define SERVER_IP "127.0.0.1"

using namespace std;

map<string, int> nick_to_socket;
map<int, string> socket_to_nick;

// Función para leer bytes con manejo de errores
ssize_t read_bytes(int socket, char* buffer, size_t length) {
    ssize_t total_received = 0;
    while (total_received < length) {
        ssize_t received = read(socket, buffer + total_received, length - total_received);
        if (received <= 0) {
            return received; // Error o cierre de conexión
        }
        total_received += received;
    }
    return total_received;
}

std::string transform_to_char_with_fixed_length(int number, int length) {
    std::ostringstream oss;
    oss << std::setw(length) << std::setfill('0') << number;
    return oss.str().substr(0, length);
}

void handle_file(int socket) {
    char length_buffer[12] = {0}; // Buffer para longitud (5 para destinatario + 5 para archivo + 5 para remitente + 11 para contenido)

    // Leer longitud del destinatario (5 dígitos)
    ssize_t received = read_bytes(socket, length_buffer, 5);
    if (received <= 0) {
        cerr << "Error reading recipient length." << endl;
        return;
    }
    int recipient_length = stoi(length_buffer);

    // Leer nombre del destinatario
    string recipient(recipient_length, '\0');
    received = read_bytes(socket, &recipient[0], recipient_length);
    if (received <= 0) {
        cerr << "Error reading recipient name." << endl;
        return;
    }

    // Leer longitud del nombre del archivo (5 dígitos)
    received = read_bytes(socket, length_buffer, 5);
    if (received <= 0) {
        cerr << "Error reading filename length." << endl;
        return;
    }
    int filename_length = stoi(length_buffer);

    // Leer nombre del archivo
    string filename(filename_length, '\0');
    received = read_bytes(socket, &filename[0], filename_length);
    if (received <= 0) {
        cerr << "Error reading filename." << endl;
        return;
    }

    // Leer longitud del contenido del archivo (11 dígitos)
    received = read_bytes(socket, length_buffer, 11);
    if (received <= 0) {
        cerr << "Error reading file content length." << endl;
        return;
    }
    int file_content_length = stoi(length_buffer);

    // Leer contenido del archivo
    string file_content(file_content_length, '\0');
    received = read_bytes(socket, &file_content[0], file_content_length);
    if (received <= 0) {
        cerr << "Error reading file content." << endl;
        return;
    }

    // Obtener el nombre del remitente del mapa socket_to_nickname
    auto it = socket_to_nick.find(socket);
    if (it == socket_to_nick.end()) {
        cerr << "Sender nickname not found for socket: " << socket << endl;
        return;
    }
    string sender_nickname = it->second;

    // Buscar el socket del destinatario en el mapa
    auto recipient_it = nick_to_socket.find(recipient);
    if (recipient_it == nick_to_socket.end()) {
        cerr << "Recipient not found: " << recipient << endl;
        return;
    }

    int recipient_socket = recipient_it->second;

    // Construir el mensaje para enviar al destinatario
    string final_request;
    final_request += "F";

    string filename_length_str = transform_to_char_with_fixed_length(filename.length(), 5);
    final_request += filename_length_str + filename;

    string sender_length_str = transform_to_char_with_fixed_length(sender_nickname.length(), 5);
    final_request += sender_length_str + sender_nickname;

    string file_content_length_str = transform_to_char_with_fixed_length(file_content.length(), 11);
    final_request += file_content_length_str + file_content;

    // Enviar el archivo al socket del destinatario
    ssize_t bytes_sent = write(recipient_socket, final_request.c_str(), final_request.size());
    if (bytes_sent < 0) {
        perror("write");
    } else if (static_cast<size_t>(bytes_sent) != final_request.size()) {
        cerr << "Warning: Not all data was written to the recipient's socket." << endl;
    }

    cout << "File sent to: " << recipient << endl;
}



void handle_nickname(int socket) {
    char length_buffer[6] = {0}; // Buffer para longitud (5 dígitos + null terminator)
    ssize_t received = read_bytes(socket, length_buffer, 5);
    if (received <= 0) {
        cerr << "Error reading nickname length." << endl;
        return;
    }
    int nickname_length = stoi(length_buffer);

    string nickname(nickname_length, '\0');
    received = read_bytes(socket, &nickname[0], nickname_length);
    if (received <= 0) {
        cerr << "Error reading nickname." << endl;
        return;
    }

    auto it = nick_to_socket.find(nickname);
    if (it == nick_to_socket.end()) {
        nick_to_socket[nickname] = socket;
        socket_to_nick[socket] = nickname;

        char response = 'A'; // 'A' para "aceptado"
        ssize_t sent = write(socket, &response, sizeof(response));
        if (sent <= 0) {
            cerr << "Error sending response to client." << endl;
        } else {
            cout << "Nickname accepted: " << nickname << endl;
        }
    } else {
        char response = 'R'; // 'R' para "rechazado"
        ssize_t sent = write(socket, &response, sizeof(response));
        if (sent <= 0) {
            cerr << "Error sending response to client." << endl;
        } else {
            cout << "Nickname rejected: " << nickname << endl;
        }
    }
}

void handle_logout(int socket) {
    auto it = socket_to_nick.find(socket);
    if (it != socket_to_nick.end()) {
        string nickname = it->second;
        socket_to_nick.erase(it);
        nick_to_socket.erase(nickname);

        char response = 'L'; // 'L' para "logout exitoso"
        ssize_t sent = write(socket, &response, sizeof(response));
        if (sent <= 0) {
            cerr << "Error sending logout confirmation to client." << endl;
        } else {
            cout << "User " << nickname << " logged out successfully." << endl;
        }
    } else {
        cerr << "Socket not found in socket_to_nick map." << endl;
    }

    close(socket);
}

void handle_client(int client_socket) {
    char instruction_received;
    while (true) {
        ssize_t bytes_received = read(client_socket, &instruction_received, sizeof(instruction_received));
        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                perror("read");
            } else {
                cout << "Connection closed or no data received." << endl;
            }
            break;
        }

        switch (instruction_received) {
        case 'n':
            handle_nickname(client_socket);
            break;
        case 'f':
            handle_file(client_socket);
            break;
        case 'o':
            handle_logout(client_socket);
            return; // Salir después de logout
        default:
            cout << "Received unknown instruction: " << instruction_received << endl;
            break;
        }
    }
    close(client_socket);
}

int main() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return 1;
    }

    if (listen(server_socket, 5) < 0) {
        perror("listen");
        close(server_socket);
        return 1;
    }

    cout << "Server is listening on " << SERVER_IP << ":" << SERVER_PORT << endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        // Crear un hilo para manejar al cliente
        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}
