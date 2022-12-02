#ifndef RED_BLACK_TREE_HPP
#define RED_BLACK_TREE_HPP

struct Node {
    int data;
    Node* parent;
    Node* left;
    Node* right;
    int color;
};


#endif