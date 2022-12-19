#include "include/stm.hpp"

#include <iostream>
#include <unistd.h>
#include <thread>
#include <malloc.h>
#include <set>
#include <algorithm>
#include <signal.h>
#include <string.h>

bool TxThread::inReadSet(uint64_t addr){
    return find(read_set.begin(), read_set.end(), (intptr_t*) addr) != read_set.end();
}


void sigHandler(int signum, siginfo_t * sig_info, void* context){
    uint64_t faulted_addr = (uint64_t) sig_info->si_addr;
    if(write(1, "Possible UAF\n", strlen("Possible UAF\n")) < 0){
        cout << "failed to write" << endl;
        exit(1);
    }
    if(_my_thread.inReadSet(faulted_addr)){
        _my_thread.txAbort();
    }
}

void registerSignalHandlers(){
    struct sigaction sig_handler;
    sig_handler.sa_sigaction = sigHandler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigaction(SIGSEGV, &sig_handler, NULL);
}

TxThread::TxThread()
    : rv { 0 }
    , wv { 0 }
    , locks_held {}
    , inTx(false)
    , txCount(0)
    , numLoads(0)
    , numStores(0)

{
    registerSignalHandlers();
}

// Start new transaction
void TxThread::txBegin()
{
    // cout << "begin" << endl;
    // Profiling/misc. info
    if (inTx)
        cout << "WARNING: txBegin() called but already in Tx" << endl;
    inTx = true;
    txCount++;
    delay = 0;
    // Reset from previous Tx
    write_map.clear();
    read_set.clear();

    assert(speculative_malloc.size() == 0);
    assert(speculative_free.size() == 0);
    assert(locks_held.size() == 0);
    assert(required_write_locks.size() == 0);

    // Step 1. Sample global version-clock
    rv = global_version_clock.load();
    // global_lock.lock();
}

// Called by txEnd at the end of a transaction
void TxThread::txCommit()
{
    assert(inTx);
    // 3. Lock write-set
    for(VersionedLock* lock: required_write_locks){

        if(locks_held.find(lock) != locks_held.end()){
            // We already own this lock
            continue;
        }

        assert(locks_held.find(lock) == locks_held.end());

        // Acquire lock, just 1 try
        if (lock->tryLock()) {
            locks_held.insert(lock);
        } else {
            txAbort();
            assert(0);
        }
    }

    // assert(locks_held.size() == write_map.size()); // NOTE not true since hash collisions for address -> lock

    // 4. Increment global version-clock
    int64_t old_clock = global_version_clock.fetch_add(1);
    wv = old_clock + 1; // fetch add returns old value
    assert(wv > rv);

    // 5. Validate read set
    if(rv + 1 != wv){
        for(intptr_t* read_addr: read_set){
            VersionedLock* read_lock = &GET_LOCK(read_addr);
            // "For each location in the read-set... the versioned-write-lock is <= rv"
            // "We also verify memory locations have not been locked by other threads"
            if(read_lock->version > rv || (read_lock->locked && locks_held.find(read_lock) == locks_held.end())){
                txAbort();
                assert(0);
            }
        }
    }


    // 6. Commit and release locks
    // Write back
    for (const auto& p : write_map) {
        *(p.first) = p.second;
    }

    // Check that we have the locks for everything we will eventually free
    #ifndef NDEBUG
    for(void* addr: speculative_free){
        size_t object_size = malloc_usable_size(addr);
        for(uint64_t temp = (uint64_t) addr; temp < (uint64_t) addr + object_size; temp+= 8){
            VersionedLock* lock = &GET_LOCK((intptr_t*) temp);
            if(locks_held.count(lock) != 1){
                cout << "missing lock: " << lock << endl;
                // assert(0);
            }
            assert(locks_held.count(lock) == 1);
            assert(lock->locked);
        }
    }
    #endif

    for(VersionedLock* write_lock: locks_held){
        assert(wv > write_lock->version);
        assert(write_lock->locked);
        write_lock->unlock(wv);
        assert(!write_lock->locked);
    }

    // Actually perform frees now
    for(void* addr: speculative_free){
        free(addr);
    }

    // Tx complete successfully, clean up
    speculative_malloc.clear();
    speculative_free.clear();
    required_write_locks.clear();

    locks_held.clear();
    write_map.clear();
}

