#ifndef HASH_TABLE_HPP
#define HASH_TABLE_HPP
#include <iostream>
#include "stm.hpp"
// Hash table implementation adapted from https://aozturk.medium.com/simple-hash-map-hash-table-implementation-in-c-931965904250

#define LOAD_HN(addr) ((HashNode*) LOAD(addr))

class HashNode {
public:
    HashNode(const int64_t &key, const int64_t &value) :
    key(key), value(value), next(NULL) {
    }

    int64_t getKey() const {
        return LOAD(key);
    }

    int64_t getValue() const {
        return LOAD(value);
    }

    void setValue(int64_t _value) {
        STORE(value, _value);
    }

    HashNode* getNext() const {
        return (HashNode*) LOAD(next);
    }

    void setNext(HashNode *_next) {
        STORE(next, _next);
    }

private:
    // key-value pair
    int64_t key;
    int64_t value;
    // next bucket with the same key
    HashNode *next;
};

class HashMap {
public:
    HashMap(int table_size) : table_size(table_size) {
        // construct zero initialized hash table of size
        table = new HashNode*[table_size]();
    }

    // ~HashMap() {
    //     // destroy all buckets one by one
    //     for (int i = 0; i < table_size; ++i) {
    //         HashNode* entry = table[i];
    //         while (entry != NULL) {
    //             HashNode* prev = entry;
    //             entry = entry->getNext();
    //             delete prev;
    //         }
    //         table[i] = NULL;
    //     }
    //     // destroy the hash table
    //     delete [] table;
    // }

    bool get(const int64_t &key, int64_t& value) {
        unsigned long hashValue = key % table_size;
        HashNode* entry = (HashNode*) LOAD(table[hashValue]);

        while (entry != NULL) {
            if (entry->getKey() == key) {
                STORE(value, entry->getValue());
                return true;
            }
            STORE(entry, entry->getNext());
        }
        return false;
    }

    void put(const int64_t key, const int64_t value) {
        // unsigned long hashValue = key % table_size;
        // HashNode* prev;
        // STORE(prev, NULL);
        // HashNode* entry;
        // STORE(entry, LOAD_HN(table[hashValue]));

        // while (LOAD_HN(entry) != NULL && LOAD_HN(entry)->getKey() != key) {
        //     // cout << "here" << endl;
        //     // cout << "entry val: " << entry->getKey() << " " << entry->getValue() << endl;
        //     assert(entry != entry->getNext());
        //     STORE(prev, LOAD_HN(entry));
        //     STORE(entry, LOAD_HN(entry)->getNext());
        // }

        // if (LOAD_HN(entry) == NULL) {
        //     STORE(entry, new HashNode(key, value));
        //     if (LOAD_HN(prev) == NULL) {
        //         // insert as first bucket
        //         STORE(table[hashValue], LOAD_HN(entry));
        //     } else {
        //         LOAD_HN(prev)->setNext(LOAD_HN(entry));
        //     }
        // } else {
        //     // just update the value
        //     LOAD_HN(entry)->setValue(value);
        // }
        unsigned long hashValue = key % table_size;
        HashNode* prev = NULL;
        HashNode* entry = (HashNode*) LOAD(table[hashValue]);

        while (entry != NULL && entry->getKey() != key) {
            prev = entry;
            entry = entry->getNext();
        }

        if (entry == NULL) {
            void* entryMem = MALLOC(sizeof(HashNode));
            entry = new(entryMem) HashNode(key, value);
            if (prev == NULL) {
                // insert as first bucket
                STORE(table[hashValue], entry);
            } else {
                prev->setNext(entry);
            }
        } else {
            // just update the value
            entry->setValue(value);
        }
    }

    void remove(const int64_t &key) {
        unsigned long hashValue = key % table_size;
        HashNode* prev = NULL;
        HashNode* entry = (HashNode*) LOAD(table[hashValue]);

        while (entry != NULL && entry->getKey() != key) {
            prev = entry;
            entry = entry->getNext();
        }

        if (entry == NULL) {
            // key not found
            return;
        }
        else {
            if (prev == NULL) {
                // remove first bucket of the list
                STORE(table[hashValue], entry->getNext());
            } else {
                prev->setNext(entry->getNext());
            }
            // delete entry; // TODO tx memory management
            FREE(entry);
        }
    }

private:
    // hash table
    HashNode**table;
    int table_size;
};

#endif