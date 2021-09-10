#include <gtest/gtest.h>
#include <functional>
#include <algorithm>

#include <coroutine.h>

using namespace CoAPI;


class SimpleFlowTest: public ::testing::Test{
public:
    static int state;
protected:
    void SetUp() override{
        state = 0;
        resetSequence();
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

TEST_F(SimpleFlowTest, copy_stack_create_coroutine_on_stack){
    EXPECT_THROW({
        SimpleStateAddOne error;
    }, CoException);
}

TEST_F(SimpleFlowTest, copy_stack_create_next_coroutine_on_stack){
    SimpleStateAddOne* first = new SimpleStateAddOne();
    EXPECT_THROW({
        SimpleStateAddOne next;
    }, CoException);
}

TEST_F(SimpleFlowTest, copy_stack_resume) {
    SimpleStateAddOne* one = new SimpleStateAddOne();
    SimpleStateAddOne* two = new SimpleStateAddOne();
    ASSERT_EQ(SimpleFlowTest::state, 0);
    resume(one);
    ASSERT_EQ(SimpleFlowTest::state, 1);
    resume(two);
    ASSERT_EQ(SimpleFlowTest::state, 2);
}

TEST_F(SimpleFlowTest, copy_stack_call) {
    SimpleStateAddOne* one = new SimpleStateAddOne();
    SimpleStateAddOne* two = new SimpleStateAddOne();
    ASSERT_EQ(SimpleFlowTest::state, 0);
    call(one);
    ASSERT_EQ(SimpleFlowTest::state, 1);
    call(two);
    ASSERT_EQ(SimpleFlowTest::state, 2);
}

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
        resetSequence();
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
    : value(value), jump(jump), Coroutine(0, id){}
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

TEST_F(NestFlowTest, copy_stack_resume){
    GStatesOperator* one = new GStatesOperator(1, resume);
    GStatesOperator* two = new GStatesOperator(2, resume);
    one->next = two;
    resume(one);
    ASSERT_EQ(NestFlowTest::idx, 3);
    ASSERT_EQ(NestFlowTest::states[0], 1);
    ASSERT_EQ(NestFlowTest::states[1], 2);
    ASSERT_EQ(NestFlowTest::states[2], 2);
}

TEST_F(NestFlowTest, copy_stack_call){
    GStatesOperator* one = new GStatesOperator(1, call);
    GStatesOperator* two = new GStatesOperator(2, call);
    GStatesOperator* three = new GStatesOperator(3, call);
    one->next = two;
    two->next = three;
    call(one);
    ASSERT_EQ(NestFlowTest::idx, 6);
    ASSERT_EQ(NestFlowTest::states[0], 1);
    ASSERT_EQ(NestFlowTest::states[1], 2);
    ASSERT_EQ(NestFlowTest::states[2], 3);
    ASSERT_EQ(NestFlowTest::states[3], 3);
    ASSERT_EQ(NestFlowTest::states[4], 2);
    ASSERT_EQ(NestFlowTest::states[5], 1);
}

TEST_F(NestFlowTest, copy_stack_call_overlap_sequences){
    GStatesOperator* one = new GStatesOperator(1, call);
    GStatesOperator* two = new GStatesOperator(2, call);
    one->next = two;
    two->next = one;
    call(one);
    ASSERT_EQ(NestFlowTest::idx, 4);
    ASSERT_EQ(NestFlowTest::states[0], 1);
    ASSERT_EQ(NestFlowTest::states[1], 2);
    ASSERT_EQ(NestFlowTest::states[2], 1);
    ASSERT_EQ(NestFlowTest::states[3], 2);
}

class GStatesDoubleOperator: public Coroutine{
public:
    Coroutine* next;
    GStatesDoubleOperator(int value, std::function<void(Coroutine*)> jump, std::string id="unknown")
    : value(value), jump(jump), Coroutine(0, id){}
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

TEST_F(NestFlowTest, copy_stack_call_overlap_complex_sequences){
    GStatesDoubleOperator* one = new GStatesDoubleOperator(1, call);
    GStatesDoubleOperator* two = new GStatesDoubleOperator(2, call);
    GStatesDoubleOperator* three = new GStatesDoubleOperator(3, call);
    one->next = two;
    two->next = three;
    three->next = one;
    call(one);
    ASSERT_EQ(NestFlowTest::idx, 9);
    ASSERT_EQ(NestFlowTest::states[0], 1);
    ASSERT_EQ(NestFlowTest::states[1], 2);
    ASSERT_EQ(NestFlowTest::states[2], 3);
    ASSERT_EQ(NestFlowTest::states[3], 1);
    ASSERT_EQ(NestFlowTest::states[4], 2);
    ASSERT_EQ(NestFlowTest::states[5], 3);

    // while teardown 1 finish and then call parent 3 and then call parent 2
    ASSERT_EQ(NestFlowTest::states[6], 1);
    ASSERT_EQ(NestFlowTest::states[7], 3);
    ASSERT_EQ(NestFlowTest::states[8], 2);
}