#include "include/RBTree.hpp"
#include "include/stm.hpp"
#include "include/HashMap.hpp"
#include <unordered_map>

void hashMap()
{
    HashMap m(10000);
    m.put(1, 1000);
    int64_t res;
    m.get(1, res);
    cout << res << endl;
    m.remove(1);
    m.remove(0);
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