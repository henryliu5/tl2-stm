#include "include/RedBlackTree.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <stdlib.h>

using namespace std;
int failures = 0;

void largeRand(){
    cout << "Starting large rand" << endl;
    unordered_set<int> s;
    RedBlackTree rb;
    int N = 100000;
    for(int i = 0; i < N; i++){
        int val = rand() % N;
        s.insert(val);
        rb.insert(val);
    }
    if(s.size() != rb.size()){
        // Should be some duplicates
        cout << "failed set equality "<< s.size() << " " << rb.size() << endl;
        failures++;
    }
    vector<int> v(s.begin(), s.end());
    sort(v.begin(), v.end());
    if(rb.inorder() != v){
        cout << "Failed order" << endl;
        failures++;
    }

}

void smallSimple(){
    RedBlackTree rb;
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
    rb.deleteVal(5);
    if(rb.inorder() != vector<int>{-1, 3, 6}){
        cout << "Delete failed" << endl;
        failures++;
    }
}

int main(){
    smallSimple();
    largeRand();

    if(failures > 0){
        cout << "\nFailed " << failures << " tests!!!" << endl;
    } else {
        cout << "\nPassed all tests" << endl;
    }
}