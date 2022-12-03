#ifndef TL2_STM_IMPL_H
#define TL2_STM_IMPL_H
#include <unordered_map>
#include <vector>

using namespace std;

// struct LogEntry {
//     intptr_t* addr;
//     intptr_t val;
// };
// class Log {
//     // vector<LogEntry> write_log;
//     unordered_map<intptr_t*, intptr_t> write_map;
// public:
//     void commit()
//     {
//         for (const auto& p : write_map) {
//             *(p.first) = p.second;
//         }
//     }
// };

class TxThread {
public:
    TxThread()
        : inTx(false)
        , txCount(0)
        , numLoads(0)
        , numStores(0)

    {
    }

    // Start new Tx
    void txBegin()
    {
        if (inTx)
            cout << "WARNING: already in Tx" << endl;
        inTx = true;
        txCount++;

        write_map.clear();
    }

    void txCommit()
    {
        for (const auto& p : write_map) {
            cout << "writing back addr: " << p.first << " val: " << p.second << endl;
            *(p.first) = p.second;
        }
    }

    // Cleanup after Tx
    void txEnd()
    {
        inTx = false;
        write_map.clear();
    }

    unordered_map<intptr_t*, intptr_t> write_map;
    bool inTx; // Currently no nesting
    // Profiling
    int txCount;
    int numLoads;
    int numStores;
};

// Put globals here, e.g. global version clock, PS lock array

// Using thread local storage for some magic here - every thread automatically
// gets this _my_thread transactional context
thread_local TxThread _my_thread;

#ifdef USE_STM
#define LOAD(var) (TxLoad((intptr_t*)&var))
#define STORE(var, val) (TxStore((intptr_t*)&var, (intptr_t)val))

#else
#define LOAD(var) (var)
#define STORE(var, val) (var = val)
#endif

void TxBegin()
{
    _my_thread.txBegin();
}

void TxEnd()
{
    _my_thread.txCommit();
    _my_thread.txEnd();
}

intptr_t TxLoad(intptr_t* addr)
{
    if(!_my_thread.inTx){
        return *addr;
    }
    // int x = 1
    // TxLoadInt(x)
    _my_thread.numLoads++;
    if (_my_thread.write_map.find(addr) != _my_thread.write_map.end()) {
        cout << "getting from write map: addr " << addr << " val: " << (int64_t) _my_thread.write_map[addr] << endl;
        return _my_thread.write_map[addr];
    }
    return *addr;
}

void TxStore(intptr_t* addr, intptr_t val)
{
    if(!_my_thread.inTx){
        *(addr) = val;
        return;
    }
    // int x = 1
    // TxStoreInt(y, x)
    // ... y = 1
    _my_thread.numStores++;
    cout << "setting addr: " << addr << " val: " << val << endl;
    _my_thread.write_map[addr] = val;
}

#endif