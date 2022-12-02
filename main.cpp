#include "include/RBTree.hpp"
using namespace std;

int main()
{
    RBTree rb;
    rb.insert(100);
    rb.insert(101);
    rb.insert(100);

    // rb.printTree();
    rb.deleteKey(100);
    cout << endl
         << "After deleting" << endl;
    // rb.printTree();
}