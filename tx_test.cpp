#include "include/RBTree.hpp"
#include "include/stm.hpp"
#include "include/HashMap.hpp"
#include <unordered_map>

void hashMap()
{
    HashMap m(10000);
    TxBegin();
    m.put(1, 1000);
    int64_t res;
    m.get(1, res);
    cout << LOAD(res) << endl;

    m.put(2, 100);
    m.get(2, res);
    TxEnd();
    cout << res << endl;
    
    cout << endl;
    int64_t res2;
    cout << m.get(1, res2) << endl;
    cout << "after commit: " << res2 << endl;
    cout << m.get(2, res2) << endl;
    cout << "after commit: " << res2 << endl;
    cout << "num loads: " << _my_thread.numLoads << " num stores: " << _my_thread.numStores << endl;
}

void nodeOps()
{
    // Node testNode{1000};
    // Node* nodePtr = &testNode;
    // // A test to see if Tx will correctly interpose on loads and stores
    // TxBegin();
    // // int x;
    // // cout << &testNode << endl;
    // cout << LOAD(nodePtr->val) << endl;
    // // STORE(x, (LOAD(testNode)));
    // TxEnd();

    // cout << "num loads: " << _my_thread.numLoads << " num stores: " << _my_thread.numStores << endl;
}

int main()
{
    nodeOps();
    hashMap();
}