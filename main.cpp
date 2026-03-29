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
#include <functional>
#include <iomanip>

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
    atomic<bool> is_paused{false};
    bool immediate_stop = false;

    atomic<int> tasks_completed{0};
    double total_wait_time = 0; 
    int total_swaps = 0;
    int total_queue_length_at_swap = 0;

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

                if (is_stopped) {
                    if (immediate_stop || execution_queue.empty()) return;
                }

                if (!execution_queue.empty()) {
                    task = new Task(execution_queue.front());
                    execution_queue.pop();
                }
            }

            if (task) {
                safe_print("[Worker " + to_string(thread_id) + "] started Task ID: " + to_string(task->id) + 
                           " (Delay: " + to_string(task->random_delay) + "s)");
                
                this_thread::sleep_for(chrono::seconds(task->random_delay));
                
                safe_print("[Worker " + to_string(thread_id) + "] completed Task ID: " + to_string(task->id));

                {
                    lock_guard<mutex> lock(queue_mtx);
                    auto now = chrono::steady_clock::now();
                    total_wait_time += chrono::duration<double>(now - task->creation_time).count();
                    tasks_completed++;
                }
                
                delete task;
            }
        }
    }

    void scheduler_loop() {
        while (!is_stopped) {
            unique_lock<mutex> lock(queue_mtx);
            cv_scheduler.wait_for(lock, chrono::seconds(60), [this] { return is_stopped.load(); });

            if (is_stopped && incoming_queue.empty()) break;

            int moved_count = 0;
            total_swaps++;
            total_queue_length_at_swap += incoming_queue.size();

            while (!incoming_queue.empty()) {
                execution_queue.push(incoming_queue.front());
                incoming_queue.pop();
                moved_count++;
            }

            safe_print("[Scheduler] Interval 60s passed. Moved tasks to execution: " + to_string(moved_count));
            cv_workers.notify_all();
        }
    }

public:
    ThreadPool(int threads_count = 6) {
        for (int i = 0; i < threads_count; ++i) {
            workers.emplace_back(&ThreadPool::worker_loop, this, i + 1);
        }
        scheduler_thread = thread(&ThreadPool::scheduler_loop, this);
        safe_print("[Pool] Created with 6 workers. Interval: 60s.");
    }

    void enqueue(Task task) {
        {
            lock_guard<mutex> lock(queue_mtx);
            incoming_queue.push(task);
        }
    }

    void pause() {
        is_paused = true;
        safe_print("[Pool] Pool paused.");
    }

    void resume() {
        is_paused = false;
        cv_workers.notify_all();
        safe_print("[Pool] Work resumed.");
    }


    void stop(bool immediate) {
        immediate_stop = immediate;
        is_stopped = true;
        cv_scheduler.notify_all();
        cv_workers.notify_all();

        if (scheduler_thread.joinable()) scheduler_thread.join();
        for (auto& w : workers) {
            if (w.joinable()) w.join();
        }
        safe_print("[Pool] Work completed.");
    }

    void print_stats() {
        lock_guard<mutex> lock(queue_mtx);
        double avg_q = total_swaps > 0 ? (double)total_queue_length_at_swap / total_swaps : 0;
        double avg_w = tasks_completed > 0 ? total_wait_time / tasks_completed : 0;

        cout << "\n--- STATISTICS ---" << endl;
        cout << "Tasks completed: " << tasks_completed << endl;
        cout << "Average queue length at swap: " << fixed << setprecision(2) << avg_q << endl;
        cout << "Average wait time in queue: " << avg_w << " sec." << endl;
        cout << "------------------\n" << endl;
    }
};

int main() {
    ThreadPool pool(6);
    atomic<bool> keep_generating{true};
    atomic<int> task_id_counter{1}; 

    vector<thread> generators;
    for (int i = 0; i < 3; ++i) {
        generators.emplace_back([&, i]() {
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> delay_dist(6, 15);
            uniform_int_distribution<> spawn_dist(1, 3); 

            while (keep_generating) {
                int count = spawn_dist(gen);
                for (int j = 0; j < count; ++j) {
                    int current_id = task_id_counter.fetch_add(1);
                    pool.enqueue(Task(current_id, delay_dist(gen)));
                }
                this_thread::sleep_for(chrono::seconds(7 + i * 2)); 
            }
        });
    }

    string command;
    cout << "Commands: pause, resume, stats, stop_graceful, stop_immediate" << endl;
    
    while (true) {
        cin >> command;
        if (command == "pause") pool.pause();
        else if (command == "resume") pool.resume();
        else if (command == "stats") pool.print_stats();
        else if (command == "stop_graceful") {
            keep_generating = false;
            pool.stop(false);
            break;
        }
        else if (command == "stop_immediate") {
            keep_generating = false;
            pool.stop(true);
            break;
        }
    }

    for (auto& g : generators) {
        if (g.joinable()) g.join();
    }
    
    pool.print_stats();

    return 0;
}
