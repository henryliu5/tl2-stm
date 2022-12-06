#include "include/stm.hpp"

#include <iostream>
#include <unistd.h>
#include <thread>
#include <malloc.h>
#include <set>
#include <algorithm>
#include <signal.h>

bool TxThread::inReadSet(uint64_t addr){
    return find(read_set.begin(), read_set.end(), (intptr_t*) addr) != read_set.end();
}


void raceHandler(int signum, siginfo_t * sig_info, void* context){
    uint64_t faulted_addr = (uint64_t) sig_info->si_addr;
    psignal(signum, "Possible UAF");
    if(_my_thread.inReadSet(faulted_addr)){
        _my_thread.txAbort();
    }
}

void registerSignalHandlers(){
    struct sigaction sig_handler;
    sig_handler.sa_sigaction = raceHandler;
    sigemptyset(&sig_handler.sa_mask);
    sig_handler.sa_flags = SA_SIGINFO;
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
    freed.clear();
    // Profiling/misc. info
    if (inTx)
        cout << "WARNING: txBegin() called but already in Tx" << endl;
    inTx = true;
    txCount++;
    
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

        bool lock_acquired = false;
        
        // useconds_t delay = 100;
        // useconds_t MAX_DELAY = 100000;
        // Bounded spinning with exponential backoff
        assert(!(lock->isLocked() && (lock->owner == this_thread::get_id())));
        for (int i = 0; i < 5; i++) {
            assert(!(lock->isLocked() && (lock->owner == this_thread::get_id())));
            if (lock->tryLock(rv)) {
                lock_acquired = true;
                locks_held.insert(lock);
                break;
            } 
        }
        if (!lock_acquired) {
            // cout << "lacquire failed " << lock << " owner: " << lock->owner << endl;
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
    // TODO add rv + 1 = wv optimization
    for(intptr_t* read_addr: read_set){
        VersionedLock* read_lock = &GET_LOCK(read_addr);

        if(locks_held.find(read_lock) != locks_held.end()){
            assert(read_lock->owner == this_thread::get_id());
        }

        // "For each location in the read-set... the versioned-write-lock is <= rv"
        // "We also verify memory locations have not been locked by other threads"
        if(read_lock->getVersion() > rv || (read_lock->isLocked() && read_lock->owner != this_thread::get_id())){
            txAbort();
            assert(0);
        }
    }

    // 6. Commit and release locks
    // Write back
    for (const auto& p : write_map) {
        *(p.first) = p.second;
    }

    // Check that we have the locks for everything we will eventually free
    #ifdef NDEBUG
    for(void* addr: speculative_free){
        size_t object_size = malloc_usable_size(addr);
        for(uint64_t temp = (uint64_t) addr; temp < (uint64_t) addr + object_size; temp ++){
            VersionedLock* lock = &GET_LOCK((intptr_t*) temp);
            if(locks_held.count(lock) != 1){
                cout << "missing lock: " << lock << endl;
                // assert(0);
            }
            assert(locks_held.count(lock) == 1);
            assert(lock->owner == this_thread::get_id());
            assert(lock->isLocked());
        }
    }
    #endif

    for(VersionedLock* write_lock: locks_held){
        assert(wv > write_lock->getVersion());
        assert(write_lock->isLocked());
        write_lock->unlock(wv);
        // After releasing the lock, we can't be the owner anymore
        assert(!write_lock->isLocked() || write_lock->owner != this_thread::get_id());
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
    freeSpeculativeMalloc();
    for(VersionedLock* write_lock: locks_held){
        assert(write_lock->isLocked());
        write_lock->abortUnlock();
        // After releasing the lock, we can't be the owner anymore
        assert(!write_lock->isLocked() || write_lock->owner != this_thread::get_id());
    }
    required_write_locks.clear();
    // global_lock.unlock();
    locks_held.clear();
    write_map.clear();
    wv = -1; // make it clear we can't use these until they are set later
    rv = -1;
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
    assert(addr != NULL);
    if (!inTx) {
        return *addr;
    }

    // 2. Pre-validation
    read_set.push_back(addr);
    VersionedLock* lock = &GET_LOCK(addr);
    int64_t prior_version = lock->getVersion();
    if (lock->isLocked() || prior_version > rv) {
        txAbort();
        assert(0);
    }

    if (write_map.find(addr) != write_map.end()) {
        // cout << "getting from write map: addr " << addr << " val: " << (int64_t) write_map[addr] << endl;
        return write_map[addr];
    }
    intptr_t return_value = *addr;

    int64_t new_version = lock->getVersion();
    // 2. Post-validation
    if (new_version != prior_version || lock->isLocked() || new_version > rv) {
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
    // cout << "marking free: " << addr << endl;
    // NOTE: Can't really do anything to this address, just trusting users don't have use-after-free
    // read_set.push_back((intptr_t*) addr);
    write_map[(intptr_t*) addr] = 0;
    speculative_free.push_back(addr);

    // 2. Pre-validation
    VersionedLock* lock = &GET_LOCK(addr);

    // 2. Post-validation
    if (lock->isLocked() || lock->getVersion() > rv) {
        txAbort();
    }

    size_t object_size = malloc_usable_size(addr);
    // We are effectively "writing" at all byte locations in the object by freeing it - add these to the 
    // write set
    for(uint64_t temp = (uint64_t) addr; temp < (uint64_t) addr + object_size; temp ++){
        VersionedLock* lock = &GET_LOCK((intptr_t*) temp);
        required_write_locks.push_back(lock);
    }
}

void TxThread::waitForQuiesce(void* base){
    assert(0);
    // size_t object_size = malloc_usable_size(base);
    // set<VersionedLock*> all_object_locks;
    // for(uint64_t temp = (uint64_t) base; temp < (uint64_t) base + (uint64_t) object_size; temp++){
    //     all_object_locks.insert(&GET_LOCK(temp));
    // }
    // for(VersionedLock* lock: all_object_locks){
    //     lock->waitLock();
    // }
    // free(base);

    // for(VersionedLock* lock: all_object_locks){
    //     lock->unlock(global_version_clock.load());
    // }
}

void TxThread::freeSpeculativeMalloc(){
    for(void* addr: speculative_malloc){
        assert(freed.count(addr) == 0);
        free(addr);
        freed.insert(addr);
    }
    speculative_malloc.clear();
    speculative_free.clear();
    required_write_locks.clear();
}

void TxThread::freeSpeculativeFree(){
    assert(0);
    // // Called by txCommit, actually perform frees, leave mallocs be
    // for(void* addr: speculative_free){
    //     assert(freed.count(addr) == 0);
    //     VersionedLock* write_lock = &GET_LOCK((intptr_t*) addr);
    //     assert(wv > write_lock->getVersion());
    //     assert(write_lock->isLocked() && write_lock->owner == this_thread::get_id());

    //     // // We want to make sure we invalidate all future reads
    //     // waitForQuiesce(addr);
    //     //!! In the paper they 'wait for objects to quiesce here'
    //     //!! to me it is unclear what this means, or at least the literal interpretation
    //     //!! has poor performance. The idea is to invalidate future readers and wait for writers to finish
    //     //!! To me this means that to wait for an object to 'quiesce' it must essentially
    //     //!! capture all locks relevant to the object
    //     //!! Instead my approach treats the entire object as part of the write set when calling TxFree

    //     size_t object_size = malloc_usable_size(addr);
    //     for(uint64_t temp = (uint64_t) addr; temp < (uint64_t) addr + (uint64_t) object_size; temp++){
    //         VersionedLock* write_lock = &GET_LOCK((intptr_t*) addr);
    //         assert(locks_held.count(write_lock) == 1);
    //         assert(wv > write_lock->getVersion());
    //         assert(write_lock->isLocked() && write_lock->owner == this_thread::get_id());
    //     }

    //     free(addr);
    //     freed.insert(addr);
    // }
    // speculative_malloc.clear();
    // speculative_free.clear();
}