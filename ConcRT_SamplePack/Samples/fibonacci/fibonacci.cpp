//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: fibonacci.cpp
//
//--------------------------------------------------------------------------
#include "windows.h"
#include <ppl.h>

using namespace Concurrency;

int SPINCOUNT = 25;

//Spins for a fixed number of loops
#pragma optimize("", off)
void delay()
{
    for(int i=0;i < SPINCOUNT;++i);
};
#pragma optimize("", on)

//Times execution of a functor in ms
template <class Functor>
__int64 time_call(Functor& fn)
{
    __int64 begin, end;
    begin = GetTickCount();
    fn();
    end = GetTickCount();
    return end - begin;
};

//Computes the fibonacci number of 'n' serially
int fib(int n)
{
    delay();
    if (n< 2)
        return n;
    int n1, n2;	
    n1 = fib(n-1);
    n2 = fib(n-2);
    return n1 + n2;
}
//Computes the fibonacci number of 'n' in parallel
int struct_fib(int n)
{
    delay();
    if (n< 2)
        return n;
    int n1, n2;

    //declare a structured task group
    structured_task_group tasks;

    //invoke the first half as a task
    auto task1 = make_task([&n1,n]{n1 = struct_fib(n-1);});
    tasks.run(task1);

    //run the second recursive call inline
    n2 = struct_fib(n-2);

    //wait for completion
    tasks.wait();

    return n1 + n2;
}

//Computes the fibonacci number of 'n' allocating storage for integers on heap
int struct_fib_heap(int n)
{
    delay();
    if (n< 2)
        return n;
    //n1 and n2 are now allocated on the heap
    int* n1;
    int* n2;

    //declare a task_group
    structured_task_group tg;	

    auto t1 = make_task([&]{
        n1 = (int*) malloc(sizeof(int));
        *n1 = struct_fib_heap(n-1);
    });
    tg.run(t1);
    n2 = (int*) malloc(sizeof(int));
    *n2 = struct_fib_heap(n-2);
    tg.wait();
    int result = *n1 + *n2;
    free(n1);
    free(n2);
    return result;
}
//Computes the fibonacci number of 'n' using the ConcRT suballocator
int struct_fib_concrt_heap(int n)
{
    delay();
    if (n< 2)
        return n;
    int* n1;
    int* n2;
    structured_task_group tg;	
    auto t1 = make_task([&]{
        n1 = (int*) Concurrency::Alloc(sizeof(int));
        *n1 = struct_fib_concrt_heap(n-1);
    });
    tg.run(t1);
    n2 = (int*) Concurrency::Alloc(sizeof(int));
    *n2 = struct_fib_concrt_heap(n-2);
    tg.wait();
    int result = *n1 + *n2;
    Concurrency::Free(n1);
    Concurrency::Free(n2);
    return result;
}
int main()
{
    int num = 30;
    SPINCOUNT = 500;
    double serial, parallel;

    //compare the timing of serial vs parallel fibonacci
    printf("computing fibonacci of %d serial vs parallel\n",num);
    printf("\tserial:   ");
    serial= (double)time_call([=](){fib(num);});
    printf("%4.0f ms\n",serial);

    printf("\tparallel: ");
    parallel = (double)time_call([=](){struct_fib(num);});
    printf("%4.0f ms\n",parallel);

    printf("\tspeedup: %4.2fX\n",serial/parallel);

    //compare the timing of malloc vs Concurrency::Alloc,
    //where we expect to get speedups because there are a large
    //number of small malloc and frees.
    
    //increase the number of tasks
    num = 34;

    //reduce the amount of 'work' in each task
    SPINCOUNT = 0;

    //execute fib using new & delete
    printf("computing fibonacci of %d using heap\n",num);
    printf("\tusing malloc:             ");
    serial= (double)time_call([=](){struct_fib_heap(num);});
    printf("%4.0f ms\n",serial);

    //execute fib using the concurrent suballocator
    printf("\tusing Concurrency::Alloc: ");
    parallel = (double)time_call([=](){struct_fib_concrt_heap(num);});
    printf("%4.0f ms\n",parallel);

    printf("\tspeedup: %4.2fX\n",serial/parallel);

    return 0;
}

