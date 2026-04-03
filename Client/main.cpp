#include <iostream>
#include <winsock2.h>
#include <string>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

const char* IP = "127.0.0.1";
const int PORT = 8080;

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);

    cout << "Attempting to connect to " << IP << ":" << PORT << "..." << endl;

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0) {
        cout << "Connected!" << endl;

        send(sock, "HELLO     ", 10, 0);

        char buf[11] = {0};
        recv(sock, buf, 10, 0);
        cout << "Server replied: " << buf << endl;
    } else {
        cout << "Connection failed." << endl;
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}