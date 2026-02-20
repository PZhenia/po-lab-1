#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include <climits>
#include <thread>
#include <mutex>
#include <atomic>

using namespace std;

long long antiOptimization = 0;

struct Result {
    long long sum = 0;
    int min_val = INT_MAX;
};

Result solveSequential(const vector<int>& data) {
    long long s = 0;
    int m = INT_MAX;
    for (int x : data) {
        if (x != 0 && x % 13 == 0) {
            s += x;
            if (x < m) m = x;
        }
    }
    return {s, m};
}

mutex mtx;
void solveMutexPart(const vector<int>& data, int start, int end, Result& globalRes) {
    long long localSum = 0;
    int localMin = INT_MAX;

    for (int i = start; i < end; ++i) {
        if (data[i] != 0 && data[i] % 13 == 0) {
            localSum += data[i];
            if (data[i] < localMin) localMin = data[i];
        }
    }

    lock_guard<mutex> lock(mtx);
    globalRes.sum += localSum;
    if (localMin < globalRes.min_val) globalRes.min_val = localMin;
}

void solveAtomicCASPart(const vector<int>& data, int start, int end, atomic<long long>& globalSum, atomic<int>& globalMin) {
    long long localSum = 0;
    int localMin = INT_MAX;

    for (int i = start; i < end; ++i) {
        if (data[i] != 0 && data[i] % 13 == 0) {
            localSum += data[i];
            if (data[i] < localMin) localMin = data[i];
        }
    }

    if (localSum != 0) {
        long long oldSum;
        do {
            oldSum = globalSum.load();
        } while (!globalSum.compare_exchange_weak(oldSum, oldSum + localSum));
    }

    if (localMin != INT_MAX) {
        int oldMin;
        do {
            oldMin = globalMin.load();
            if (localMin >= oldMin) break;
        } while (!globalMin.compare_exchange_weak(oldMin, localMin));
    }
}

void prepareData(vector<int>& data, size_t n) {
    mt19937 gen(42);
    uniform_int_distribution<> dis(1, 1000000);
    for (size_t i = 0; i < n; ++i) data[i] = dis(gen);
}

int main() {
    vector<size_t> dimensions = { 10000, 100000, 1000000, 10000000, 100000000 };
    vector<int> threadCounts = { 4, 8, 16, 32, 64, 128, 256 };

    cout << fixed << setprecision(3);

    for (size_t n : dimensions) {
        vector<int> data(n);
        prepareData(data, n);

        auto s1 = chrono::high_resolution_clock::now();
        Result resSeq = solveSequential(data);
        auto s2 = chrono::high_resolution_clock::now();
        double timeSeq = chrono::duration<double, milli>(s2 - s1).count();

        cout << "\n>>> N = " << n << " | Sequential: " << timeSeq << " ms" << endl;
        cout << setw(10) << "Threads" << " | " << setw(15) << "Mutex (ms)" << " | " << setw(15) << "CAS (ms)" << endl;
        cout << "-----------|-----------------|-----------------" << endl;

        for (int tc : threadCounts) {

            Result resMtx;
            vector<thread> threads;
            int chunkSize = n / tc;

            auto m1 = chrono::high_resolution_clock::now();
            for (int i = 0; i < tc; ++i) {
                int start = i * chunkSize;
                int end = (i == tc - 1) ? (int)n : (i + 1) * chunkSize;
                threads.emplace_back(solveMutexPart, ref(data), start, end, ref(resMtx));
            }
            for (auto& t : threads) t.join();
            auto m2 = chrono::high_resolution_clock::now();
            double timeMtx = chrono::duration<double, milli>(m2 - m1).count();

            atomic<long long> atomicSum(0);
            atomic<int> atomicMin(INT_MAX);
            threads.clear();

            auto a1 = chrono::high_resolution_clock::now();
            for (int i = 0; i < tc; ++i) {
                int start = i * chunkSize;
                int end = (i == tc - 1) ? (int)n : (i + 1) * chunkSize;
                threads.emplace_back(solveAtomicCASPart, ref(data), start, end, ref(atomicSum), ref(atomicMin));
            }
            for (auto& t : threads) t.join();
            auto a2 = chrono::high_resolution_clock::now();
            double timeCAS = chrono::duration<double, milli>(a2 - a1).count();

            cout << setw(10) << tc << " | " 
                 << setw(15) << timeMtx << " | " 
                 << setw(15) << timeCAS << endl;

            antiOptimization += resSeq.sum + resMtx.sum + atomicSum.load();
        }
        cout << "-----------------------------------------------" << endl;
        
        data.clear();
        data.shrink_to_fit();
    }

    cout << "\nCheck sum: " << antiOptimization << endl;
    return 0;
}