#include <random>
#include <iomanip>
#include "../config.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

int main() {
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = { AF_INET, htons(SERVER_PORT) };
    addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) return 1;
    cout << "Connected to server!" << endl;

    char buf[11] = {0};

    send(sock, "CONFIG    ", 10, 0);
    uint32_t conf[2] = { (uint32_t)htonl(MATRIX_SIZE), (uint32_t)htonl(THREAD_COUNT) };
    send(sock, (char*)conf, sizeof(conf), 0);
    recv_all(sock, buf, 10);
    cout << "Server: " << buf << endl;

    random_device rd; mt19937 gen(rd());
    uniform_real_distribution<double> dis(1.0, 10.0);
    send(sock, "DATA      ", 10, 0);
    for(int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; ++i) {
        uint64_t nv = packDouble(dis(gen));
        send(sock, (char*)&nv, sizeof(nv), 0);
    }
    recv_all(sock, buf, 10);
    cout << "Server: " << buf << endl;

    send(sock, "START     ", 10, 0);
    recv_all(sock, buf, 10);

    while(true) {
        send(sock, "STATUS    ", 10, 0);
        recv_all(sock, buf, 10);
        if (string(buf).find("DONE") != string::npos) break;
        cout << "Waiting for server..." << endl;
        Sleep(500);
    }

    send(sock, "RESULT    ", 10, 0);
    for(int i = 0; i < MATRIX_SIZE * MATRIX_SIZE; i++) {
        uint64_t nv; recv_all(sock, (char*)&nv, sizeof(nv));
    }
    cout << "Result received successfully." << endl;

    closesocket(sock);
    WSACleanup();
    return 0;
}