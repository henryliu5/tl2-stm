#include "include/stm.hpp"
#include <iostream>

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

TxThread::TxThread()
    : rv { 0 }
    , inTx(false)
    , txCount(0)
    , numLoads(0)
    , numStores(0)

{
}

// Start new transaction
void TxThread::txBegin()
{
    // Profiling/misc. info
    if (inTx)
        cout << "WARNING: txBegin() called but already in Tx" << endl;
    inTx = true;
    txCount++;

    // Reset from previous Tx
    write_map.clear();

    // Step 1. Sample global version-clock
    rv = global_version_clock.load();
    global_lock.lock();
}

void TxThread::txCommit()
{
    for (const auto& p : write_map) {
        // cout << "writing back addr: " << p.first << " val: " << p.second << endl;
        *(p.first) = p.second;
    }
}

void TxThread::txAbort() { }

// Cleanup after Tx
void TxThread::txEnd()
{
    if (!inTx)
        cout << "WARNING: txEnd() called but not in Tx" << endl;
    global_lock.unlock();
    inTx = false;
    write_map.clear();
}

intptr_t TxThread::txLoad(intptr_t* addr)
{
    if (!inTx) {
        return *addr;
    }
    // int x = 1
    // TxLoadInt(x)
    numLoads++;
    if (write_map.find(addr) != write_map.end()) {
        // cout << "getting from write map: addr " << addr << " val: " << (int64_t) write_map[addr] << endl;
        return write_map[addr];
    }
    return *addr;
}

void TxThread::txStore(intptr_t* addr, intptr_t val)
{
    if (!inTx) {
        *(addr) = val;
        return;
    }
    // int x = 1
    // TxStoreInt(y, x)
    // ... y = 1
    numStores++;
    // cout << "setting addr: " << addr << " val: " << val << endl;
    write_map[addr] = val;
}