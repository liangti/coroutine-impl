#ifndef COROUTINE_COPY_STACK_IMPL
#define COROUTINE_COPY_STACK_IMPL

#include <stddef.h>
#include <setjmp.h>
#include <string>

namespace copy_stack_impl{
    
struct CoException: public std::exception{
public:
    CoException(std::string message): message(message){}
    inline std::string get_message(){
        return message;
    }
private:
    std::string message;
};

class Coroutine{
// API expose to user
    friend void resume(Coroutine*);
    friend void call(Coroutine*);
    friend void detach();
    friend bool terminated(Coroutine*);

// user of the class is not allow to construct/run/destruct coroutine directly
// derived class can override following functions to achieve custom logics
protected:
    // make routine pure virtual function so we don't have to define it in coroutine.cpp
    virtual void routine() = 0;
    // derived class is allowed to change coroutine id
    std::string id;
public:
    Coroutine(std::string id="unknown");
    ~Coroutine();
    std::string get_id();
    // reset stack buffer
    void reset();

// user of the class is not allow to directly access following membergs
// derived class is not allow to modify following core members
private:
    void enter();
    // store runtime stack to buffer
    void store_stack();
    // store buffer to runtime stack
    void restore_stack();

    char* stack_buffer;
    char* low;
    char* high;
    size_t buffer_size;
    jmp_buf env;
    Coroutine* caller;
    Coroutine* callee;
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
void resetSequence(char*);

// call COSTART macro before every sequence
// it has to be macro since local have be
// in same stack of corresponding coroutines
#define COSTART {char local; resetSequence(&local);}

}; // namespace copy_stack_impl

#endif