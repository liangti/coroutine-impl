#include <gtest/gtest.h>

#include <coroutine.h>

int global_state = 0;

class GStateToOne: public Coroutine{
public:

    void routine(){
        global_state = 1;
    }
};

class GStateToTwo: public Coroutine{
public:
    void routine(){
        global_state = 2;
    }
};

TEST(copy_stack, simple) {
    char start = 'x';
    StackBottom = &start;
    GStateToOne* one = new GStateToOne();
    GStateToTwo* two = new GStateToTwo();
    ASSERT_EQ(global_state, 0);
    resume(one);
    ASSERT_EQ(global_state, 1);
    resume(two);
    ASSERT_EQ(global_state, 2);
    // Coroutine* c = new Coroutine();
}