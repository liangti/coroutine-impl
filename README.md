# coroutine-impl

C++ Implementation of Coroutine

From paper: http://akira.ruc.dk/~keld/research/COROUTINE/COROUTINE-1.0/DOC/COROUTINE_REPORT.pdf

## Background

Normal function call like A calls B, execution will back to A ONLY after B finished. The stack of B will get clean up and A will continue from last function call(B) to
execute the rest of the code.

Coroutine is a concept that allow execution to be passed around two tasks A and B,
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

TODO