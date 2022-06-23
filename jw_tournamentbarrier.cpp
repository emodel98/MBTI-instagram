#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <queue>
#include "barrier.hpp"

using namespace std;

class TornamentBarrier : public Barrier {
private:
    TornamentBarrier *parent, *partner;
    bool isRoot;

public:
    TornamentBarrier(): parent(NULL), partner(NULL), isRoot(false) {}
    TornamentBarrier(TornamentBarrier *_parent): parent(_parent), partner(new TornamentBarrier()), isRoot(false) {
        partner->setPartner(this);
    }

    void setPartner(TornamentBarrier* _partner) { this->partner = _partner; } 
    void setRootMark() { this->isRoot = true; }

    void await(bool);

    bool Top() { return isRoot; };
    TornamentBarrier *getParent() { return this->parent; }
    TornamentBarrier *getPartner() { return this->partner; }
};

void TornamentBarrier::await(bool mySense) {
    if(this->isRoot) return;
    else if(this->parent != NULL){
        while(this->Flag() != mySense) {};
        this->parent->await(mySense);
        this->partner->setSense(mySense);
    } else {
        this->partner->setSense(mySense);
        while(this->Flag() != mySense);
    }
}


TornamentBarrier* root;
vector<TornamentBarrier*> nodes;

void useTB(int myNum) {
    nodes[myNum]->await(false);
}

void buildTree(TornamentBarrier* rt, int depth) {
    queue<TornamentBarrier*> temp_nodes;
    temp_nodes.push(rt);
   

    // Build tree structure
    TornamentBarrier *cur, *new_node;
    for(int i=0; i<(depth-1); i++) {
        for(int j=0; j<(1<<i); j++) {
            cur = temp_nodes.front();
            new_node = new TornamentBarrier(cur);
            temp_nodes.push(new_node);
            temp_nodes.push(new_node->getPartner());
            temp_nodes.pop();
        }
    }
    
    // put leaf nodes in vector
    for(int i=0; i<1<<(depth-1); i++) {
        cur = temp_nodes.front();
        new_node = new TornamentBarrier(cur);
        nodes.push_back(new_node);
        nodes.push_back(new_node->getPartner());
        temp_nodes.pop();
    }
}

int main(int argc, char *argv[]) {
    root = new TornamentBarrier();
    root->setRootMark();
    root->setPartner(new TornamentBarrier());
    root->getPartner()->setPartner(root);

    buildTree(root, TREE_DEPTH);

    vector<thread> threads;

    chrono::system_clock::time_point start = chrono::system_clock::now();
    for(int i=0; i<THREAD_NUM; i++)
        threads.push_back(thread(useTB,i));
    for(int i=0; i<THREAD_NUM; i++)
        threads[i].join();
    chrono::system_clock::time_point end = chrono::system_clock::now();

    // print out result
    chrono::microseconds us = chrono::duration_cast<chrono::microseconds>(end - start);
    cout << "Total execution time: " << us.count() << " us" << endl;

    return 0;
}