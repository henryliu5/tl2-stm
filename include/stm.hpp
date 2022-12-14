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

#include <ankerl/unordered_dense.h>
#include <boost/container/small_vector.hpp>

using namespace std;

// Put globals here, e.g. global version clock, PS lock array
inline static atomic<int64_t> global_version_clock { 0 };
// Global lock for testing
inline mutex global_lock;
inline bool debug{false};

class VersionedLock {
public:
    int64_t version;
    mutex lock;
    bool locked;

    VersionedLock() : version{0}, lock{}, locked{false} {}

    bool tryLock(){
        bool res = lock.try_lock();
        if(res){
            // Acquired lock
            locked = true; 
            //!!       I think it might be really important this "locked" is at the end here.
            //!!       There are assertions sprinkled around that check isLocked() and check 
            //!!       conditions about the above fields. If the fields are changed after locked,
            //!!       then an assertion can see the lock is held, but get incorrect information
            //!!       about the lock holder.
        }
        return res;
    }

    void unlock(int64_t new_version){
        version = new_version;
        // Version is advanced every successful lock release
        locked = false;
        lock.unlock();
    }

    void abortUnlock(){
        // Used by abort to unlock this lock without changing the version
        locked = false;
        lock.unlock();
    }
};

// Per stripe lock array - basically just a hash map
#define NUM_LOCKS (2 << 20)
inline VersionedLock PSLocks[NUM_LOCKS];
#define GET_LOCK(addr) (PSLocks[(((uint64_t) addr & 0x3FFFFC) + (uint64_t) addr) % NUM_LOCKS])

class TxThread {
    int64_t rv;
    int64_t wv;
    ankerl::unordered_dense::set<VersionedLock*> locks_held;
    boost::container::small_vector<intptr_t*, 256> read_set;
    ankerl::unordered_dense::map<intptr_t*, intptr_t> write_map;
    vector<void*> speculative_malloc;
    vector<void*> speculative_free;
    vector<VersionedLock*> required_write_locks;

    
    void txCommit();
    
public:
    TxThread();

    void txBegin();
    void txEnd();

    intptr_t txLoad(intptr_t* addr);
    void txStore(intptr_t* addr, intptr_t val);

    void* txMalloc(size_t);
    void txFree(void* p);

    bool inReadSet(uint64_t);
    void txAbort();

    jmp_buf jump_buffer;
    bool inTx; // Currently no nesting
    // Profiling
    int txCount;
    int numLoads;
    int numStores;
    bool read_only;
    useconds_t delay;
};


// Using thread local storage for some magic here - every thread automatically
// gets this _my_thread transactional context
inline thread_local TxThread _my_thread;

// Macros for instrumenting loads and stores
#ifdef USE_STM
#define LOAD(var) (_my_thread.txLoad((intptr_t*)&var))
#define STORE(var, val) (_my_thread.txStore((intptr_t*)&var, (intptr_t)val))
#define MALLOC(size) (_my_thread.txMalloc(size))
#define FREE(ptr) (_my_thread.txFree(ptr))
// #define FREE(ptr) ({})
#else
#define LOAD(var) (var)
#define STORE(var, val) (var = val)
#define MALLOC(size) (malloc(size))
#define FREE(ptr) (free(ptr))
#endif

#ifdef OPTIMISTIC_READ_ONLY // Optimistically assume that everything is read only until we get to a store
    #define TxBegin() _my_thread.read_only = true; setjmp(_my_thread.jump_buffer); _my_thread.txBegin();
#else
    #define TxBegin() _my_thread.read_only = false; setjmp(_my_thread.jump_buffer); _my_thread.txBegin();
#endif
#define TxEnd() (_my_thread.txEnd())
    #ifdef NO_RO_TX // Disable read only transactions entirely
    #define TxBeginReadOnly() TxBegin()
    #else
    #define TxBeginReadOnly() _my_thread.read_only = true; setjmp(_my_thread.jump_buffer); _my_thread.txBegin();
    #endif

#endif