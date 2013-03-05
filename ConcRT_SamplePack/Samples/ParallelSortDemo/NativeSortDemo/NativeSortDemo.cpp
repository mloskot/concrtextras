// NativeSortDemo.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"

#define TRADITIONALDLL_API __declspec(dllexport)

//exported APIs to make it callable from the managed code.
extern "C" {
   TRADITIONALDLL_API double native_sort(int sortType, int *a, int size, int concurrency, double comparatorSize);
}

#include <windows.h>
#include <time.h>
#include <algorithm>
#include <ppl.h>
#include "..\..\..\concrtextras\ppl_extras.h"

/// <summary>
/// Enum for Sort Type, needs to match
/// the enum order defined in managed code
/// </summary>
enum SortType
{
    kStdSort=0,
    kParallelSort,
    kParallelBufferedSort,
    kParallelRadixSort
};

/// <summary>
/// Spin function that does an arbitary amount
/// note: the 'work' param does not constitute a notion of time
/// </summary>
#pragma optimize("", off)
__declspec(noinline)
    void Spin(unsigned int work)
{
    for(unsigned long long i=0; i < work*work*work; ++i);
}
#pragma optimize("",on)

/// <summary>
/// Sort Function passed to the sort function
/// Takes work as a parameter, to mimic work done at
/// the comparator
/// </summary>
class SortFunc
{
public:
    /// <summary>
    /// Constructor - sets amount of work to be done 
    /// in the comparator
    /// </summary>
    SortFunc(unsigned int work)
        :m_work(work)
    {
    }

    /// <summary>
    /// Comparator called by std::sort, parallel_sort
    /// parallel_buffered_sort
    /// </summary>
    bool operator()(int left, int right) const
    {
        Spin(m_work);
        return (left < right);
    }

    /// <summary>
    /// Comparator called by parallel_radixsort
    /// </summary>
    size_t operator()(int val) const
    {
        Spin(m_work);
        return Concurrency::samples::_Radix_sort_default_function<int>()(val);
    }

private:
    /// <summary>
    /// Defines the amount of work 
    /// done in the comparator
    /// </summary>
    unsigned int m_work;
};

/// <summary>
/// Function that calls std::sort, with the input array
/// and uses SortFun structure as the comparator function
/// </summary>
void std_sort(int *a, int size, double comparatorSize)
{    
    std::sort(a, &a[size-1], SortFunc((unsigned int)comparatorSize));
}

/// <summary>
/// Function that calls Concurrency::parallel_sort, with the input array
/// and uses SortFun structure as the comparator function
/// </summary>
void ppl_parallel_sort(int *a, int size, double comparatorSize)
{
    Concurrency::samples::parallel_sort(a, &a[size-1], SortFunc((unsigned int)comparatorSize));
}

/// <summary>
/// Function that calls Concurrency::parallel_buffered_sort, with the input array
/// and uses SortFun structure as the comparator function
/// </summary>
void ppl_parallel_buffered_sort(int *a, int size, double comparatorSize)
{
    Concurrency::samples::parallel_buffered_sort(a, &a[size-1], SortFunc((unsigned int)comparatorSize));    
}

/// <summary>
/// Function that calls Concurrency::parallel_radixsort, with the input array
/// and uses SortFun structure as the comparator function
/// </summary>
void ppl_parallel_radixsort(int *a, int size, double comparatorSize)
{
    Concurrency::samples::parallel_radixsort(a, &a[size-1], SortFunc((unsigned int)comparatorSize));
}

/// <summary>
/// The function that would be invoked from the managed front
/// calls the native version of sort as specified in the parameter
/// </summary>
double native_sort(int sortType, int *a, int size, int concurrency, double comparatorSize)
{    
    volatile clock_t start, finish;
    Concurrency::CurrentScheduler::Create(Concurrency::SchedulerPolicy(2, Concurrency::MinConcurrency, concurrency, Concurrency::MaxConcurrency, concurrency));
    start = clock();

    switch(sortType)
    {
    case kStdSort:
        std_sort(a, size, comparatorSize);
        break;
    case kParallelSort:
        ppl_parallel_sort(a, size, comparatorSize);
        break;
    case kParallelBufferedSort:
        ppl_parallel_buffered_sort(a, size, comparatorSize);
        break;
    case kParallelRadixSort:
        ppl_parallel_radixsort(a, size, comparatorSize);
        break;
    }

    finish = clock();
    Concurrency::CurrentScheduler::Detach();

    return (double(finish-start)/CLOCKS_PER_SEC);
}


