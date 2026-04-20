#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

std::string readFile(const std::string& fileName) {
    std::ifstream file(fileName, std::ios::binary);
    if (!file.is_open()) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void handleRequest(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE] = { 0 };
    int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    
    if (bytesRead > 0) {
        std::string request(buffer);
        std::istringstream iss(request);
        std::string method, path, protocol;
        iss >> method >> path >> protocol;

        if (path == "/") path = "/index.html";
        std::string fileName = path.substr(1);

        std::string content = readFile(fileName);
        std::string response;

        if (!content.empty()) {
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + std::to_string(content.size()) + "\r\n";
            response += "Connection: close\r\n\r\n";
            response += content;
        } else {
            std::string errorMsg = "<h1>404 Not Found</h1>";
            response = "HTTP/1.1 404 Not Found\r\n";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + std::to_string(errorMsg.size()) + "\r\n";
            response += "Connection: close\r\n\r\n";
            response += errorMsg;
        }

        send(clientSocket, response.c_str(), (int)response.size(), 0);
        
        shutdown(clientSocket, SD_SEND);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server started on http://localhost:" << PORT << std::endl;

    while (true) {
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket != INVALID_SOCKET) {
            std::thread(handleRequest, clientSocket).detach();
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}