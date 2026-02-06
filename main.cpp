#include <iostream>
#include <vector>
#include <random>
#include <thread>

using namespace std;

void solvePart(vector<double>& matrix, int n, int startRow, int endRow) {
    for (int i = startRow; i < endRow; ++i) {
        double rowProduct = 1.0;
        for (int j = 0; j < n; ++j) {
            rowProduct *= matrix[i * (size_t)n + j];
        }
        matrix[i * (size_t)n + (n - 1 - i)] = rowProduct;
    }
}

void runParallel(vector<double>& matrix, int n, int numThreads) {
    vector<thread> threads;
    int rowsPerThread = n / numThreads;

    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? n : (i + 1) * rowsPerThread;
        threads.emplace_back(solvePart, ref(matrix), n, startRow, endRow);
    }

    for (auto& t : threads) { t.join(); }
}

void prepareData(vector<double>& matrix, int n) {
    mt19937 gen(42);
    uniform_real_distribution<> dis(1.0, 3.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            matrix[(size_t)i * n + j] = (j == n - 1 - i) ? 1.0 : dis(gen);
        }
    }
}

int main() {
    vector<int> dimensions = { 100, 1000, 5000 };
    vector<int> threadCounts = { 1, 4, 8 };

    for (int n : dimensions) {
        vector<double> matrix((size_t)n * n);
        prepareData(matrix, n);
        for (int tc : threadCounts) {
            runParallel(matrix, n, tc);
        }
    }
    return 0;
}