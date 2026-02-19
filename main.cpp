#include <iostream>
#include <vector>
#include <chrono>
#include <iomanip>
#include <random>
#include <climits>

using namespace std;

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

void prepareData(vector<int>& data, size_t n) {
    mt19937 gen(42);
    uniform_int_distribution<> dis(1, 1000000);
    for (size_t i = 0; i < n; ++i) data[i] = dis(gen);
}

int main() {
    vector<size_t> dimensions = { 10000, 100000, 1000000 };
    cout << fixed << setprecision(3);

    for (size_t n : dimensions) {
        vector<int> data(n);
        prepareData(data, n);

        auto s1 = chrono::high_resolution_clock::now();
        Result resSeq = solveSequential(data);
        auto s2 = chrono::high_resolution_clock::now();
        
        double timeSeq = chrono::duration<double, milli>(s2 - s1).count();
        cout << "N = " << n << " | Sequential: " << timeSeq << " ms | Sum: " << resSeq.sum << endl;
    }
    return 0;
}