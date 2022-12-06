#ifndef RB_TREE_HPP
#define RB_TREE_HPP
#include <iostream>
#include <vector>
#include "stm.hpp"

// Red black tree implementation based on https://www.geeksforgeeks.org/deletion-in-red-black-tree/
using namespace std;

#define LOAD_NODE(addr) ((Node*) LOAD(addr))
#define COLOR int64_t
#define RED 0
#define BLACK 1
// enum COLOR { RED,
//     BLACK };

class Node {
public:
    int64_t val;
    COLOR color;
    Node *left, *right, *parent;

    Node(int val)
        : val(val)
        , color(RED) // Node is red at insertion
        , left(NULL)
        , right(NULL)
        , parent(NULL)
    {}

    // returns pointer to uncle
    Node* uncle()
    {
        // If no parent or grandparent, then no uncle
        if (LOAD_NODE(parent) == NULL || LOAD_NODE(LOAD_NODE(parent)->parent) == NULL)
            return NULL;

        Node* grandparent = LOAD_NODE(LOAD_NODE(parent)->parent);
        if (LOAD_NODE(parent)->isOnLeft())
            // uncle on right
            return LOAD_NODE(grandparent->right);
        else
            // uncle on left
            return LOAD_NODE(grandparent->left);
    }

    // check if node is left child of parent
    bool isOnLeft() { return this == LOAD_NODE(LOAD_NODE(parent)->left); }

    // returns pointer to sibling
    Node* sibling()
    {
        // sibling null if no parent
        if (LOAD_NODE(parent) == NULL)
            return NULL;

        if (isOnLeft())
            return LOAD_NODE(LOAD_NODE(parent)->right);

        return LOAD_NODE(LOAD_NODE(parent)->left);
    }

    // moves node down and moves given node in its place
    void moveDown(Node* nParent)
    {
        if (LOAD_NODE(parent) != NULL) {
            if (isOnLeft()) {
                STORE(LOAD_NODE(parent)->left, nParent);
            } else {
                STORE(LOAD_NODE(parent)->right, nParent);
            }
        }
        STORE(nParent->parent, LOAD_NODE(parent));
        STORE(parent, nParent);
    }

    bool hasRedChild()
    {
        return (LOAD_NODE(left) != NULL && LOAD(LOAD_NODE(left)->color) == RED) || (LOAD_NODE(right) != NULL && LOAD(LOAD_NODE(right)->color) == RED);
    }
};

class RBTree {
    Node* root;

    // left rotates the given node
    void leftRotate(Node* x)
    {
        // new parent will be node's right child
        Node* nParent = LOAD_NODE(x->right);

        // update root if current node is root
        if (x == LOAD_NODE(root))
            STORE(root, nParent);

        x->moveDown(nParent);

        // connect x with new parent's left element
        STORE(x->right, LOAD_NODE(nParent->left));
        // connect new parent's left element with node
        // if it is not null
        if (LOAD_NODE(nParent->left) != NULL){
            Node* nParentLeft = LOAD_NODE(nParent->left);
            STORE(nParentLeft->parent, x);
        }
        // connect new parent with x
        STORE(nParent->left, x);
    }

    void rightRotate(Node* x)
    {
        // new parent will be node's left child
        Node* nParent = LOAD_NODE(x->left);

        // update root if current node is root
        if (x == LOAD_NODE(root))
            STORE(root, nParent);

        x->moveDown(nParent);

        // connect x with new parent's right element
        STORE(x->left, LOAD_NODE(nParent->right));
        // connect new parent's right element with node
        // if it is not null
        if (LOAD_NODE(nParent->right) != NULL){
            Node* nParentRight = LOAD_NODE(nParent->right);
            STORE(nParentRight->parent, x);
        }
        // connect new parent with x
        STORE(nParent->right, x);
    }

    void swapColors(Node* x1, Node* x2)
    {
        COLOR temp = (COLOR) LOAD(x1->color);
        STORE(x1->color, LOAD(x2->color));
        STORE(x2->color, temp);
    }

    void swapValues(Node* u, Node* v)
    {
        int64_t temp = LOAD(u->val);
        STORE(u->val, LOAD(v->val));
        STORE(v->val, temp);
    }

