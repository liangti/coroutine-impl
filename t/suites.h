#include <gtest/gtest.h>
#include <functional>
#include <algorithm>

#ifdef COPY_STACK_IMPL
#include <copy_stack.h>
using namespace copy_stack_impl;
#endif

#ifdef SHARE_STACK_IMPL
#include <share_stack.h>
using namespace share_stack_impl;
#endif

class SimpleFlowTest: public ::testing::Test{
public:
    static int state;
protected:
    void SetUp() override{
        state = 0;
    }
    void TearDown() override{
        state = 0;
    }
};

int SimpleFlowTest::state = 0;

class SimpleStateAddOne: public Coroutine{
public:
    void routine(){
        SimpleFlowTest::state++;
    }
};

class NestFlowTest: public ::testing::Test{
public:
    const static int max = 10;
    static int* states;
    static int idx;
    static void add(int value){
        if(idx < max){
            states[idx++] = value;
        }
    }
protected:
    void SetUp() override{
        idx = 0;
        std::fill(states, states + max, 0);
    }
    void TearDown() override{
        idx = 0;
        std::fill(states, states + max, 0);
    }
};

int* NestFlowTest::states = new int[NestFlowTest::max];

int NestFlowTest::idx = 0;

class GStatesOperator: public Coroutine{
public:
    Coroutine* next;
    GStatesOperator(int value, std::function<void(Coroutine*)> jump, std::string id="unknown")
    : value(value), jump(jump), Coroutine(id){}
    void routine(){
        NestFlowTest::add(value);
        if(next){
            jump(next);
        }
        NestFlowTest::add(value);
    }
private:
    int value;
    std::function<void(Coroutine*)> jump;
};

class GStatesDoubleOperator: public Coroutine{
public:
    Coroutine* next;
    GStatesDoubleOperator(int value, std::function<void(Coroutine*)> jump, std::string id="unknown")
    : value(value), jump(jump), Coroutine(id){}
    void routine(){
        NestFlowTest::add(value);
        if(next){
            jump(next);
        }
        NestFlowTest::add(value);
        if(next){
            jump(next);
        }
        NestFlowTest::add(value);
    }
private:
    int value;
    std::function<void(Coroutine*)> jump;
};