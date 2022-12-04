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

class TxThread {
    int64_t rv;

    void txAbort();
    
public:
    TxThread();

    void txBegin();
    void txCommit();
    void txEnd();

    intptr_t txLoad(intptr_t* addr);
    void txStore(intptr_t* addr, intptr_t val);

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

// Macros for instrumenting loads and stores
#ifdef USE_STM
#define LOAD(var) (_my_thread.txLoad((intptr_t*)&var))
#define STORE(var, val) (_my_thread.txStore((intptr_t*)&var, (intptr_t)val))

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

#endif