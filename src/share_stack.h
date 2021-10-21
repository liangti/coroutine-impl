#ifndef COROUTINE_SHARE_STACK_IMPL
#define COROUTINE_SHARE_STACK_IMPL

#include <setjmp.h>
#include <string>

#include <utils.h>

namespace share_stack_impl{

class Coroutine;

// internal default macros
#define MIN_STACK_SIZE 500
#define DEFAULT_STACK_SIZE 10000
#define MAX_STACK_SIZE 50000

// A control block that allocate for runtime stack
struct Task{
    Coroutine* co;
    jmp_buf env;
    size_t used;
    size_t size;
    struct Task *prev, *next;
    struct Task *pred, *suc;
};

class Coroutine{
// API expose to user
    friend void resume(Coroutine*);
    friend void call(Coroutine*);
    friend void detach();
    friend bool terminated(Coroutine*);
    friend void resetSequence(size_t);
// user of the class is not allow to construct/run/destruct coroutine directly
// derived class can override following functions to achieve custom logics
protected:
    // make routine pure virtual function so we don't have to define it in coroutine.cpp
    virtual void routine() = 0;
    // derived class is allowed to change coroutine id
    std::string id;
public:
    Coroutine(); // default constructor with default stack size and id="unknown"
    Coroutine(std::string); // custom coroutine id
    Coroutine(size_t); // custom stack size
    Coroutine(size_t, std::string);
    ~Coroutine();
    std::string get_id();
private:
    Coroutine *caller, *callee;
    size_t ready, terminated;
    Task* task;
    void enter();
    void eat();
    size_t stack_size;
    static Coroutine* ToBeResume;
};


// determine if given coroutine is terminated
bool terminated(Coroutine*);

// resumes execution from last left location
// when detach, back to main coroutine
void resume(Coroutine*);

// attach current coroutine as input coroutine's caller
// when detach, back to caller
void call(Coroutine*);

// relinquish control to its caller
void detach();

// reset sequence
void resetSequence(size_t main_stack_size=DEFAULT_STACK_SIZE);

#define COSTART resetSequence();

// internal functions

Task* getMainTask();

Coroutine* getCurrentCoroutine();

}; // namespace share_stack_impl

#endif