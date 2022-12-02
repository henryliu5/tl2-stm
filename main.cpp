// Red black tree implementation from https://www.programiz.com/dsa/deletion-from-a-red-black-tree

#include "include/RBTree.hpp"
using namespace std;

int main()
{
    RBTree bst;
    bst.insert(55);
    bst.insert(40);
    bst.insert(65);
    bst.insert(60);
    bst.insert(75);
    bst.insert(57);

    bst.printTree();
    cout << endl
         << "After deleting" << endl;
    bst.deleteKey(40);
    bst.printTree();
}