#include <iostream>
#include <thread>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>  // Needed for inet_pton()
#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080

std::mutex print_mutex;

void receive_messages(SOCKET sock, std::string username) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Disconnected from server.\n";
            closesocket(sock);
            exit(0);
        }

        std::string message = buffer;

        // Lock output to prevent overlapping text
        {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "\r\33[2K";  // Clear the current input line
            std::cout << message << "\n";

            // Reprint "Enter Message:" prompt
            std::cout << "Enter Message: ";
            std::cout.flush();
        }
    }
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Winsock initialization failed\n";
        return -1;
    }

    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed\n";
        return -1;
    }

    std::cout << "Connected to server.\n\n";

    // Receive the assigned username
    char username_buffer[1024];
    memset(username_buffer, 0, sizeof(username_buffer));
    recv(client_socket, username_buffer, sizeof(username_buffer), 0);
    std::string username(username_buffer);

    std::cout << "You are " << username << "\n\n";

    // Start receiving messages in a separate thread
    std::thread recv_thread(receive_messages, client_socket, username);
    recv_thread.detach();

    std::cout << "Enter Message: ";
    std::cout.flush();

    std::string message;
    while (true) {

        std::getline(std::cin, message);
        if (message.empty()) continue;

        // Clear the input prompt before printing the sent message
        std::cout << "\033[A\033[2K";  // Clears the current line

        // Send message
        std::string full_message = username + ": " + message;
        send(client_socket, full_message.c_str(), full_message.size(), 0);

        // Print the sent message with correct spacing
        std::cout << username << ": " << message << " (you)\n";

        // Reprint "Enter Message:" prompt
        std::cout << "Enter Message: ";
        std::cout.flush();
    }

    closesocket(client_socket);
    WSACleanup();
    return 0;
}