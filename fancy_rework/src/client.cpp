#include <iostream>
#include <thread>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <limits>
#include <mutex>
#define SERVER_PORT 25001
#define SERVER_IP "127.0.0.1"

using namespace std;

string nickname;
bool logged_in = false;
mutex login_mutex;  // Para proteger el acceso a la variable logged_in


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

void handle_received_file(int socket) {
    char length_buffer[12] = {0}; // Buffer para longitud (5 para archivo + 5 para remitente + 11 para contenido)

    // Leer longitud del nombre del archivo (5 dígitos)
    ssize_t received = read_bytes(socket, length_buffer, 5);
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

    // Leer longitud del nombre del remitente (5 dígitos)
    received = read_bytes(socket, length_buffer, 5);
    if (received <= 0) {
        cerr << "Error reading sender nickname length." << endl;
        return;
    }
    int sender_length = stoi(length_buffer);

    // Leer nombre del remitente
    string sender_nickname(sender_length, '\0');
    received = read_bytes(socket, &sender_nickname[0], sender_length);
    if (received <= 0) {
        cerr << "Error reading sender nickname." << endl;
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

    // Guardar el contenido del archivo
    ofstream outfile(filename+"2", ios::binary);
    if (outfile.is_open()) {
        outfile.write(file_content.c_str(), file_content_length);
        outfile.close();
        cout << "File received from " << sender_nickname << " and saved as: " << filename+"2" << endl;
    } else {
        cerr << "Error opening file for writing: " << filename+"2" << endl;
    }
}
void receive_messages(int socket) {
    char response;
    while (read(socket, &response, sizeof(response)) > 0) {
        if (response == 'A') {
            logged_in = true;
            cout << "Logged in successfully!" << endl;
        } else if (response == 'R') {
            cout << "Nickname already in use. Try a different one." << endl;
            logged_in = false;
        } else if (response == 'L') {
            cout << "Logout successful." << endl;
            close(socket);
            exit(0);
        } else if (response == 'F') {
            handle_received_file(socket);
        } else {
            cerr << "Unknown response from server: " << response << endl;
        }
    }
    cerr << "Connection closed by server." << endl;
    close(socket);
    exit(1); // Exit the client if the server closes the connection
}


std::string transform_to_char_with_fixed_length(int number, int length) {
    std::ostringstream oss;
    oss << std::setw(length) << std::setfill('0') << number;
    return oss.str().substr(0, length);
}

void login_loop(int socket){
    while(true){
        cout << "Insert nickname: ";
        cin >> nickname;
        string nickname_length = transform_to_char_with_fixed_length(nickname.length(), 5);
        string final_request = "n" + nickname_length + nickname;
        write(socket, final_request.c_str(), final_request.length());

        // Esperar a que el servidor responda
        sleep(1); // Breve pausa para esperar la respuesta del servidor

        lock_guard<mutex> lock(login_mutex); // Asegurarse de que solo un hilo acceda a logged_in a la vez
        if (logged_in) {
            break; // Salir del bucle si se ha iniciado sesión con éxito
        }
    }
}

void message_request(int socket) {
    string destinatary;
    string message;

    cout << "Destinatary: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar el buffer de entrada
    getline(cin, destinatary);

    cout << "Message: ";
    getline(cin, message);

    string final_request;
    final_request += "m";

    string destinatary_length = transform_to_char_with_fixed_length(destinatary.size(), 5);
    final_request += destinatary_length + destinatary;

    string message_length = transform_to_char_with_fixed_length(message.length(), 5);
    final_request += message_length + message;

    ssize_t bytes_written = write(socket, final_request.c_str(), final_request.size());
    if (bytes_written < 0) {
        perror("write");
    } else if (static_cast<size_t>(bytes_written) != final_request.size()) {
        cerr << "Warning: Not all data was written to the socket." << endl;
    }
}

void file_request(int socket) {
    string destinatary;
    string filename;
    string file_content;

    cout << "Destinatary: ";
    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Limpiar el buffer de entrada
    getline(cin, destinatary);

    cout << "Filename: ";
    getline(cin, filename);

    ifstream file(filename, ios::in | ios::binary);
    if (!file) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    stringstream file_stream;
    file_stream << file.rdbuf();
    file_content = file_stream.str();
    file.close();

    string final_request;
    final_request += "f";

    string destinatary_length = transform_to_char_with_fixed_length(destinatary.size(), 5);
    final_request += destinatary_length + destinatary;

    string filename_length = transform_to_char_with_fixed_length(filename.length(), 5);
    final_request += filename_length + filename;

    string file_content_length = transform_to_char_with_fixed_length(file_content.length(), 11);
    final_request += file_content_length + file_content;

    ssize_t bytes_written = write(socket, final_request.c_str(), final_request.size());
    if (bytes_written < 0) {
        perror("write");
    } else if (static_cast<size_t>(bytes_written) != final_request.size()) {
        cerr << "Warning: Not all data was written to the socket." << endl;
    }
}

void logout_request(int socket) {
    string final_request = "o";
    ssize_t bytes_written = write(socket, final_request.c_str(), final_request.size());
    if (bytes_written < 0) {
        perror("write");
    }
    close(socket);
    exit(0);
}

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);

    if (connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_socket);
        return 1;
    }

    std::thread(receive_messages, client_socket).detach();
    login_loop(client_socket);

    while (true) {
        char request;
        cout << "Available requests\nMessage: m\nBroadcast: b\nList of users: l\nFile: f\nLogout: o\n";
        cin >> request;

        switch (request) {
        case 'm':
            message_request(client_socket);
            break;
        case 'b':
            cout << "Broadcast request not implemented yet." << endl;
            break;
        case 'l':
            cout << "List of users request not implemented yet." << endl;
            break;
        case 'f':
            file_request(client_socket);
            break;
        case 'o':
            logout_request(client_socket);
            break;
        default:
            cout << "Invalid request." << endl;
            break;
        }
    }

    close(client_socket);
    return 0;
}
