#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib")  // Link against the Windows Socket library

mutex clientsMutex;                // Mutex to ensure thread-safe access to clientSockets vector
vector<SOCKET> clientSockets;      // Vector to store connected client sockets

// Function to initialize Winsock for Windows network communication
bool InitializeWinsock() {
    WSADATA data;
    int result = WSAStartup(MAKEWORD(2, 2), &data);
    if (result != 0) {
        cerr << "Error: Unable to initialize Winsock, code: " << result << endl;
        return false;
    }
    return true;
}

// Function to broadcast a message to all clients except the sender
void BroadcastMessage(const string& message, SOCKET senderSocket) {
    lock_guard<mutex> lock(clientsMutex); // Lock the mutex for thread-safe access
    for (auto& sock : clientSockets) {
        if (sock != senderSocket) {  // Ensure not sending the message back to the sender
            int sendResult = send(sock, message.c_str(), message.length(), 0);
            if (sendResult == SOCKET_ERROR) {
                cerr << "Error: Failed to send message to client, code: " << WSAGetLastError() << endl;
            }
        }
    }
}

// Function to handle incoming data from a connected client
void HandleClient(SOCKET clientSocket) {
    {
        lock_guard<mutex> lock(clientsMutex);
        clientSockets.push_back(clientSocket); // Add the new client to the list
    }

    char buffer[4096]; // Buffer to store received data from the client
    try {
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived <= 0) {
                if (bytesReceived == 0) {
                    cout << "Info: Client disconnected gracefully." << endl;
                }
                else {
                    cerr << "Error: Receive error code: " << WSAGetLastError() << endl;
                }
                break;
            }

            string message(buffer, bytesReceived);
            cout << "Received message from client: " << message << endl;
            BroadcastMessage(message, clientSocket); // Broadcast the received message to other clients
        }
    }
    catch (const exception& e) {
        cerr << "Exception in HandleClient: " << e.what() << endl;
    }

    // Cleanup after client disconnection
    {
        lock_guard<mutex> lock(clientsMutex);
        clientSockets.erase(remove(clientSockets.begin(), clientSockets.end(), clientSocket), clientSockets.end()); // Remove the client from the list
    }
    closesocket(clientSocket); // Close the client socket
    cout << "Info: Client socket closed." << endl;
}

// Main function to set up the server and accept incoming client connections
int main() {
    if (!InitializeWinsock()) {
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        cerr << "Error: Socket creation failed, code: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Error: Bind failed, code: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Error: Listen failed, code: " << WSAGetLastError() << endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    cout << "Info: Server is listening on port 12345..." << endl;
    while (true) {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket != INVALID_SOCKET) {
            thread(HandleClient, clientSocket).detach();  // Start a new thread for each connected client
        }
        else {
            cerr << "Error: Failed to accept client, code: " << WSAGetLastError() << endl;
        }
    }

    // Cleanup (theoretically unreachable)
    closesocket(listenSocket);
    WSACleanup();
    return 0;
}




