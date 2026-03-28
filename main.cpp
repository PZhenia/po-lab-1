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
    int random_delay; 
    Task(int _id, int _delay) : id(_id), random_delay(_delay) {}
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
    atomic<bool> is_paused{false};
    bool immediate_stop = false;

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
                    return is_stopped || (!is_paused && !execution_queue.empty());
                });
                if (is_stopped && (immediate_stop || execution_queue.empty())) return;
                if (!execution_queue.empty()) {
                    task = new Task(execution_queue.front());
                    execution_queue.pop();
                }
            }
            if (task) {
                this_thread::sleep_for(chrono::seconds(task->random_delay));
                safe_print("[Worker " + to_string(thread_id) + "] completed Task " + to_string(task->id));
                delete task;
            }
        }
    }

    void scheduler_loop() {
        while (!is_stopped) {
            unique_lock<mutex> lock(queue_mtx);
            cv_scheduler.wait_for(lock, chrono::seconds(5));
            while (!incoming_queue.empty()) {
                execution_queue.push(incoming_queue.front());
                incoming_queue.pop();
            }
            cv_workers.notify_all();
        }
    }

public:
    ThreadPool(int threads_count = 6) {
        for (int i = 0; i < threads_count; ++i) workers.emplace_back(&ThreadPool::worker_loop, this, i + 1);
        scheduler_thread = thread(&ThreadPool::scheduler_loop, this);
    }

    void enqueue(Task task) {
        lock_guard<mutex> lock(queue_mtx);
        incoming_queue.push(task);
    }

    void pause() { is_paused = true; safe_print("Paused"); }
    void resume() { is_paused = false; cv_workers.notify_all(); safe_print("Resumed"); }

    void stop(bool immediate) {
        immediate_stop = immediate;
        is_stopped = true;
        cv_scheduler.notify_all();
        cv_workers.notify_all();
        if (scheduler_thread.joinable()) scheduler_thread.join();
        for (auto& w : workers) if (w.joinable()) w.join();
    }
};

int main() {
    ThreadPool pool(6);
    string cmd;
    while (cin >> cmd) {
        if (cmd == "pause") pool.pause();
        else if (cmd == "resume") pool.resume();
        else if (cmd == "stop") { pool.stop(false); break; }
        else pool.enqueue(Task(rand()%100, 2));
    }
    return 0;
}