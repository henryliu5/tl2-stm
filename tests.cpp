#include "include/HashMap.hpp"
#include "include/RBTree.hpp"
#include <algorithm>
#include <iostream>
#include <random>
#include <thread>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <cstdlib>
#include "include/stm.hpp"

using namespace std;
int failures = 0;


namespace RBTreeTests {
void checkOrderAndSize(unordered_set<int64_t>& s, RBTree& rb)
{
    // See if the size is right
    if (s.size() != rb.size()) {
        cout << "RBTree and set have different sizes: " << s.size() << " " << rb.size() << endl;
        failures++;
    }
    vector<int> v(s.begin(), s.end());
    sort(v.begin(), v.end());
    // See if order is maintained
    if (rb.inorder() != v) {
        cout << "Failed order" << endl;
        failures++;
    }

    int height = rb.maxHeight();
    if(v.size() != 0 && height >= 2 * log2(v.size() + 1)){
        cout << "Tree imbalanced, got height: " << height << " expected < " << 2 * log2(v.size() + 1) << " total nodes: " << v.size() << endl;
        failures++;
    }
}

void largeRand()
{
    // Do a bunch of random insertions
    cout << "Starting large rand" << endl;
    unordered_set<int64_t> s;
    RBTree rb;
    int N = 100000;
    for (int i = 0; i < N; i++) {
        TxBegin();
        int val = rand() % N;
        rb.insert(val);
        TxEnd();
        s.insert(val);
    }
    checkOrderAndSize(s, rb);

    // Now do a bunch of random deletions
    for (int i = 0; i < N / 10; i++) {
        TxBegin();
        int val = rand() % N;
        bool deleteRes = rb.deleteKey(val);
        TxEnd();
        if (deleteRes != (s.count(val) == 1)) {
            cout << "Tree and set deletion disagree key " << val << ", tree: " << deleteRes << " set: " << (s.count(val) == 1 )<< endl;
            // cout << "deleted " << val << " from tree but not found in set" << endl;
            failures++;
        }
        s.erase(val);
    }
    checkOrderAndSize(s, rb);

    // Check containment of whatever remains
    for (int i = 0; i < N; i++) {
        if (s.count(i) == 1) {
            if (!rb.get(i)) {
                cout << "set contained, but rb tree didn't: " << i << endl;
                failures++;
            }
        }
    }
}

void largeRandThreads(int numInserts, int numDeletes, int numThreads)
{
    // Do a bunch of random insertions
    cout << "Starting large rand with " << numThreads << " threads" << endl;
    const int64_t keyMax = 20000;
    const int64_t keyMin = 10000;
    vector<pair<int64_t, int64_t>> insert_ops;
    unordered_map<int64_t, int64_t> base_map;
    RBTree rb;
    cout << "Starting insert phase" << endl;
    { // Insert test
        // Generate insert operations
        for (int i = 0; i < numInserts; i++) {
            int key = (rand() % (keyMax - keyMin)) + keyMin;
            // int64_t key = i % 1000;
            int64_t val = rand();
            insert_ops.push_back(make_pair(key, val));
            base_map[key] = val;
        }
        // Shuffle key value operations
        auto rng = default_random_engine {};
        shuffle(begin(insert_ops), end(insert_ops), rng);

        vector<thread> workers;
        // Spawn threads
        for (int thread_id = 0; thread_id < numThreads; thread_id++) {
            workers.push_back(thread([&rb, thread_id, numThreads, numInserts, insert_ops]() {
                _thread_id = thread_id;
                for (int i = thread_id; i < numInserts; i += numThreads) {
                    TxBegin();
                    rb.insert(insert_ops[i].first);
                    TxEnd();
                }
            }));
        }
        // Barrier
        for_each(workers.begin(), workers.end(), [](thread& t) {
            t.join();
        });
        unordered_set<int64_t> res;
        for(auto& p: base_map){
            res.insert(p.first);
        }
        checkOrderAndSize(res, rb);
    }
    cout << "Starting delete phase" << endl;
    { // Delete tests
        vector<int64_t> delete_ops;
        for (int i = 0; i < numDeletes; i++) {
            int key = (rand() % (keyMax - keyMin)) + keyMin;
            delete_ops.push_back(key);
            base_map.erase(key);
        }
        // Shuffle key value operations
        auto rng = default_random_engine {};
        shuffle(begin(insert_ops), end(insert_ops), rng);

        vector<thread> workers;
        // Spawn threads
        for (int thread_id = 0; thread_id < numThreads; thread_id++) {
            workers.push_back(thread([&rb, thread_id, numThreads, numDeletes, delete_ops]() {
                for (int i = thread_id; i < numDeletes; i += numThreads) {
                    TxBegin();
                    rb.deleteKey(delete_ops[i]);
                    TxEnd();
                }
            }));
        }
        // Barrier
        for_each(workers.begin(), workers.end(), [](thread& t) {
            t.join();
        });
        unordered_set<int64_t> res;
        for(auto& p: base_map){
            res.insert(p.first);
        }
        checkOrderAndSize(res, rb);
    }
}

void smallSimple()
{
    RBTree rb;
    rb.insert(5);
    rb.insert(3);
    rb.insert(-1);
    rb.insert(6);
    if (rb.size() != 4) {
        cout << "Failed size check" << endl;
        failures++;
    }
    if (rb.inorder() != vector<int> { -1, 3, 5, 6 }) {
        cout << "Out of order" << endl;
        failures++;
    }
    rb.deleteKey(5);
    if (rb.inorder() != vector<int> { -1, 3, 6 }) {
        cout << "Delete failed" << endl;
        failures++;
    }
}
}

