#ifndef TL2_STM_IMPL_H
#define TL2_STM_IMPL_H
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <csetjmp>
#include <assert.h>
#include <iostream>
#include <thread>

using namespace std;

// Put globals here, e.g. global version clock, PS lock array
inline static atomic<int64_t> global_version_clock { 0 };
// Global lock for testing
inline mutex global_lock;

class VersionedLock {
    int64_t version;
    mutex lock;
    bool locked;
    int64_t versionAtLock;
    int64_t globalClockAtLock;
    

    void incrementVersion(){
        version = version + 1;
    }

public:
    thread::id owner;
    VersionedLock() : version{0}, lock{}, locked{false} {}

    int64_t getVersion(){
        return version;
    }

    bool tryLock(int rv){
        bool res = lock.try_lock();
        if(res){
            // Acquired lock
            locked = true;
            versionAtLock = getVersion();
            globalClockAtLock = rv;
            owner = this_thread::get_id();
        }
        return res;
    }

    void unlock(int64_t new_version){
        assert(owner == this_thread::get_id());
        if(new_version <= version){
            cout << "new version > version " << new_version << " " << version << endl;
            cout << "version at lock: " << versionAtLock << endl;
            cout << "rv at lock: " << globalClockAtLock << endl;
            assert(0);
        }
        version = new_version;
        // TODO add a check to make sure this thread owns the lock
        
        // Version is advanced every successful lock release
        // incrementVersion();
        locked = false;
        lock.unlock();
    }

    void abortUnlock(){
        // Used by abort to unlock this lock without changing the version
        locked = false;
        lock.unlock();
    }

    bool isLocked(){
        return locked;
    }
};

// Per stripe lock array - basically just a hash map
#define NUM_LOCKS (2 << 20)
inline VersionedLock PSLocks[NUM_LOCKS];
#define GET_LOCK(addr) (PSLocks[(((uint64_t) addr & 0x3FFFFC) + (uint64_t) addr) % NUM_LOCKS])

class TxThread {
    int64_t rv;
    int64_t wv;
    unordered_set<VersionedLock*> locks_held;
    vector<intptr_t*> read_set;
    unordered_map<intptr_t*, intptr_t> write_map;

    void txAbort();
    void txCommit();
public:
    TxThread();

    void txBegin();
    void txEnd();

    intptr_t txLoad(intptr_t* addr);
    void txStore(intptr_t* addr, intptr_t val);

    jmp_buf jump_buffer;
    bool inTx; // Currently no nesting
    // Profiling
    int txCount;
    int numLoads;
    int numStores;
};


// Using thread local storage for some magic here - every thread automatically
// gets this _my_thread transactional context
inline thread_local TxThread _my_thread;
inline thread_local int _thread_id;

// Macros for instrumenting loads and stores
#ifdef USE_STM
#define LOAD(var) (_my_thread.txLoad((intptr_t*)&var))
#define STORE(var, val) (_my_thread.txStore((intptr_t*)&var, (intptr_t)val))

#else
#define LOAD(var) (var)
#define STORE(var, val) (var = val)
#endif

#ifdef USE_STM
#define TxBegin() setjmp(_my_thread.jump_buffer); _my_thread.txBegin();
#define TxEnd() (_my_thread.txEnd())
#else
#define TxBegin() ({})
#define TxEnd() ({})
#endif

#endif