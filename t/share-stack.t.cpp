#define SHARE_STACK_IMPL
#include <suites.h>

#include <share_stack.h>

using namespace share_stack_impl;

// helper function
void validate_pred_suc_linkedlist(){
    // pred suc linkedlist must be cycle, main back to main
    Task* main = getMainTask();
    Task* ptr;
    for(ptr = main->suc; ptr != main; ptr = ptr->suc){
        ASSERT_EQ(ptr, ptr->pred->suc);
        // detect self cycle    
        ASSERT_NE(ptr, ptr->suc);
    }
}

void validate_prev_next_linkedlist(){
    // prev next linkedlist can be broken, so that next could be NULL
    Task* main = getMainTask();
    Task* ptr;
    for(ptr = main->next; ptr && ptr != main; ptr = ptr->next){
        ASSERT_EQ(ptr, ptr->prev->next);
    }
}

// number of tasks avaliable(MainTask not count)
size_t number_of_tasks(){
    Task* main = getMainTask();
    Task* ptr;
    size_t count = 0;
    for(ptr = main->suc; ptr != main; ptr = ptr->suc){
        count++;
        if(ptr == ptr->suc){
            return 0;
        }
    }
    return count;
}

class LinkedListTest: public ::testing::Test{
protected:
    void SetUp() override{}
    void TearDown() override{
        validate_pred_suc_linkedlist();
        validate_prev_next_linkedlist();
    }
};

TEST_F(LinkedListTest, reset_state){
    COSTART 
    ASSERT_EQ(number_of_tasks(), 1);

    Task* main = getMainTask();
    // MainTask <-> local_task <-> MainTask
    ASSERT_EQ(main->suc->suc, main);
    ASSERT_EQ(main->pred->pred, main);

    // MainTask <-> local_task <-> null
    ASSERT_EQ(main->next->next, nullptr);
    ASSERT_EQ(main->next, main->suc);

    // size equals to `share_offset` in eat()
    // TODO: exact number here is very fragile, any new line will break this
    // ASSERT_EQ(main->size, DEFAULT_STACK_SIZE + 80);
}

TEST_F(SimpleFlowTest, share_stack_resume){
    COSTART
    SimpleStateAddOne* one = new SimpleStateAddOne();
    SimpleStateAddOne* two = new SimpleStateAddOne();
    ASSERT_EQ(SimpleFlowTest::state, 0);
    resume(one);
    ASSERT_EQ(SimpleFlowTest::state, 1);
    resume(two);
    ASSERT_EQ(SimpleFlowTest::state, 2);
}

// infinite loop, second task node point to itself
// everytime a new added task is suc to MainTask
// so local_task.suc always fresh
// it is local_task that change its suc so that suc->suc == suc 
TEST_F(LinkedListTest, share_stack_resume){
    COSTART
    SimpleStateAddOne* one = new SimpleStateAddOne();
    SimpleStateAddOne* two = new SimpleStateAddOne();
    ASSERT_EQ(number_of_tasks(), 1);
    resume(one);
    ASSERT_EQ(number_of_tasks(), 2);
    resume(two);
    ASSERT_EQ(number_of_tasks(), 3);
}

TEST_F(SimpleFlowTest, share_stack_call) {
    COSTART
    SimpleStateAddOne* one = new SimpleStateAddOne();
    SimpleStateAddOne* two = new SimpleStateAddOne();
    ASSERT_EQ(SimpleFlowTest::state, 0);
    call(one);
    ASSERT_EQ(SimpleFlowTest::state, 1);
    call(two);
    ASSERT_EQ(SimpleFlowTest::state, 2);
}

TEST_F(NestFlowTest, share_stack_resume){
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