#include <iostream>
#include <winsock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int PORT = 8080;

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(srv, (sockaddr*)&addr, sizeof(addr));
    listen(srv, 1);

    cout << "Server started on port " << PORT << ". Waiting for client..." << endl;

    SOCKET cl = accept(srv, nullptr, nullptr);
    if (cl != INVALID_SOCKET) {
        cout << "Client connected!" << endl;
        
        char buffer[1024] = {0};
        recv(cl, buffer, 1024, 0);
        cout << "Received from client: " << buffer << endl;

        send(cl, "HELLO_OK  ", 10, 0);
    }

    closesocket(cl);
    closesocket(srv);
    WSACleanup();
    return 0;
}