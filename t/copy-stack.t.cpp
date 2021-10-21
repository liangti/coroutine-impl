#define COPY_STACK_IMPL
#include <suites.h>

#include <copy_stack.h>

using namespace copy_stack_impl;


TEST_F(SimpleFlowTest, copy_stack_create_coroutine_on_stack){
    EXPECT_THROW({
        COSTART
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
    COSTART
    SimpleStateAddOne* one = new SimpleStateAddOne();
    SimpleStateAddOne* two = new SimpleStateAddOne();
    ASSERT_EQ(SimpleFlowTest::state, 0);
    resume(one);
    ASSERT_EQ(SimpleFlowTest::state, 1);
    resume(two);
    ASSERT_EQ(SimpleFlowTest::state, 2);
}

TEST_F(SimpleFlowTest, copy_stack_call) {
    COSTART
    SimpleStateAddOne* one = new SimpleStateAddOne();
    SimpleStateAddOne* two = new SimpleStateAddOne();
    ASSERT_EQ(SimpleFlowTest::state, 0);
    call(one);
    ASSERT_EQ(SimpleFlowTest::state, 1);
    call(two);
    ASSERT_EQ(SimpleFlowTest::state, 2);
}

TEST_F(NestFlowTest, copy_stack_resume){
    COSTART
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
    COSTART
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
    COSTART
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


TEST_F(NestFlowTest, copy_stack_call_overlap_complex_sequences){
    COSTART
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