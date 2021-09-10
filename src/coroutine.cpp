#include <iostream>
#include <stdlib.h>

#include <coroutine.h>
#include <exception>


static char* StackBottom = 0;

using namespace CoAPI;

// global/static data
static Coroutine* Current = 0;
static Coroutine* Next;
Coroutine* Coroutine::to_be_resume = 0;
static class MainCoroutine: public Coroutine{
public:
    MainCoroutine() {
        Current = this;
        id = "main";
    }
    void routine(){}
} Main;

// global utilities
void EmitError(const char* message){
    std::cerr << "Error: " << message << std::endl;
}

// global external utilities
void EmitLog(const char* message){
    std::cout << "Log: " << message << std::endl;
}

std::string Coroutine::get_id(){
    return id;
}

Coroutine::Coroutine(size_t dummy, std::string co_id){
    char stack_local;
    if(&stack_local < (char*)this){
        throw CoException("Attempting to allocate a Coroutine on stack");
    }
    reset();
    buffer_size = dummy;
    id = co_id;
}

void Coroutine::reset(){
    stack_buffer = 0;
    low = 0;
    high = 0;
    callee = 0;
    caller = 0;
}

Coroutine::~Coroutine(){
    delete stack_buffer;
    stack_buffer = 0;
}

bool CoAPI::terminated(Coroutine* c){
    // if stack_buffer is deleted but buffer_size is larger than 0
    // we say the coroutine is terminated
    return (!c->stack_buffer) && (c->buffer_size > 0);
}

// storeStack, restoreStack, enter has to be inline
// because they must be in exact SAME stack where
// resume/call is in!!!

inline void Coroutine::restore_stack(){
    char local;
    if (&local >= low && &local <= high){
        restore_stack();
    }
    Current = this;
    // store stack_buffer to runtime stack
    memcpy(low, stack_buffer, high - low);
    longjmp(Current->env, 1);
}

inline void Coroutine::store_stack(){
    if(!low){
        if(!StackBottom){
            throw CoException("StackBottom is not initialized");
        }
        low = StackBottom;
        high = StackBottom;
    }
    char stack_local;
    if(&stack_local > StackBottom){
        high = &stack_local;
    }
    else{
        low = &stack_local;
    }
    if(high - low > buffer_size || !stack_buffer){
        if (stack_buffer){
            delete stack_buffer;
        }
        buffer_size = high - low;
        if(!(stack_buffer = new char[buffer_size])){
            throw CoException("No more space available");
        }
    }

    // store runtime stack(low, high) to stack_buffer
    memcpy(stack_buffer, low, high - low);
}

inline void CoAPI::Coroutine::enter(){
    // determine if current coroutine is still running
    if (!terminated(this)){
        Current->store_stack();
        // Current->restoreStack() will go back here
        // execute the rest of the code previous coroutine left
        if(setjmp(Current->env)){
            return;
        }
    }
    Current = this;
    if (!stack_buffer){
        routine();
        delete Current->stack_buffer;
        Current->stack_buffer = 0;
        detach();
        return;
    }
    restore_stack();
}

void CoAPI::resume(Coroutine* next){
    if (!next){
        throw CoException("Attempt to resume an empty coroutine");
    }
    if (next == Current){
        return;
    }
    if (terminated(next)){
        throw CoException("Attempt to resume a terminated coroutine");
    }

    next->enter();
}

void CoAPI::call(Coroutine* next){
    if (!next){
        throw CoException("Attempt to call an empty coroutine");
    }
    if (next == Current){
        return;
    }
    if (terminated(next)){
        throw CoException("Attempt to call a terminated coroutine");
    }
    // current coroutine call next
    // so that current is next's caller, next is current's callee
    // this is the only difference between call and resume
    // after next finish, call will return to current
    Current->callee = next;
    next->caller = Current;

    next->enter();
}

void CoAPI::detach(){
    Coroutine* parent = Current->caller;
    if (parent){
        Current->caller = 0;
        parent->callee = 0;
    }
    else {
        parent = &Main;
        if (parent->callee){
            parent = parent->callee;
        }
    }
    parent->enter();
}

void CoAPI::resetSequence(char* start){
    Main.reset();
    Current = &Main;
    StackBottom = start;
}

// internal functions

Coroutine* currentCoroutine(){
    return Current;
}

Coroutine* mainCoroutine(){
    return &Main;
}