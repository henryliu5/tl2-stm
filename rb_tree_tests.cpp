#include "include/RBTree.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <stdlib.h>

using namespace std;
int failures = 0;

void checkOrderAndSize(unordered_set<int>& s, RBTree& rb){
    // See if the size is right
    if(s.size() != rb.size()){
        // Should be some duplicates
        cout << "failed set equality "<< s.size() << " " << rb.size() << endl;
        failures++;
    }
    vector<int> v(s.begin(), s.end());
    sort(v.begin(), v.end());
    // See if order is maintained
    if(rb.inorder() != v){
        cout << "Failed order" << endl;
        failures++;
    }
}

void largeRand(){
    // Do a bunch of random insertions
    cout << "Starting large rand" << endl;
    unordered_set<int> s;
    RBTree rb;
    int N = 100000;
    for(int i = 0; i < N; i++){
        int val = rand() % N;
        s.insert(val);
        rb.insert(val);
    }
    checkOrderAndSize(s, rb);

    // Now do a bunch of random deletions
    for(int i = 0; i < N / 10; i++){
        int val = rand() % N;
        bool deleteRes = rb.deleteKey(val);
        if(deleteRes != s.count(val) == 1){
            cout << "deleted " << val << " from tree but not found in set" << endl;
            failures++;
        }
        s.erase(val);
    }
    checkOrderAndSize(s, rb);

    // Check containment of whatever remains
    for(int i = 0; i < N; i++){
        if(s.count(i) == 1){
            if(!rb.contains(i)){
                cout << "set contained, but rb tree didn't: " << i << endl;
                failures++;
            }
        }
    }
}

void smallSimple(){
    RBTree rb;
    rb.insert(5);
    rb.insert(3);
    rb.insert(-1);
    rb.insert(6);
    if(rb.size() != 4){
        cout << "Failed size check" << endl;
        failures++;
    }
    if(rb.inorder() != vector<int>{-1, 3, 5, 6}){
        cout << "Out of order" << endl;
        failures++;
    }
    rb.deleteKey(5);
    if(rb.inorder() != vector<int>{-1, 3, 6}){
        cout << "Delete failed" << endl;
        failures++;
    }
}

int main(){
    // RB Tree tests
    smallSimple();
    largeRand();

    if(failures > 0){
        cout << "\nFailed " << failures << " tests!!!" << endl;
    } else {
        cout << "\nPassed all tests" << endl;
    }
}