#include <iostream>
#include <winsock2.h>
#include <vector>
#include <random>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 8080;
const int MATRIX_SIZE = 5; 

uint64_t packDouble(double d) {
    uint64_t res; memcpy(&res, &d, sizeof(double)); return htonll(res); 
}

int main() {
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = { AF_INET, htons(SERVER_PORT) };
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) return 1;

    send(sock, "CONFIG    ", 10, 0);
    uint32_t conf[2] = { (uint32_t)htonl(MATRIX_SIZE), (uint32_t)htonl(1) };
    send(sock, (char*)conf, sizeof(conf), 0);
    char buf[11] = {0};
    recv(sock, buf, 10, 0);
    cout << "Config: " << buf << endl;

    send(sock, "DATA      ", 10, 0);
    for(int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; ++i) {
        uint64_t nv = packDouble(1.5 + i); 
        send(sock, (char*)&nv, sizeof(nv), 0);
    }
    recv(sock, buf, 10, 0);
    cout << "Data: " << buf << endl;

    send(sock, "START     ", 10, 0);
    recv(sock, buf, 10, 0);
    cout << "Computation: " << buf << endl;

    closesocket(sock);
    WSACleanup();
    return 0;
}