void TxThread::txAbort()
{
    inTx = false;

    for(void* addr: speculative_malloc){
        free(addr);
    }
    speculative_malloc.clear();
    speculative_free.clear();

    for(VersionedLock* write_lock: locks_held){
        assert(write_lock->locked);
        write_lock->abortUnlock();
        assert(!write_lock->locked);
    }

    required_write_locks.clear();
    // global_lock.unlock();
    locks_held.clear();
    write_map.clear();
    #ifndef NDEBUG
    wv = -1; // make it clear we can't use these until they are set later
    rv = -1;
    #endif

    #ifdef USE_BACKOFF
    usleep(delay);
    if(delay < 10000) // Maximum backoff
        delay *= 2;
    #endif

    longjmp(jump_buffer, txCount);
    assert(0);
}

// Cleanup after Tx
void TxThread::txEnd()
{
    if (!inTx)
        cout << "WARNING: txEnd() called but not in Tx" << endl;
    // cout << "starting txCommit: " << txCount << endl;
    txCommit();
    inTx = false;
    write_map.clear();
    read_set.clear();
    // cout << "tx completed: " << txCount << endl;
    // global_lock.unlock();
}

intptr_t TxThread::txLoad(intptr_t* addr)
{
    if (!inTx) {
        return *addr;
    }

    if(read_only){
        intptr_t return_value = *addr;
        VersionedLock* lock = &GET_LOCK(addr);
        // 2. Post-validation
        if (lock->locked || lock->version > rv) {
            txAbort();
        }
        return return_value;
    }

    // 2. Pre-validation
    VersionedLock* lock = &GET_LOCK(addr);
    int64_t prior_version = lock->version;
    if (prior_version > rv || lock->locked) {
        txAbort();
        assert(0);
    }

    auto iter = write_map.find(addr);
    if (iter != write_map.end()) {
        return iter->second;
    }
    read_set.push_back(addr);
    intptr_t return_value = *addr;

    int64_t new_version = lock->version;
    // 2. Post-validation
    if (new_version != prior_version || new_version > rv || lock->locked) {
        txAbort();
    }
    return return_value;
}

void TxThread::txStore(intptr_t* addr, intptr_t val)
{
    assert(addr != NULL);
    if (!inTx) {
        *(addr) = val;
        return;
    }

    #ifdef OPTIMISTIC_READ_ONLY
    if(read_only){
        read_only = false;
        txAbort();
    }
    #endif

    // cout << "setting addr: " << addr << " val: " << val << endl;
    // Speculative, just write to log
    write_map[addr] = val;
    required_write_locks.push_back(&GET_LOCK(addr));
}

void* TxThread::txMalloc(size_t size)
{
    assert(size != 0);
    if (!inTx) {
        return malloc(size);
    }

    #ifdef OPTIMISTIC_READ_ONLY
    if(read_only){
        read_only = false;
        txAbort();
    }
    #endif

    void* ptr = malloc(size);
    // read_set.push_back((intptr_t*) ptr);
    speculative_malloc.push_back(ptr);
    return ptr;
}

void TxThread::txFree(void* addr)
{
    assert(addr != 0);
    if (!inTx) {
        return free(addr);
    }

    #ifdef OPTIMISTIC_READ_ONLY
    if(read_only){
        read_only = false;
        txAbort();
    }
    #endif

    // cout << "marking free: " << addr << endl;
    // NOTE: Can't really do anything to this address, just trusting users don't have use-after-free
    // read_set.push_back((intptr_t*) addr);
    write_map[(intptr_t*) addr] = 0;
    speculative_free.push_back(addr);

    // 2. Pre-validation
    VersionedLock* lock = &GET_LOCK(addr);

    // 2. Post-validation
    if (lock->locked || lock->version > rv) {
        txAbort();
    }

    size_t object_size = malloc_usable_size(addr);
    // We are effectively "writing" at all byte locations in the object by freeing it - add these to the 
    // write set
    for(uint64_t temp = (uint64_t) addr; temp < (uint64_t) addr + object_size; temp += 8){
        VersionedLock* lock = &GET_LOCK((intptr_t*) temp);
        required_write_locks.push_back(lock);
    }
}
