#include <iostream>
#include <string>
#include <WinSock2.h>
#include <WS2tcpip.h>

using namespace std;

#pragma comment(lib, "ws2_32.lib") // Link against the WinSock2 library for network communication

// Function for Caesar cipher encryption
string encrypt(string message, int shift) {
    string encryptedMessage = "";
    for (char& c : message) {
        if (isalpha(c)) {
            char shiftedChar = islower(c) ? 'a' + (c - 'a' + shift) % 26 : 'A' + (c - 'A' + shift) % 26;
            encryptedMessage += shiftedChar;
        }
        else {
            encryptedMessage += c;
        }
    }
    return encryptedMessage;
}

// Function to initialize WinSock
bool InitializeWinSock() {
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        cerr << "Error: Failed to initialize Winsock." << endl;
        return false;
    }
    return true;
}

int main() {
    if (!InitializeWinSock()) {
        cout << "Error: Winsock initialization failed" << endl;
        return 1;
    }

    // Create a TCP socket
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        cout << "Error: Invalid socket created, code: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // Set up server address
    string serverAddress = "127.0.0.1";
    int port = 12345;
    sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverAddress.c_str(), &serveraddr.sin_addr) <= 0) {
        cout << "Error: Invalid address/ Address not supported" << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    // Connect to the server
    if (connect(s, reinterpret_cast<sockaddr*>(&serveraddr), sizeof(serveraddr)) == SOCKET_ERROR) {
        cout << "Error: Connection Failed, code: " << WSAGetLastError() << endl;
        closesocket(s);
        WSACleanup();
        return 1;
    }

    cout << "Info: Connected to server at " << serverAddress << ":" << port << endl;

    // Main loop for sending and receiving data
    string input;
    char buffer[4096];
    do {
        cout << "> ";
        getline(cin, input);
        if (!input.empty()) {
            // Encrypt the message using Caesar cipher with a shift of 3
            string encryptedMessage = encrypt(input, 3);

            // Send encrypted message to the server
            if (send(s, encryptedMessage.c_str(), encryptedMessage.length(), 0) == SOCKET_ERROR) {
                cout << "Error: Send failed, code: " << WSAGetLastError() << endl;
                break;
            }

            // Wait for a response from the server
            int bytesReceived = recv(s, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                // Decrypt the server's response
                string decryptedMessage = encrypt(string(buffer, bytesReceived), -3); // Decrypt with a shift of -3
                cout << "Server says: " << decryptedMessage << endl;
            }
            else if (bytesReceived == 0) {
                cout << "Server closed the connection." << endl;
                break;
            }
            else {
                cout << "Error: Receive failed, code: " << WSAGetLastError() << endl;
                break;
            }
        }
    } while (input != "exit");

    // Cleanup
    closesocket(s); // Close the socket
    WSACleanup(); // Cleanup Winsock
    cout << "Info: Connection closed." << endl;
    return 0;
}

