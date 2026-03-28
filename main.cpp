#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <string>
#include <random>

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
    thread scheduler_thread;
    queue<Task> incoming_queue; 
    queue<Task> execution_queue;  
    mutex queue_mtx;
    mutex cout_mtx;
    condition_variable cv_workers;
    condition_variable cv_scheduler;
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
                this_thread::sleep_for(chrono::seconds(task->random_delay));
                safe_print("[Worker " + to_string(thread_id) + "] completed Task ID: " + to_string(task->id));
                delete task;
            }
        }
    }

    void scheduler_loop() {
        while (!is_stopped) {
            unique_lock<mutex> lock(queue_mtx);
            cv_scheduler.wait_for(lock, chrono::seconds(10)); 
            while (!incoming_queue.empty()) {
                execution_queue.push(incoming_queue.front());
                incoming_queue.pop();
            }
            cv_workers.notify_all();
        }
    }

public:
    ThreadPool(int threads_count = 6) {
        for (int i = 0; i < threads_count; ++i) {
            workers.emplace_back(&ThreadPool::worker_loop, this, i + 1);
        }
        scheduler_thread = thread(&ThreadPool::scheduler_loop, this);
    }

    void enqueue(Task task) {
        lock_guard<mutex> lock(queue_mtx);
        incoming_queue.push(task);
    }

    ~ThreadPool() {
        is_stopped = true;
        cv_scheduler.notify_all();
        cv_workers.notify_all();
        if (scheduler_thread.joinable()) scheduler_thread.join();
        for (auto& w : workers) if (w.joinable()) w.join();
    }
};

int main() {
    ThreadPool pool(6);
    for (int i = 1; i <= 15; ++i) {
        pool.enqueue(Task(i, 2));
    }
    this_thread::sleep_for(chrono::seconds(30));
    return 0;
}