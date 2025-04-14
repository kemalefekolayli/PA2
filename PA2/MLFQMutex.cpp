#include <atomic>
#include <unordered_map>
#include <chrono>
#include <pthread.h>
#include <iostream>
#include "queue.h"
#include "park.h"

using namespace std;

class MLFQMutex {
private:
    atomic_flag guard = ATOMIC_FLAG_INIT;
    atomic<bool> is_locked = false;
    
    Garage garage;

    int max_levels;
    double Qval;
    vector<Queue<pthread_t>> queues;
    unordered_map<pthread_t, int> thread_level;
    unordered_map<pthread_t, chrono::high_resolution_clock::time_point> startTimes;

    void acquireGuard() {
        while (guard.test_and_set(memory_order_acquire)) {}
    }

    void releaseGuard() {
        guard.clear(memory_order_release);
    }

public:
    MLFQMutex(int levels, double quantum) : max_levels(levels), Qval(quantum) {
        queues.resize(max_levels);
    }

    void lock() {
        pthread_t tid = pthread_self();

        bool expected = false;
        if (is_locked.compare_exchange_weak(expected, true)) {
            startTimes[tid] = chrono::high_resolution_clock::now();
            return;
        }

        acquireGuard();
        int level = 0;
        if (thread_level.find(tid) != thread_level.end()) {
            level = thread_level[tid];
        } else {
            thread_level[tid] = 0;
        }

        cout << "Adding thread with ID: " << tid << " to level " << level << "\n";
        cout.flush();

        queues[level].enqueue(tid);
        garage.setPark();
        releaseGuard();
        garage.park();

        startTimes[tid] = chrono::high_resolution_clock::now();
    }

    void unlock() {
        pthread_t tid = pthread_self();

        auto endTime = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration<double>(endTime - startTimes[tid]).count();

        acquireGuard();

        int curr_level = thread_level[tid];
        int level_increase = static_cast<int>(elapsed / Qval);
        int new_level = min(curr_level + level_increase, max_levels - 1);
        thread_level[tid] = new_level;

        pthread_t next_tid;
        bool found = false;

        for (int lvl = 0; lvl < max_levels; ++lvl) {
            if (!queues[lvl].isEmpty()) {
                next_tid = queues[lvl].dequeue();
                found = true;
                break;
            }
        }

        if (found) {
            garage.unpark(next_tid);
        } else {
            is_locked.store(false);
        }

        releaseGuard();
    }

    void print() {
        for (int lvl = 0; lvl < max_levels; ++lvl) {
