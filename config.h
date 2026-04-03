#ifndef CONFIG_H
#define CONFIG_H

#include <winsock2.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

const int MATRIX_SIZE = 1000; 
const int THREAD_COUNT = 8;
const int SERVER_PORT = 8080;
const char* SERVER_IP = "127.0.0.1";

inline uint64_t packDouble(double d) {
    uint64_t res; 
    std::memcpy(&res, &d, sizeof(double)); 
    return htonll(res); 
}

inline double unpackDouble(uint64_t net_int) {
    uint64_t host_int = ntohll(net_int);
    double res; 
    std::memcpy(&res, &host_int, sizeof(double)); 
    return res;
}

inline bool recv_all(SOCKET s, char* buf, int len) {
    int total = 0;
    while (total < len) {
        int n = recv(s, buf + total, len - total, 0);
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

#endif