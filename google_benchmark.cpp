#include "include/RBTree.hpp"
#include "include/HashMap.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <benchmark/benchmark.h>

static void BM_PlainInsert(benchmark::State& state) {
    RBTree rb;
    int keyMin = 10000;
    int keyMax = 20000;
    for (auto _ : state){
        int key = (rand() % (keyMax - keyMin)) + keyMin;
        rb.insert(key);
    }
}

static void BM_InstrumentedInsert(benchmark::State& state) {
    RBTree rb;
    int keyMin = 10000;
    int keyMax = 20000;
    for (auto _ : state){
        TxBegin();
        int key = (rand() % (keyMax - keyMin)) + keyMin;
        rb.insert(key);
        TxEnd();
    }
}


BENCHMARK(BM_PlainInsert);
BENCHMARK(BM_InstrumentedInsert);

BENCHMARK_MAIN();