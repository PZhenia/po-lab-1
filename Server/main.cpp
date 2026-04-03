#include <iostream>
#include <winsock2.h>
#include <vector>
#include <string>
#include <thread>
#include <map>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")
using namespace std;

const int SERVER_PORT = 8080;

struct ClientData {
    int matrixSize = 0;
    int threadCount = 1;
    vector<double> matrix;
    bool isDone = false;
};

map<SOCKET, ClientData> clients;
mutex clientsMutex;

uint64_t packDouble(double d) {
    uint64_t res; 
    memcpy(&res, &d, sizeof(double)); 
    return htonll(res); 
}

double unpackDouble(uint64_t net_int) {
    uint64_t host_int = ntohll(net_int);
    double res; 
    memcpy(&res, &host_int, sizeof(double)); 
    return res;
}

bool recv_all(SOCKET s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

void solvePart(vector<double>* matrix, int n, int startRow, int endRow) {
    for (int i = startRow; i < endRow; ++i) {
        double rowProduct = 1.0;
        for (int j = 0; j < n; ++j) {
            if (j != (n - 1 - i)) rowProduct *= (*matrix)[(size_t)i * n + j];
        }
        (*matrix)[(size_t)i * n + (n - 1 - i)] = rowProduct;
    }
}

void handleClient(SOCKET clientSocket) {
    char cmdBuf[11] = {0};
    while (true) {
        if (!recv_all(clientSocket, cmdBuf, 10)) break;
        string cmd(cmdBuf);

        if (cmd.find("CONFIG") != string::npos) {
            uint32_t config[2];
            if (recv_all(clientSocket, (char*)config, sizeof(config))) {
                lock_guard<mutex> lock(clientsMutex);
                clients[clientSocket].matrixSize = ntohl(config[0]);
                clients[clientSocket].threadCount = ntohl(config[1]);
                send(clientSocket, "CONFIG_OK ", 10, 0);
            }
        }
        else if (cmd.find("DATA") != string::npos) {
            int n = clients[clientSocket].matrixSize;
            clients[clientSocket].matrix.resize((size_t)n * n);
            for (int i = 0; i < n * n; ++i) {
                uint64_t netVal;
                recv_all(clientSocket, (char*)&netVal, sizeof(netVal));
                clients[clientSocket].matrix[i] = unpackDouble(netVal);
            }
            send(clientSocket, "DATA_OK   ", 10, 0);
        }
        else if (cmd.find("START") != string::npos) {
            int n = clients[clientSocket].matrixSize;
            solvePart(&clients[clientSocket].matrix, n, 0, n); 
            clients[clientSocket].isDone = true;
            send(clientSocket, "STARTED   ", 10, 0);
        }
    }
    closesocket(clientSocket);
}

int main() {
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a = { AF_INET, htons(SERVER_PORT), INADDR_ANY };
    bind(srv, (sockaddr*)&a, sizeof(a));
    listen(srv, SOMAXCONN);

    cout << "Server multithreaded started..." << endl;
    while (true) {
        SOCKET cl = accept(srv, nullptr, nullptr);
        if (cl != INVALID_SOCKET) thread(handleClient, cl).detach();
    }
}