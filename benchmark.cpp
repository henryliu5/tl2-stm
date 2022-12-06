#include "include/RBTree.hpp"
#include "include/HashMap.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <stdlib.h>
#include <chrono>
#include <random>
#include <cstdlib>

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
void hashbenchmark(int totalOps, int numThreads, int keyMin, int keyMax, double puts, double deletes, double gets){
    HashMap m(100000);
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
        workers.push_back(thread([&m, thread_id, numThreads, totalOps, ops]() {
            for (int i = thread_id; i < totalOps; i += numThreads) {
                // cout << "worker " << thread_id << " doing op "  << i << "\n";
                Operation op = ops[i];
                if(op.op_type == PUT){
                    TxBegin();
                    m.put(op.key, 0);
                    TxEnd();
                } else if(op.op_type == DELETE){
                    TxBegin();
                    m.remove(op.key);
                    TxEnd();
                } else if(op.op_type == GET){
                    TxBegin();
                    int64_t res;
                    m.get(op.key, res);
                    TxEnd();
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
                    TxBeginReadOnly();
                    rb.get(op.key);
                    TxEnd();
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
    assert(argc == 4);
    int numThreads = atoi(argv[1]);
    bool smallBench = argv[2][0] == 's';
    bool readHeavy = argv[3][0] == 'r';

    // Num samples
    int N = 1000000; 

    int keyMin = 10000;
    int keyMax = 20000;

    if(smallBench){
        keyMin = 100;
        keyMax = 200;
    }

    if(readHeavy){
        benchmark(N, numThreads, keyMin, keyMax, 0.05, 0.05, 0.9);
    } else {
        benchmark(N, numThreads, keyMin, keyMax, 0.3, 0.3, 0.4);
    }
    

    // benchmark(N, 4, 100, 200, 0.05, 0.05, 0.9);
    // benchmark(N, 8, 100, 200, 0.05, 0.05, 0.9);
    // benchmark(N, 16, 100, 200, 0.05, 0.05, 0.9);
    // benchmark(N, 32, 100, 200, 0.05, 0.05, 0.9);
    // largeRandThreads(N, 2, 100, 200, 0.05, 0.05, 0.9);
}