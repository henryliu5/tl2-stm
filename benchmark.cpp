#include "include/RBTree.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <stdlib.h>
#include <chrono>

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

/**
 * @brief 
 * 
 * @param totalOps Total operations to benchmark
 * @param keyMin Min value in key range
 * @param keyMax Max value in key range
 * @param puts Proportion of puts
 * @param deletes Proportion of deletes
 * @param gets Proportion of gets
 */
void benchmark(int totalOps, int keyMin, int keyMax, double puts, double deletes, double gets){
    // RedBlackTree rb;
    RBTree rb;
    auto t1 = high_resolution_clock::now();
    for(int i = 0; i < totalOps; i++){
        int key = (rand() % (keyMax - keyMin)) + keyMin;
        double type = rand() / double(RAND_MAX);
        if(type < puts){
            rb.insert(key);
        } else if(type < puts + deletes){
            rb.deleteKey(key);
        } else{
            rb.contains(key);
        }
    }
    auto t2 = high_resolution_clock::now();
    /* Getting number of milliseconds as a double. */
    duration<double, std::milli> ms_double = t2 - t1;
    duration<double> s_double = t2 - t1;
    cout << ms_double.count() << "ms\n";
    cout << (totalOps / s_double.count()) / 1000.0 << " 1000x ops per second\n";
}

int main(){
    int N = 5000000; 
    // small bench
    benchmark(N, 100, 200, 0.05, 0.05, 0.9);
}