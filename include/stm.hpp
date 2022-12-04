#ifndef TL2_STM_IMPL_H
#define TL2_STM_IMPL_H
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <vector>

using namespace std;

// Put globals here, e.g. global version clock, PS lock array
inline atomic<int64_t> global_version_clock { 0 };
// Global lock for testing
inline mutex global_lock;

class VersionedLock {
    uint64_t version;
    mutex lock;
    bool locked;

    void incrementVersion(){
        version = version + 1;
    }

public:
    VersionedLock() : version{0}, lock{}, locked{false} {}

    int64_t getVersion(){
        return version;
    }

    bool tryLock(){
        bool res = lock.try_lock();
        if(res){
            locked = true;
        }
        return res;
    }

    void unlock(){
        // TODO add a check to make sure this thread owns the lock
        lock.unlock();
        locked = false;
        // Version is advanced every successful lock release
        incrementVersion();
    }
};

// Per stripe lock array - basically just a hash map
#define NUM_LOCKS (2 << 20)
inline VersionedLock PSLocks[NUM_LOCKS];
#define GET_LOCK(addr) (PSLocks[((addr & 0x3FFFFC) + addr) % NUM_LOCKS])


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
    int64_t rv;

public:
    TxThread()
        : rv { 0 }
        , inTx(false)
        , txCount(0)
        , numLoads(0)
        , numStores(0)

    {
    }

    // Start new transaction
    void txBegin()
    {
        // Profiling/misc. info
        if (inTx) cout << "WARNING: txBegin() called but already in Tx" << endl;
        inTx = true;
        txCount++;

        // Reset from previous Tx 
        write_map.clear();

        rv = global_version_clock.load();
        global_lock.lock();
    }

    void txCommit()
    {
        for (const auto& p : write_map) {
            // cout << "writing back addr: " << p.first << " val: " << p.second << endl;
            *(p.first) = p.second;
        }
    }

    // Cleanup after Tx
    void txEnd()
    {
        if (!inTx) cout << "WARNING: txEnd() called but not in Tx" << endl;
        global_lock.unlock();
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


// Using thread local storage for some magic here - every thread automatically
// gets this _my_thread transactional context
inline thread_local TxThread _my_thread;

#ifdef USE_STM
#define LOAD(var) (TxLoad((intptr_t*)&var))
#define STORE(var, val) (TxStore((intptr_t*)&var, (intptr_t)val))

#else
#define LOAD(var) (var)
#define STORE(var, val) (var = val)
#endif

inline void TxBegin()
{
    #ifdef USE_STM
    // cout << "locking" << endl;
    _my_thread.txBegin();
    // cout << "lock acquired" << endl;
    #endif
}

inline void TxEnd()
{
    #ifdef USE_STM
    _my_thread.txCommit();
    _my_thread.txEnd();
    // cout << "unlocking" << endl;
    #endif
}

inline intptr_t TxLoad(intptr_t* addr)
{
    if (!_my_thread.inTx) {
        return *addr;
    }
    // int x = 1
    // TxLoadInt(x)
    _my_thread.numLoads++;
    if (_my_thread.write_map.find(addr) != _my_thread.write_map.end()) {
        // cout << "getting from write map: addr " << addr << " val: " << (int64_t) _my_thread.write_map[addr] << endl;
        return _my_thread.write_map[addr];
    }
    return *addr;
}

inline void TxStore(intptr_t* addr, intptr_t val)
{
    if (!_my_thread.inTx) {
        *(addr) = val;
        return;
    }
    // int x = 1
    // TxStoreInt(y, x)
    // ... y = 1
    _my_thread.numStores++;
    // cout << "setting addr: " << addr << " val: " << val << endl;
    _my_thread.write_map[addr] = val;
}

#endif