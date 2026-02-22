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
    for (int i = start; i < end; ++i) {
        if (data[i] != 0 && data[i] % 13 == 0) {
            lock_guard<mutex> lock(mtx);
            globalRes.sum += data[i];
            if (data[i] < globalRes.min_val) globalRes.min_val = data[i];
        }
    }
}

void solveAtomicCASPart(const vector<int>& data, int start, int end, atomic<long long>& globalSum, atomic<int>& globalMin) {
    for (int i = start; i < end; ++i) {
        if (data[i] != 0 && data[i] % 13 == 0) {
            int val = data[i];

            long long oldSum;
            do {
                oldSum = globalSum.load();
            } while (!globalSum.compare_exchange_weak(oldSum, oldSum + val));

            int oldMin;
            do {
                oldMin = globalMin.load();
                if (val >= oldMin) break; 
            } while (!globalMin.compare_exchange_weak(oldMin, val));
        }
    }
}

void prepareData(vector<int>& data, size_t n) {
    mt19937 gen(42);
    uniform_int_distribution<> dis(1, 1000000);
    for (size_t i = 0; i < n; ++i) data[i] = dis(gen);
}

int main() {
    const int tc = 4;
    const size_t start_n = 1000;      
    const size_t end_n = 10000000;    
    const size_t step = 500000;      

    cout << fixed << setprecision(3);
    cout << "-----------------------------------------------------------------------" << endl;
    cout << setw(12) << "N" << " | " << setw(12) << "Seq (ms)" << " | " << setw(12) << "Mutex (ms)" << " | " << setw(12) << "CAS (ms)" << endl;
    cout << "-------------|--------------|--------------|--------------" << endl;

    for (size_t n = start_n; n <= end_n; n += step) { 
        vector<int> data(n);
        prepareData(data, n);

        auto s1 = chrono::high_resolution_clock::now();
        Result resSeq = solveSequential(data);
        auto s2 = chrono::high_resolution_clock::now();
        double timeSeq = chrono::duration<double, milli>(s2 - s1).count();

        Result resMtx;
        vector<thread> threads;
        int chunkSize = n / tc;

        auto m1 = chrono::high_resolution_clock::now();
        for (int i = 0; i < tc; ++i) {
            int start = (int)(i * chunkSize);
            int end = (i == tc - 1) ? (int)n : (int)((i + 1) * chunkSize);
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
            int start = (int)(i * chunkSize);
            int end = (i == tc - 1) ? (int)n : (i + 1) * chunkSize;
            threads.emplace_back(solveAtomicCASPart, ref(data), start, end, ref(atomicSum), ref(atomicMin));
        }
        for (auto& t : threads) t.join();
        auto a2 = chrono::high_resolution_clock::now();
        double timeCAS = chrono::duration<double, milli>(a2 - a1).count();

        cout << setw(12) << n << " | " 
             << setw(12) << timeSeq << " | " 
             << setw(12) << timeMtx << " | " 
             << setw(12) << timeCAS << endl;

        antiOptimization += resSeq.sum + resMtx.sum + atomicSum.load();
        
        data.clear();
        data.shrink_to_fit();
    }

    cout << "-----------------------------------------------------------------------" << endl;
    cout << "Check sum: " << antiOptimization << endl;
    return 0;
}