namespace HashMapTests {
void checkCorrect(const unordered_map<int64_t, int64_t>& base, HashMap& m)
{
    for (const auto& p : base) {
        int64_t res = -1;
        if (!m.get(p.first, res)) {
            cout << "HashMap did not contain key " << p.first << endl;
            failures++;
        } else if (res != p.second) {
            cout << "HashMap contained incorrect value" << endl;
            cout << "key: " << p.first << " expected v: " << p.second << " got " << res << endl;
            failures++;
        }
    }
}

void largeRand()
{
    // Do a bunch of random insertions
    cout << "Starting large rand" << endl;
    unordered_map<int64_t, int64_t> base_map;
    int N = 100000;
    HashMap m(N / 100);
    for (int i = 0; i < N; i++) {
        TxBegin();
        int64_t key = rand() % N;
        int64_t val = rand();
        base_map[key] = val;
        m.put(key, val);
        TxEnd();
    }
    checkCorrect(base_map, m);

    // Now do a bunch of random deletions
    for (int i = 0; i < N / 10; i++) {
        TxBegin();
        int key = rand() % N;
        m.remove(key);
        base_map.erase(key);
        TxEnd();
    }
    checkCorrect(base_map, m);
}

void largeRandThreads(int numInserts, int numDeletes, int numThreads)
{
    // Do a bunch of random insertions
    cout << "Starting large rand with " << numThreads << " threads" << endl;
    vector<pair<int64_t, int64_t>> insert_ops;

    unordered_map<int64_t, int64_t> base_map;
    HashMap m(10000);
    cout << "Starting insert phase" << endl;
    { // Insert test
        // Generate insert operations
        for (int i = 0; i < numInserts; i++) {
            int64_t key = i;
            int64_t val = rand();
            insert_ops.push_back(make_pair(key, val));
            base_map[key] = val;
        }
        // Shuffle key value operations
        auto rng = default_random_engine {};
        shuffle(begin(insert_ops), end(insert_ops), rng);

        vector<thread> workers;
        // Spawn threads
        for (int thread_id = 0; thread_id < numThreads; thread_id++) {
            workers.push_back(thread([&m, thread_id, numThreads, numInserts, insert_ops]() {
                for (int i = thread_id; i < numInserts; i += numThreads) {
                    TxBegin();
                    m.put(insert_ops[i].first, insert_ops[i].second);
                    TxEnd();
                }
            }));
        }
        // Barrier
        for_each(workers.begin(), workers.end(), [](thread& t) {
            t.join();
        });
        checkCorrect(base_map, m);
    }
    cout << "Starting delete phase" << endl;
    { // Delete tests
        vector<int64_t> delete_ops;
        for (int i = 0; i < numDeletes; i++) {
            int64_t key = rand() % numInserts;
            delete_ops.push_back(key);
            base_map.erase(key);
        }
        // Shuffle key value operations
        auto rng = default_random_engine {};
        shuffle(begin(insert_ops), end(insert_ops), rng);

        vector<thread> workers;
        // Spawn threads
        for (int thread_id = 0; thread_id < numThreads; thread_id++) {
            workers.push_back(thread([&m, thread_id, numThreads, numDeletes, delete_ops]() {
                for (int i = thread_id; i < numDeletes; i += numThreads) {
                    TxBegin();
                    m.remove(delete_ops[i]);
                    TxEnd();
                }
            }));
        }
        // Barrier
        for_each(workers.begin(), workers.end(), [](thread& t) {
            t.join();
        });
        checkCorrect(base_map, m);
    }
}
}

int main()
{
    srand(time(NULL));
    // RB Tree tests
    cout << "Starting RB Tree tests" << endl;
    #ifndef USE_STM
    RBTreeTests::smallSimple();
    RBTreeTests::largeRand();
    #endif
    #ifdef USE_STM
    RBTreeTests::largeRandThreads(1000000, 1000000, 30);
    #endif

    // // HashMap tests
    // cout << "Starting HashMap tests" << endl;
    // #ifndef USE_STM
    // HashMapTests::largeRand();
    // #endif
    // #ifdef USE_STM
    // HashMapTests::largeRandThreads(100000, 100000, 30);
    // #endif

    if (failures > 0) {
        cout << "\nFailed " << failures << " tests!!!" << endl;
        exit(1);
    } else {
        cout << "\nPassed all tests" << endl;
    }
}