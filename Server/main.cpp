#include <thread>
#include <map>
#include <mutex>
#include "../config.h"

#pragma comment(lib, "ws2_32.lib")
using namespace std;

struct ClientData {
    int matrixSize = 0;
    int threadCount = 1;
    vector<double> matrix;
    bool isDone = false;
    bool isProcessing = false;
};

map<SOCKET, ClientData> clients;
mutex clientsMutex;

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
    cout << "[SERVER] Client connected: " << clientSocket << endl;
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
            int n;
            {
                lock_guard<mutex> lock(clientsMutex);
                n = clients[clientSocket].matrixSize;
                clients[clientSocket].matrix.resize((size_t)n * n);
            }
            for (int i = 0; i < n * n; ++i) {
                uint64_t netVal;
                recv_all(clientSocket, (char*)&netVal, sizeof(netVal));
                clients[clientSocket].matrix[i] = unpackDouble(netVal);
            }
            send(clientSocket, "DATA_OK   ", 10, 0);
        }
        else if (cmd.find("START") != string::npos) {
            {
                lock_guard<mutex> lock(clientsMutex);
                clients[clientSocket].isProcessing = true;
                clients[clientSocket].isDone = false;
            }
            thread([clientSocket]() {
                int n, t_count;
                vector<double>* mat_ptr;
                {
                    lock_guard<mutex> lock(clientsMutex);
                    n = clients[clientSocket].matrixSize;
                    t_count = clients[clientSocket].threadCount;
                    mat_ptr = &clients[clientSocket].matrix;
                }
                vector<thread> workers;
                int rpt = n / t_count;
                for (int i = 0; i < t_count; ++i) {
                    int s = i * rpt, e = (i == t_count - 1) ? n : (i + 1) * rpt;
                    if (s < n) workers.emplace_back(solvePart, mat_ptr, n, s, e);
                }
                for (auto& t : workers) t.join();
                lock_guard<mutex> lock(clientsMutex);
                clients[clientSocket].isDone = true;
                clients[clientSocket].isProcessing = false;
            }).detach();
            send(clientSocket, "STARTED   ", 10, 0);
        }
        else if (cmd.find("STATUS") != string::npos) {
            lock_guard<mutex> lock(clientsMutex);
            send(clientSocket, clients[clientSocket].isDone ? "DONE      " : "BUSY      ", 10, 0);
        }
        else if (cmd.find("RESULT") != string::npos) {
            lock_guard<mutex> lock(clientsMutex);
            for (double v : clients[clientSocket].matrix) {
                uint64_t nv = packDouble(v);
                send(clientSocket, (char*)&nv, sizeof(nv), 0);
            }
        }
    }
    lock_guard<mutex> lock(clientsMutex);
    clients.erase(clientSocket);
    closesocket(clientSocket);
    cout << "[SERVER] Client disconnected." << endl;
}

int main() {
    WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET srv = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in a = { AF_INET, htons(SERVER_PORT), INADDR_ANY };
    bind(srv, (sockaddr*)&a, sizeof(a));
    listen(srv, SOMAXCONN);
    cout << "Server started on port " << SERVER_PORT << "..." << endl;
    while (true) {
        SOCKET cl = accept(srv, nullptr, nullptr);
        if (cl != INVALID_SOCKET) thread(handleClient, cl).detach();
    }
}