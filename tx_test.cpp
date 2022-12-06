#include "include/RBTree.hpp"
#include "include/HashMap.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <stdlib.h>
#include <chrono>
#include <random>
#include <cstdlib>

using namespace std;
using std::chrono::high_resolution_clock;
using std::chrono::duration_cast;
using std::chrono::duration;
using std::chrono::milliseconds;

int main()
{
    cout << "Starting benchmark" << endl;
    vector<thread> workers;
    // Spawn threads
    void* node1_mem = malloc(sizeof(HashNode));
    HashNode* node1 = new(node1_mem) HashNode(1, -1);

    void* node2_mem = malloc(sizeof(HashNode));
    HashNode* node2 = new(node2_mem) HashNode(2, -2);

    void* node3_mem = malloc(sizeof(HashNode));
    HashNode* node3 = new(node3_mem) HashNode(3, -3);

    void* node4_mem = malloc(sizeof(HashNode));
    HashNode* node4 = new(node4_mem) HashNode(4, -4);

    node1->setNext(node2);
    node2->setNext(node3);

    workers.push_back(thread([node1]() {
        TxBegin();
        // Remove the second node in the list
        HashNode* tempNext = node1->getNext();
        HashNode* tempNextNext = tempNext->getNext();
        node1->setNext(tempNextNext);
        FREE(tempNext);
        TxEnd();
    }));

    workers.push_back(thread([node1, node4]() {
        TxBegin();
        // Append something after the second node in the list
        HashNode* tempNext = node1->getNext();
        HashNode* tempNextNext = tempNext->getNext();
        tempNext->setNext(node4);
        node4->setNext(tempNextNext);
        TxEnd();
    }));
    // Barrier
    for_each(workers.begin(), workers.end(), [](thread& t) {
        t.join();
    });
    
    HashNode* temp = node1;
    int size = 0;
    while(temp != NULL){
        size++;
        temp = temp->getNext();
    }
    assert(size == 3);
}