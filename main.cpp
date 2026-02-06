#include <iostream>
#include <vector>
#include <random>
#include <thread>
#include <chrono>
#include <iomanip>

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

double runParallel(vector<double>& matrix, int n, int numThreads) {
    vector<thread> threads;
    threads.reserve(numThreads);

    int rowsPerThread = n / numThreads;

    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < numThreads; ++i) {
        int startRow = i * rowsPerThread;
        int endRow = (i == numThreads - 1) ? n : (i + 1) * rowsPerThread;

        threads.emplace_back(solvePart, ref(matrix), n, startRow, endRow);
    }

    for (auto& t : threads) { t.join(); }

    auto end = chrono::high_resolution_clock::now();
    return chrono::duration<double, milli>(end - start).count();
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
    vector<int> dimensions = { 100, 1000, 5000, 10000, 20000 };
    vector<int> threadCounts = { 1, 4, 8, 16, 32, 64, 128, 256 };

    cout << fixed << setprecision(4);
    cout << setw(10) << "N" << " | " << setw(8) << "Threads" << " | " << "Time (ms)" << endl;
    cout << "-----------|----------|------------" << endl;

    for (int n : dimensions) {
        vector<double> matrix((size_t)n * n);
        prepareData(matrix, n);

        for (int tc : threadCounts) {
            runParallel(matrix, n, tc);

            double time = runParallel(matrix, n, tc);

            cout << setw(10) << n << " | " << setw(8) << tc << " | " << time << " ms" << endl;
            
            volatile double checksum = matrix[0];
        }
        cout << "-----------|----------|------------" << endl;
        
        matrix.clear();
        matrix.shrink_to_fit();
    }

    return 0;
}