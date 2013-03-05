// ==++== 
//
//    Copyright (c) 2009 Microsoft Corporation.  All rights reserved.
//
// This mimics parallel_for functionality by doing some baisc chunking and scheduling LWT's and waiting on them via event
//
// ==--==

#include <tchar.h>
#include <concrt.h>
#include <ppl.h>
#include <windows.h>

using namespace Concurrency;

typedef void (__cdecl * SimpleParallelForProc)(int);

/// <summary>
/// Structured used by SimpleParallelForWrapper 
/// </summary>
template <typename _Index_type, typename _Function>
struct SimpleParallelForData
{
    SimpleParallelForData() : m_proc(NULL), m_beginInc(0), m_endExc(0), m_pSyncronizer(NULL), m_concurrencyLevel(0), m_pCompletionCount(0) {}

    SimpleParallelForData(_Function proc) : m_proc(proc), m_beginInc(0), m_endExc(0), m_pSyncronizer(NULL), m_concurrencyLevel(0), m_pCompletionCount(0) {}

    void Set(_Index_type beginInc, _Index_type endExc, event* pSyncronizer, int concurrencyLevel, volatile LONG* pCompletionCount )
    { 
        m_beginInc = beginInc; 
        m_endExc = endExc; 
        m_pSyncronizer = pSyncronizer;
        m_concurrencyLevel = concurrencyLevel;
        m_pCompletionCount = pCompletionCount;
    }

    _Function m_proc;
    _Index_type m_beginInc;
    _Index_type m_endExc;
    event* m_pSyncronizer;
    int m_concurrencyLevel;
    volatile LONG* m_pCompletionCount;
};

/// <summary>
/// LWT that executes chunks of work serially 
/// </summary>
///
/// <param name="pSimpleParallelForDataAddress">SimpleParallelForData data<param>
template <typename _Index_type, typename _Function>
void __cdecl SimpleParallelForWrapper( void* pSimpleParallelForDataAddress )
{
    SimpleParallelForData<_Index_type, _Function>* pSimpleParallelForData = (SimpleParallelForData<_Index_type, _Function>*) pSimpleParallelForDataAddress;

    _Index_type beginInc = pSimpleParallelForData->m_beginInc;
    _Index_type endExc = pSimpleParallelForData->m_endExc;
    _Function proc = pSimpleParallelForData->m_proc;

    for( int i = beginInc; i < endExc; ++i )
    {
        proc(i);
    }

    LONG counter = InterlockedIncrement(pSimpleParallelForData->m_pCompletionCount);
    if( counter == pSimpleParallelForData->m_concurrencyLevel )
    {
        pSimpleParallelForData->m_pSyncronizer->set();
    }
}

/// <summary>
/// This mimics parallel_for where we do some basic chunking and schedule LWTs that do work
/// ideal when work done within parallel_for is not considerable
/// </summary>
///
/// <param name="beginInc">Begin index<param>
/// <param name="endExc">End index<param>
/// <param name="proc">Function to execute in parallel<param>
template <typename _Index_type, typename _Function>
void SimpleParallelFor( _Index_type beginInc, _Index_type endExc, _Function proc )
{
    event syncronizer;
    volatile LONG completionCount = 0;

    int concurrencyLevel = CurrentScheduler::GetPolicy().GetPolicyValue(MaxConcurrency);
    _Index_type chunkSize = ( endExc - beginInc ) / concurrencyLevel;
    _Index_type remainder = ( endExc - beginInc ) % concurrencyLevel;

    SimpleParallelForData<_Index_type, _Function>* pDataArray = (SimpleParallelForData<_Index_type, _Function>*)alloca(sizeof(SimpleParallelForData<_Index_type, _Function>) * concurrencyLevel);

    for( int i = 0; i < concurrencyLevel; ++i )
    {
        new(&pDataArray[i]) SimpleParallelForData<_Index_type, _Function>(SimpleParallelForData<_Index_type, _Function>(proc));
        if( i < remainder )
        {
            pDataArray[i].Set(beginInc, beginInc + chunkSize + 1, &syncronizer, concurrencyLevel - 1, &completionCount );
            beginInc += chunkSize + 1;
        }
        else
        {
            pDataArray[i].Set(beginInc, beginInc + chunkSize, &syncronizer, concurrencyLevel - 1, &completionCount );
            beginInc += chunkSize;
        }

        if( i == concurrencyLevel - 1 )
        {
            int beginInc = pDataArray[i].m_beginInc;
            int endExc = pDataArray[i].m_endExc;

            for( int j = beginInc; j < endExc; ++j )
            {
                proc(j);
            }
        }
        else
        {
            CurrentScheduler::ScheduleTask( SimpleParallelForWrapper<_Index_type, _Function>, &pDataArray[i]  );
        }
    }

    syncronizer.wait();
}

