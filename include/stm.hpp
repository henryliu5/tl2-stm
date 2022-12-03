#ifndef TL2_STM_IMPL_H
#define TL2_STM_IMPL_H

// Put globals here, e.g. global version clock, PS lock array

struct TxThread {
    // Profiling
    int txCount;
    int numLoads;
    int numStores;
};

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
    _my_thread.txCount++;
}

void TxEnd()
{
}

intptr_t TxLoad(intptr_t* addr)
{
    // int x = 1
    // TxLoadInt(x)
    _my_thread.numLoads++;
    return *addr;
}

void TxStore(intptr_t* addr, intptr_t val)
{
    // int x = 1
    // TxStoreInt(y, x)
    // ... y = 1
    _my_thread.numStores++;
    *addr = val;
}

#endif