    // fix red red at given node
    void fixRedRed(Node* x)
    {
        // if x is root color it black and return
        if (x == LOAD_NODE(root)) {
            STORE(x->color, BLACK);
            return;
        }

        // initialize parent, grandparent, uncle
        Node* parent = LOAD_NODE(x->parent);
        Node* grandparent = LOAD_NODE(parent->parent);
        Node* uncle = x->uncle();

        if (LOAD(parent->color) != BLACK) {
            if (uncle != NULL && LOAD(uncle->color) == RED) {
                // uncle red, perform recoloring and recurse
                STORE(parent->color, BLACK);
                STORE(uncle->color, BLACK);
                STORE(grandparent->color, RED);
                fixRedRed(grandparent);
            } else {
                // Else perform LR, LL, RL, RR
                if (parent->isOnLeft()) {
                    if (x->isOnLeft()) {
                        // for left right
                        swapColors(parent, grandparent);
                    } else {
                        leftRotate(parent);
                        swapColors(x, grandparent);
                    }
                    // for left left and left right
                    rightRotate(grandparent);
                } else {
                    if (x->isOnLeft()) {
                        // for right left
                        rightRotate(parent);
                        swapColors(x, grandparent);
                    } else {
                        swapColors(parent, grandparent);
                    }

                    // for right right and right left
                    leftRotate(grandparent);
                }
            }
        }
    }

    // find node that do not have a left child
    // in the subtree of the given node
    Node* successor(Node* x)
    {
        Node* temp = x;

        while (LOAD_NODE(temp->left) != NULL)
            temp = LOAD_NODE(temp->left);

        return temp;
    }

    // find node that replaces a deleted node in BST
    Node* BSTreplace(Node* x)
    {
        // when node have 2 children
        if (LOAD_NODE(x->left) != NULL && LOAD_NODE(x->right) != NULL)
            return successor(LOAD_NODE(x->right));

        // when leaf
        if (LOAD_NODE(x->left) == NULL && LOAD_NODE(x->right) == NULL)
            return NULL;

        // when single child
        if (LOAD_NODE(x->left) != NULL)
            return LOAD_NODE(x->left);
        else
            return LOAD_NODE(x->right);
    }

    // deletes the given node
    void deleteNode(Node* v)
    {
        Node* u = BSTreplace(v);

        // True when u and v are both black
        bool uvBlack = ((u == NULL || LOAD(u->color) == BLACK) && (LOAD(v->color) == BLACK));
        Node* parent = LOAD_NODE(v->parent);

        if (u == NULL) {
            // u is NULL therefore v is leaf
            if (v == LOAD_NODE(root)) {
                // v is root, making root null
                STORE(root, NULL);
            } else {
                if (uvBlack) {
                    // u and v both black
                    // v is leaf, fix double black at v
                    fixDoubleBlack(v);
                } else {
                    // u or v is red
                    if (v->sibling() != NULL)
                        // sibling is not null, make it red"
                        STORE(v->sibling()->color, RED);
                }

                // delete v from the tree
                if (v->isOnLeft()) {
                    STORE(parent->left, NULL);
                } else {
                    STORE(parent->right, NULL);
                }
            }
            // delete v; // TODO handle memory management
            // v->~Node();
            STORE(v->color, RED);
            STORE(v->val, 0);
            STORE(v->right, NULL);
            STORE(v->left, NULL);
            STORE(v->parent, NULL);
            STORE(*v, 0);
            FREE(v);
            return;
        }

        if (LOAD_NODE(v->left) == NULL || LOAD_NODE(v->right) == NULL) {
            // v has 1 child
            if (v == LOAD_NODE(root)) {
                // v is root, assign the value of u to v, and delete u
                STORE(v->val, u->val);
                STORE(v->left, NULL);
                STORE(v->right, NULL);
                // delete u; // TODO handle memory management
                STORE(u->color, RED);
                STORE(u->val, 0);
                STORE(u->right, NULL);
                STORE(u->left, NULL);
                STORE(u->parent, NULL);
                STORE(*u, 0);
                FREE(u);
            } else {
                // Detach v from tree and move u up
                if (v->isOnLeft()) {
                    STORE(parent->left, u);
                } else {
                    STORE(parent->right, u);
                }
                // delete v; // TODO handle memory management
                STORE(v->color, RED);
                STORE(v->val, 0);
                STORE(v->right, NULL);
                STORE(v->left, NULL);
                STORE(v->parent, NULL);
                STORE(*v, 0);
                FREE(v);
                
                STORE(u->parent, parent);
                if (uvBlack) {
                    // u and v both black, fix double black at u
                    fixDoubleBlack(u);
                } else {
                    // u or v red, color u black
                    STORE(u->color, BLACK);
                }
            }
            return;
        }

        // v has 2 children, swap values with successor and recurse
        swapValues(u, v);
        deleteNode(u);
    }

