#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <vector>
#include "barrier.hpp"

using namespace std;

static atomic<bool> done_flag(true);

class StaticTreeBarrier : public AtomicCountBarrier {
private:
    StaticTreeBarrier *parent;
public:
    StaticTreeBarrier() : AtomicCountBarrier() {}
    StaticTreeBarrier(int counter) : AtomicCountBarrier(counter) {}
    StaticTreeBarrier(StaticTreeBarrier* _parent, int counter) : AtomicCountBarrier(counter), parent(_parent) {}

    void await(bool);
    void setParent(StaticTreeBarrier *_parent) { parent = _parent; }

    StaticTreeBarrier *getParent() { return parent; }
};

void StaticTreeBarrier::await(bool mySense) {
    if(this->parent == NULL) {
        while(this->getCounter() != 0);
        this->flipSense();
        done_flag.store(mySense);
    } else {
        while(this->getCounter() != 0);
        this->parent->decrementCounter();
        while(done_flag.load() != mySense);
    }
}

// void resetAll(vector<StaticTreeBarrier> &tree) {


StaticTreeBarrier *nodes;

void buildStaticTree(int node_count) {
    nodes = new StaticTreeBarrier[node_count+1];

    int depth = 0, tmp_count = node_count;
    while(tmp_count != 1) {
        tmp_count = tmp_count >> 1;
        ++depth;
    }
    
    nodes[1].setCounter(2);
    nodes[1].setSize(2);
    for(int i=1; i<(depth); i++){
        for(int j=0; j<(1<<i); j++) {
            nodes[(1<<i)+j].setParent(&nodes[(1<<(i-1))+(j/2)]);
            nodes[(1<<i)+j].setCounter(2);
            nodes[(1<<i)+j].setSize(2);
        }
    }
    nodes[(1<<(depth-1))].setCounter(1);
    nodes[(1<<(depth-1))].setSize(1);
    for(int j=1; j<(1<<(depth-1)); j++) {
        nodes[(1<<(depth-1))+j].setCounter(0);
        nodes[(1<<(depth-1))+j].setSize(0);
    }
    nodes[(1<<depth)].setCounter(0);
    nodes[(1<<depth)].setSize(0);
    nodes[(1<<depth)].setParent(&nodes[1<<(depth-1)]); 
                                              
    
}

void useSTB(int thread_no) {
    nodes[thread_no].await(false);
}


int main(int argc, char *argv[])
{
    buildStaticTree(THREAD_NUM);
    vector<thread> threads;

    chrono::system_clock::time_point start = chrono::system_clock::now();
    for(int i=0; i<THREAD_NUM; i++)
        threads.push_back(thread(useSTB, i+1));
    for(int i=0; i<THREAD_NUM; i++)
        threads[i].join();
    chrono::system_clock::time_point end = chrono::system_clock::now();

    // print out result
    chrono::microseconds us = chrono::duration_cast<chrono::microseconds>(end - start);
    cout << "Total execution time: " << us.count() << " us" << endl;

    return 0;
}
