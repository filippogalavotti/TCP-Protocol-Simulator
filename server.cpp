#include <iostream>
#include <vector>
#include <thread>
#include <winsock2.h>
#include <map>
#include <algorithm>  // For std::remove

#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library

#define PORT 8080
#define MAX_CLIENTS 3

std::vector<SOCKET> clients;
std::map<SOCKET, std::string> client_names;  // Map to store client names based on their socket

// Function to assign a name based on the connection order
std::string get_client_name(int client_id) {
    return "User" + std::to_string(client_id);
}

// Function to handle each client connection
void handle_client(SOCKET client_socket, int client_id) {
    char buffer[1024];
    
    // Send the client's name upon connection
    std::string client_name = get_client_name(client_id);
    send(client_socket, client_name.c_str(), client_name.length(), 0);
    
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Client disconnected.\n";
            closesocket(client_socket);

            // Remove client from the list safely using std::remove and erase
            clients.erase(std::remove(clients.begin(), clients.end(), client_socket), clients.end());
            return;
        }

        std::string received_message(buffer);
        
        // Ensure only the message is forwarded, not the sender’s username again
        size_t separator = received_message.find(": ");
        std::string message_content = (separator != std::string::npos) ? received_message.substr(separator + 2) : received_message;

        // Broadcast the message to all other clients
        std::string full_message = client_name + ": " + message_content;
        for (SOCKET sock : clients) {
            if (sock != client_socket) {
                send(sock, full_message.c_str(), full_message.length(), 0);
            }
        }
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);  // Initialize Winsock

    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return -1;
    }

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Binding failed\n";
        return -1;
    }

    if (listen(server_socket, MAX_CLIENTS) == SOCKET_ERROR) {
        std::cerr << "Listening failed\n";
        return -1;
    }

    std::cout << "Server listening on port " << PORT << "...\n";

    int client_id = 1;

    while (clients.size() <= MAX_CLIENTS) {
        sockaddr_in client_addr;
        int addr_size = sizeof(client_addr);
        SOCKET new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
        if (new_socket == INVALID_SOCKET) {
            std::cerr << "Failed to accept client\n";
            continue;
        }

        clients.push_back(new_socket);
        std::cout << "Client " << client_id << " connected\n";

        // Assign a name to the new client and send it
        client_names[new_socket] = get_client_name(client_id);

        // Start a new thread to handle this client's communication
        std::thread(handle_client, new_socket, client_id).detach();

        client_id++;
    }

    closesocket(server_socket);
    WSACleanup();  // Clean up Winsock
    return 0;
}