    void fixDoubleBlack(Node* x)
    {
        if (x == LOAD_NODE(root))
            // Reached root
            return;

        Node* sibling = x->sibling();
        Node* parent = LOAD_NODE(x->parent);
        if (sibling == NULL) {
            // No sibiling, double black pushed up
            fixDoubleBlack(parent);
        } else {
            if (LOAD(sibling->color) == RED) {
                // Sibling red
                STORE(parent->color, RED);
                STORE(sibling->color, BLACK);
                if (sibling->isOnLeft()) {
                    // left case
                    rightRotate(parent);
                } else {
                    // right case
                    leftRotate(parent);
                }
                fixDoubleBlack(x);
            } else {
                // Sibling black
                if (sibling->hasRedChild()) {
                    // at least 1 red children
                    if (LOAD_NODE(sibling->left) != NULL && LOAD(LOAD_NODE(sibling->left)->color) == RED) {
                        if (sibling->isOnLeft()) {
                            // left left
                            STORE(LOAD_NODE(sibling->left)->color, LOAD(sibling->color));
                            STORE(sibling->color, LOAD(parent->color));
                            rightRotate(parent);
                        } else {
                            // right left
                            STORE(LOAD_NODE(sibling->left)->color, LOAD(parent->color));
                            rightRotate(sibling);
                            leftRotate(parent);
                        }
                    } else {
                        if (sibling->isOnLeft()) {
                            // left right
                            STORE(LOAD_NODE(sibling->right)->color, LOAD(parent->color));
                            leftRotate(sibling);
                            rightRotate(parent);
                        } else {
                            // right right
                            STORE(LOAD_NODE(sibling->right)->color, LOAD(sibling->color));
                            STORE(sibling->color, LOAD(parent->color));
                            leftRotate(parent);
                        }
                    }
                    STORE(parent->color, BLACK);
                } else {
                    // 2 black children
                    STORE(sibling->color, RED);
                    if (LOAD(parent->color) == BLACK)
                        fixDoubleBlack(parent);
                    else
                        STORE(parent->color, BLACK);
                }
            }
        }
    }

    // prints inorder recursively
    void inorderHelp(Node* x, vector<int>& v)
    {
        if (x == NULL)
            return;
        inorderHelp(x->left, v);
        v.push_back(x->val);
        inorderHelp(x->right, v);
    }

    size_t sizeHelp(Node* n)
    {
        if (!n)
            return 0;
        return 1 + sizeHelp(n->left) + sizeHelp(n->right);
    }

    bool getHelp(Node* n, int64_t key)
    {
        if (!n)
            return false;
        if (LOAD(n->val) == key) {
            return true;
        }
        if (LOAD(n->val) < key) {
            return getHelp(LOAD_NODE(n->right), key);
        }
        return getHelp(LOAD_NODE(n->left), key);
    }

    int maxHeightHelp(Node *n){
        if(!n) return 0;
        return max(maxHeightHelp(n->left), maxHeightHelp(n->right)) + 1;
    }

public:
    // constructor
    // initialize root
    RBTree() { root = NULL; }

    Node* getRoot() { return root; }

    // searches for given value
    // if found returns the node (used for delete)
    // else returns the last node while traversing (used in insert)
    Node* search(int64_t n)
    {
        Node* temp = LOAD_NODE(root);
        while (temp != NULL) {
            if (n < LOAD(temp->val)) {
                if (LOAD_NODE(temp->left) == NULL)
                    break;
                else
                    temp = LOAD_NODE(temp->left);
            } else if (n == LOAD(temp->val)) {
                break;
            } else {
                if (LOAD_NODE(temp->right) == NULL)
                    break;
                else
                    temp = LOAD_NODE(temp->right);
            }
        }

        return temp;
    }

    // inserts the given value to tree
    void insert(int64_t n)
    {
        // Node* newNode = new Node(n);
        void* newNodeMem = MALLOC(sizeof(Node));
        // The "placement new"
        Node* newNode = new(newNodeMem) Node(n);

        if (LOAD_NODE(root) == NULL) {
            // when root is null
            // simply insert value at root
            newNode->color = BLACK;
            STORE(root, newNode);
        } else {
            Node* temp = search(n);

            if (LOAD(temp->val) == n) {
                // return if value already exists
                return;
            }

            // if value is not found, search returns the node
            // where the value is to be inserted

            // connect new node to correct node
            newNode->parent = temp;

            if (n < LOAD(temp->val))
                STORE(temp->left, newNode);
            else
                STORE(temp->right, newNode);

            // fix red red voilaton if exists
            fixRedRed(newNode);
        }
    }

    // utility function that deletes the node with given value
    bool deleteKey(int64_t n)
    {
        if (LOAD_NODE(root) == NULL)
            // Tree is empty
            return false;

        Node* v = search(n);

        if (LOAD(v->val) != n) {
            return false;
        }

        deleteNode(v);
        return true;
    }

    vector<int> inorder()
    {
        vector<int> v;
        inorderHelp(root, v);
        return v;
    }

    size_t size()
    {
        return sizeHelp(root);
    }

    bool get(int key)
    {
        return getHelp(LOAD_NODE(root), key);
    }

    int maxHeight(){
        return maxHeightHelp(root);
    }
};

#endif