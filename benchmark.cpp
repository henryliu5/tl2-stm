#include "include/RBTree.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <stdlib.h>
#include <chrono>
#include <random>

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

enum OperationType { PUT, DELETE, GET };
typedef struct Operation {
    OperationType op_type;
    int64_t key;
} Operation;

/**
 * @brief Benchmarking for RB tree
 * 
 * @param totalOps Total operations to benchmark
 * @param keyMin Min value in key range
 * @param keyMax Max value in key range
 * @param puts Proportion of puts
 * @param deletes Proportion of deletes
 * @param gets Proportion of gets
 */
void benchmark(int totalOps, int numThreads, int keyMin, int keyMax, double puts, double deletes, double gets){
    RBTree rb;
    vector<Operation> ops;
    for(int i = 0; i < totalOps; i++){
        int key = (rand() % (keyMax - keyMin)) + keyMin;
        double type = rand() / double(RAND_MAX);
        if(type < puts){
            ops.push_back(Operation{PUT, key});
            // rb.insert(key);
        } else if(type < puts + deletes){
            ops.push_back(Operation{DELETE, key});
            // rb.deleteKey(key);
        } else{
            ops.push_back(Operation{GET, key});
            // rb.contains(key);
        }
    }

    assert(ops.size() == (size_t) totalOps);

    // Shuffle operations
    auto rng = default_random_engine {};
    shuffle(begin(ops), end(ops), rng);
    cout << "Starting benchmark" << endl;
    auto t1 = high_resolution_clock::now();
    vector<thread> workers;
    // Spawn threads
    for (int thread_id = 0; thread_id < numThreads; thread_id++) {
        workers.push_back(thread([&rb, thread_id, numThreads, totalOps, ops]() {
            for (int i = thread_id; i < totalOps; i += numThreads) {
                // cout << "worker " << thread_id << " doing op "  << i << "\n";
                Operation op = ops[i];
                if(op.op_type == PUT){
                    TxBegin();
                    rb.insert(op.key);
                    TxEnd();
                } else if(op.op_type == DELETE){
                    TxBegin();
                    rb.deleteKey(op.key);
                    TxEnd();
                } else if(op.op_type == GET){
                    // rb.contains(op.key);
                }

            }
        }));
    }
    // Barrier
    for_each(workers.begin(), workers.end(), [](thread& t) {
        t.join();
    });
    auto t2 = high_resolution_clock::now();
    /* Getting number of milliseconds as a double. */
    duration<double, std::milli> ms_double = t2 - t1;
    duration<double> s_double = t2 - t1;
    cout << "Threads: " << numThreads << endl;
    cout << "\t" << ms_double.count() << "ms\n";
    cout << "\t" << (totalOps / s_double.count()) / 1000.0 << " 1000x ops per second\n";
}

int main(int argc, char** argv){
    int N = 1000000; 
    // small bench
    benchmark(N, 30, 100, 200, 0.05, 0.05, 0.9);
    // benchmark(N, 4, 100, 200, 0.05, 0.05, 0.9);
    // benchmark(N, 8, 100, 200, 0.05, 0.05, 0.9);
    // benchmark(N, 16, 100, 200, 0.05, 0.05, 0.9);
    // benchmark(N, 32, 100, 200, 0.05, 0.05, 0.9);
    // largeRandThreads(N, 2, 100, 200, 0.05, 0.05, 0.9);
}