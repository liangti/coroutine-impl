# coroutine-impl

C++ Implementation of Coroutine

From paper: http://akira.ruc.dk/~keld/research/COROUTINE/COROUTINE-1.0/DOC/COROUTINE_REPORT.pdf

## Background

Normal function call like A calls B, execution control will back to A ONLY after B 
finished. The stack of B will get clean up and A will continue from last function call(B) 
to execute the rest of the code.

Coroutine is a concept that allows execution to be passed around two tasks A and B,
so that while A executes it could yield to B and while B wants to stop it could yield back 
to A to let A continue.

This is useful in async IO scenario. For example A and B are both waiting for different fd
and keep active so the execution control needs to be passed around A and B

## Copy Stack

Is the first method the paper mentions.

Traditional function call will clean up stack so when B back to A, stack belongs to B will
be cleaned. So that B cannot be resumed.

This method copy runtime stack to heap while a coroutine decide to yield to next coroutine.

When execution control back to current coroutine, recover runtime stack before yield by
copying from heap.

Memory copy could be the bottleneck of this method

## Share Stack

Is the second method the paper presents.

Still focus on how to keep function runtime stack not being cleaned up.

The idea is to have a never return infinite while loop that consume routines. This function
must be in a safe place so that it does not corrupt:
- normal runtime stack(non coroutine parts)
- heap space

In the demo code from the paper it specifies a fixed size(or let user specify) and use
recursive call to climb to that location and start infinite loop there.

The way to return from infinite loop is by `setjmp/longjmp` so that the runtime stack
never gets cleaned up.

Meanwhile the infinite loop also keep climbing the stack to pre-allocate necessary runtime
stack space. When it climb to fixed stack size it stop and the place it start to the place
it stop becomes available runtime stack. Next routine will start execute from the place it
start, enter by `setjmp/longjmp` also.

### Caveat

This implementation highly relies on the whether the stack size is safe or not. If it is
too small it will corrupt runtime stack, if it is too large either stackoverflow or it
corrupt heap space.

> Could it be possible to protect that piece of memory from being consume by runtime stack
and also heap(malloc for example)?