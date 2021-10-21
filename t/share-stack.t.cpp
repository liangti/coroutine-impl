#define SHARE_STACK_IMPL
#include <suites.h>

#include <share_stack.h>

using namespace share_stack_impl;

// while eat() climbing stack it will have extra align space
// different from stack_size to stack_size
// This 80 is the align number for DEFAULT_STACK_SIZE
#define EXTRA_ALIGN_SPACE 80


class CustomStackSizeCoroutine: public Coroutine{
public:
    CustomStackSizeCoroutine(size_t stack_size): Coroutine(stack_size){}
    void routine(){}
};

/*
A test case to demonstrate how share stack works.

The eat() function is actually climbing stack and allocating a space
far from current function frame. And it never returns so stack of eat()
never recycle but just keep there. Any time we want, longjmp() can
bring us back to eat().

MainTask is actually in static section. The gap between stack_start and
first task is decided by `main_stack_size`. It is to guarantee that coroutine
stack will not corrupt current function frame.

+--TEST_F()--------------^
+                        | 
+                        |
+--frame_bottom----------| 
+                   main_stack_size
...                      |
+                        |
+--MainTask.eat()--------^
+                        |
+                   coroutine_stack_size
+                        |
+--FirstTask.eat()-------^
+                        |
+                        |
+(task lives here)-------^
+                   coroutine_stack_size
+                        | 
+--SecondTask.eat()------^
+
+(task lives here)
+
....
*/
TEST(share_stack_impl_test, visualize_stack_pointer){
    size_t stack_start;
    // verify that it is main_stack_size making effort
    size_t main_stack_size = DEFAULT_STACK_SIZE - 1000;
    // verify that it is coroutine_stack_size making effort
    size_t coroutine_stack_size = DEFAULT_STACK_SIZE - 2000;
    resetSequence(main_stack_size);
    // Macro COSTART call resetSequence by using default main_stack_size
    CustomStackSizeCoroutine* co1 = new CustomStackSizeCoroutine(coroutine_stack_size);
    CustomStackSizeCoroutine* co2 = new CustomStackSizeCoroutine(coroutine_stack_size);
    CustomStackSizeCoroutine* co3 = new CustomStackSizeCoroutine(coroutine_stack_size);
    resume(co1);
    resume(co2);
    resume(co3);
    Task* main = getMainTask();
    Task* ptr;
    size_t stack_gap = 0;
    for(ptr = main->suc; ptr != main; ptr = ptr->suc){
        // first inserted task, the one that is closest to current stack
        if(ptr->suc == main){
            // the gap is main_stack_size + align
            // so it must be bigger than
            std::cout << (char*)&stack_start - (char*)ptr << std::endl; 
            ASSERT_GT((char*)&stack_start - (char*)ptr, main_stack_size + EXTRA_ALIGN_SPACE);
        }
        if(ptr->pred != main){
            size_t current_stack_gap = (char*)ptr - (char*)ptr->pred;
            ASSERT_GT(current_stack_gap, coroutine_stack_size + EXTRA_ALIGN_SPACE);
            // gap for each task address is fixed since coroutine_stack_size is fixed
            if(stack_gap > 0){
                ASSERT_EQ(stack_gap, current_stack_gap);
            }
            else{
                stack_gap = current_stack_gap;
            }
        }
    }
}

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
    ASSERT_EQ(main->size, DEFAULT_STACK_SIZE + EXTRA_ALIGN_SPACE);
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