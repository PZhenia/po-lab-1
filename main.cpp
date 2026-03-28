#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <string>

using namespace std;

struct Task {
    int id;
    chrono::steady_clock::time_point creation_time;
    int random_delay; 

    Task(int _id, int _delay) 
        : id(_id), creation_time(chrono::steady_clock::now()), random_delay(_delay) {}
};

class ThreadPool {
private:
    vector<thread> workers;
    queue<Task> execution_queue;
    mutex queue_mtx;
    mutex cout_mtx;
    condition_variable cv_workers;
    atomic<bool> is_stopped{false};

    void safe_print(const string& msg) {
        lock_guard<mutex> lock(cout_mtx);
        cout << msg << endl;
    }

    void worker_loop(int thread_id) {
        while (true) {
            Task* task = nullptr;
            {
                unique_lock<mutex> lock(queue_mtx);
                cv_workers.wait(lock, [this] {
                    return is_stopped || !execution_queue.empty();
                });

                if (is_stopped && execution_queue.empty()) return;

                if (!execution_queue.empty()) {
                    task = new Task(execution_queue.front());
                    execution_queue.pop();
                }
            }

            if (task) {
                safe_print("[Worker " + to_string(thread_id) + "] started Task ID: " + to_string(task->id));
                this_thread::sleep_for(chrono::seconds(task->random_delay));
                safe_print("[Worker " + to_string(thread_id) + "] completed Task ID: " + to_string(task->id));
                delete task;
            }
        }
    }

public:
    ThreadPool(int threads_count = 6) {
        for (int i = 0; i < threads_count; ++i) {
            workers.emplace_back(&ThreadPool::worker_loop, this, i + 1);
        }
    }

    void enqueue(Task task) {
        {
            lock_guard<mutex> lock(queue_mtx);
            execution_queue.push(task);
        }
        cv_workers.notify_one();
    }

    ~ThreadPool() {
        is_stopped = true;
        cv_workers.notify_all();
        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }
    }
};

int main() {
    ThreadPool pool(6);
    for (int i = 1; i <= 10; ++i) {
        pool.enqueue(Task(i, 2));
        this_thread::sleep_for(chrono::milliseconds(500));
    }
    this_thread::sleep_for(chrono::seconds(10));
    return 0;
}