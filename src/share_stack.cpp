#include <iostream>
#include <cmath>

#include <share_stack.h>

using namespace share_stack_impl;

// Enable setjmp line tracing: #define SHARE_STACK_DEBUG

// Enable remove used task from linkedlist: #define SHARE_STACK_REMOVE_USED_TASK
// The benefit of removing used task from linkedlist is saving searching time
// Currently it is broken since it introduce cycle to some nodes

// static variables
static Task MainTask;
static jmp_buf TmpEnv;
static Coroutine *Current = 0;
Coroutine* Coroutine::ToBeResume = 0;

static class MainCoroutine: public Coroutine{
public:
    MainCoroutine() {
        Current = this;
        id = "main";
    }
    void routine();
} Main;

Coroutine::Coroutine(size_t stack_size, std::string id): stack_size(stack_size), id(id){
    caller = 0;
    callee = 0;
    ready = 1;
    terminated = 0;
}

Coroutine::Coroutine(size_t stack_size): Coroutine::Coroutine(stack_size, "unknown"){

}

Coroutine::Coroutine(std::string id): Coroutine::Coroutine(DEFAULT_STACK_SIZE, id){

}

Coroutine::Coroutine(): Coroutine::Coroutine(DEFAULT_STACK_SIZE, "unknown"){

}

Coroutine::~Coroutine(){}

std::string Coroutine::get_id(){
    return id;
}

void Coroutine::enter(){
    #ifdef SHARE_STACK_DEBUG
    std::cout << "Coroutine ID: " << id << std::endl;
    #endif
    if(!Current){
        EmitError("Enter without initializing!");
    }
    if(ready){
        // search for fixed block
        // MainTask always back to MainTask
        for(task = MainTask.suc; task != &MainTask; task=task->suc){
            if(task->size >= stack_size){
                break;
            }
        }
        if(task == &MainTask){
            EmitError("No space available!");
        }
        task->co = this;
        ready = 0;
        if(!setjmp(TmpEnv)){
            longjmp(task->env, 1);
        }
        else{  
            #ifdef SHARE_STACK_DEBUG
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            #endif
        }
    }
    if(!setjmp(Current->task->env)){
        Current = this;
        longjmp(task->env, 1);
    }
    else{
        #ifdef SHARE_STACK_DEBUG
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        #endif
    }
}

void Coroutine::eat(){
    static size_t share_offset;
    static Task* share_task;
    Task local_task;

    // acquire more stack space
    // task is local_task from previous frame
    // share_offset is the stack pointer offset and is available stack space
    if((share_offset = std::labs((char*)&local_task - (char*)task)) < stack_size){
        eat();
    }
    // local_task get the rest stack space
    local_task.size = task->size - share_offset;
    task->size = share_offset;
    local_task.used = 0;
    // MainTask <-> local_task <-> MainTask.suc
    local_task.suc = MainTask.suc;
    local_task.pred = &MainTask;
    local_task.suc->pred = &local_task;
    MainTask.suc = &local_task;

    // task <-> local_task <-> task->next
    if(task->next != &local_task){
        local_task.next = task->next;
        task->next = &local_task;
        local_task.prev = task;
        if(local_task.next){
            local_task.next->prev = &local_task;
        }
    }

    // since MainTask <-> local_task it will go back here from MainTask
    if(!setjmp(local_task.env)){
        // jump back to resetSequence() at very beginning
        longjmp(task->env, 1);
    }
    else{  
        #ifdef SHARE_STACK_DEBUG
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        #endif
    }

    // infinite loop, will jump out by longjmp
    while(1){
        // split task block
        if(stack_size < local_task.size && !setjmp(local_task.env)){
            local_task.co->eat();
        }
        else{  
            #ifdef SHARE_STACK_DEBUG
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            #endif
        }
        local_task.used = 1;

        // pred <-> suc, remove local_task from linkedlist
        // TODO: after this local_task.suc become a cycle
        #ifdef SHARE_STACK_REMOVE_USED_TASK
        local_task.pred->suc = local_task.suc;
        local_task.suc->pred = local_task.pred;
        #endif

        if(!setjmp(local_task.env)){
            longjmp(TmpEnv, 1);
        }
        else{  
            #ifdef SHARE_STACK_DEBUG
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            #endif
        }

        local_task.co->routine(); // execute routine
        local_task.co->terminated = 1;
        local_task.used = 0; // mark task as free

        // coalesce task block by combining task.next & task
        share_task = local_task.next;
        if(share_task && !share_task->used){
            local_task.size += share_task->size;
            local_task.next = share_task->next;
            if(local_task.next){
                local_task.prev = share_task;
            }
        }
        // coalesce task block by combining task.prev & task(now task+task.next)
        share_task = local_task.prev;
        if(!share_task->used){
            share_task->size += local_task.size;
            share_task->next = local_task.next;
            if(local_task.next){
                local_task.next->prev = share_task;
            }
        }
        // add local_task back to linkedlist MainTask <-> local_task <-> MainTask.suc
        else{
            #ifdef SHARE_STACK_REMOVE_USED_TASK
            local_task.suc = MainTask.suc;
            local_task.pred = &MainTask;
            local_task.suc->pred = &local_task;
            MainTask.suc = &local_task;
            #endif
        }

        if(!setjmp(local_task.env)){
            if(ToBeResume){
                static Coroutine *Next;
                Next = ToBeResume;
                ToBeResume = 0;
                resume(Next);
            }
            else{
                detach();
            }
        }
        else{  
            #ifdef SHARE_STACK_DEBUG
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            #endif
        }
    }
}

void share_stack_impl::resetSequence(size_t main_stack_size){
    Task local_task;
    local_task.size = MAX_STACK_SIZE;
    Main.stack_size = main_stack_size;
    local_task.next = 0;

    // Main Coroutine start from local_task
    Main.task = &local_task;

    // MainTask <-> MainTask
    MainTask.pred = &MainTask;
    MainTask.suc = &MainTask;

    // start from Main Coroutine
    local_task.co = &Main;
    Current = &Main;

    if(!setjmp(local_task.env)){
        Main.eat();
    }
    // after eat jump back MainTask equals to local_task
    // which means get all items in local_task including (env, size, used)
    local_task.pred = MainTask.pred;
    local_task.suc = MainTask.suc;
    MainTask = local_task;
    MainTask.next->prev = &MainTask;
    Main.task = &MainTask;
    MainTask.used = 1;
    Main.ready = 0;
}

bool share_stack_impl::terminated(Coroutine* co){
    return co->terminated;
}

void share_stack_impl::resume(Coroutine* next){
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

void share_stack_impl::call(Coroutine* next){
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

void share_stack_impl::detach(){
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

Task* share_stack_impl::getMainTask(){
    return &MainTask;
}

Coroutine* share_stack_impl::getCurrentCoroutine(){
    return Current;
}