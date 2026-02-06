#include <iostream>
#include <vector>
#include <random>

using namespace std;

void prepareData(vector<double>& matrix, int n) {
    mt19937 gen(42);
    uniform_real_distribution<> dis(1.0, 1.01);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            matrix[(size_t)i * n + j] = (j == n - 1 - i) ? 1.0 : dis(gen);
        }
    }
}

int main() {
    int n = 100;
    vector<double> matrix((size_t)n * n);
    prepareData(matrix, n);
    return 0;
}