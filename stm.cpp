#include "include/stm.hpp"

#include <iostream>
#include <unistd.h>
#include <thread>

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
    , wv { 0 }
    , locks_held {}
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
    read_set.clear();
    if (locks_held.size() > 0)
        cout << "WARNING: txBegin() called but holding locks" << endl;

    // Step 1. Sample global version-clock
    rv = global_version_clock.load();
    // global_lock.lock();
}

// Called by txEnd at the end of a transaction
void TxThread::txCommit()
{
    // 3. Lock write-set
    for (const auto& p : write_map) {
        VersionedLock* lock = &GET_LOCK(p.first);
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
            // else {
            //     usleep(delay);
            //     if (delay < MAX_DELAY)
            //     {
            //         delay *= 2;
            //     }
            // }
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
            // cout << "version, rv, locked, " << read_lock->getVersion() << " " << rv << " " << read_lock->isLocked() << endl;
            // Failed validation
            // cout << "failed validation" << endl;
            txAbort();
            assert(0);
        }
    }

    // 6. Commit and release locks
    // Write back
    for (const auto& p : write_map) {
        // cout << "writing back addr: " << p.first << " val: " << p.second << endl;
        *(p.first) = p.second;
    }
    for(VersionedLock* write_lock: locks_held){
        assert(wv > write_lock->getVersion());
        assert(write_lock->isLocked());
        write_lock->unlock(wv);
        // After releasing the lock, we can't be the owner anymore
        assert(!write_lock->isLocked() || write_lock->owner != this_thread::get_id());
    }
    locks_held.clear();
    write_map.clear();
}

void TxThread::txAbort()
{
    inTx = false;
    for(VersionedLock* write_lock: locks_held){
        assert(write_lock->isLocked());
        write_lock->abortUnlock();
        // After releasing the lock, we can't be the owner anymore
        assert(!write_lock->isLocked() || write_lock->owner != this_thread::get_id());
    }
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
    numLoads++;
    if (!inTx) {
        return *addr;
    }

    // 2. Pre-validation
    read_set.push_back(addr);
    VersionedLock* lock = &GET_LOCK(addr);
    int64_t prior_version = lock->getVersion();

    if (write_map.find(addr) != write_map.end()) {
        // cout << "getting from write map: addr " << addr << " val: " << (int64_t) write_map[addr] << endl;
        return write_map[addr];
    }
    intptr_t return_value = *addr;

    // 2. Post-validation
    if (lock->getVersion() != prior_version || lock->isLocked() || lock->getVersion() > rv) {
        txAbort();
    }
    return return_value;
}

void TxThread::txStore(intptr_t* addr, intptr_t val)
{
    if (!inTx) {
        *(addr) = val;
        return;
    }
    numStores++;
    // cout << "setting addr: " << addr << " val: " << val << endl;
    // Speculative, just write to log
    write_map[addr] = val;
}