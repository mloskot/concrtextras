/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* Utility.h
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

#pragma once

//-------------------------------------------------------------------------------------------------
// Randomization
//------------------------------------------------------------------------------------------------

#define _USE_CONCURRENT_VECTOR
#ifdef _USE_CONCURRENT_VECTOR
    #define MyVector( X ) Concurrency::concurrent_vector< X, concrt_suballocator< X > >
    //#define MyVector( X ) Concurrency::concurrent_vector< X >
#else
    #define MyVector( X ) std::vector< X, concrt_suballocator< X > >
#endif // _USE_CONCURRENT_VECTOR

typedef minstd_rand0 MyEng;
extern combinable< MyEng > eng_combinable;
extern MyEng g_eng;

#define RANDOMIZATIONSEED 17

/// returns randoum double from 0.0 - 1.0
double
DoubleRand(
    MyEng& eng
    );

/// returns randoum int from min - max
int 
IntRand(
    MyEng& eng,
    __in const int min, 
    __in const int max 
    );

/// selects a random integer from min - max execluding lelments in skip
int 
random_select_except(
    MyEng& eng,
    __in const int min, 
    __in const int max, 
    __in const int skip[], 
    __in const int skipSize 
    );

//-------------------------------------------------------------------------------------------------
// Math functions
//------------------------------------------------------------------------------------------------

void 
Normalize( 
	MyVector( double ) &v 
	);

double 
RootMeanSquare ( 
	MyVector( double ) &v 
	);
    
double 
TrackingError ( 
	MyVector( double ) &v, 
	MyVector( double ) &r 
	);

#define MY_EPSILON DBL_EPSILON * 10
#define IN_CUT_OFF( x, y ) ( ( ( x ) > - ( y ) ) && ( ( x ) < ( y ) ) )


template <typename IndexType,  typename UnsignedType, class Func>
void parallel_for_worker(IndexType begin,IndexType stride, UnsignedType numItems, Func Fn){

    //if there's just one loop execute it...
    if (numItems == 1){
        Fn(begin);
        return;
    }
    else if (numItems == 2){
        {
            auto t1 = make_task([&](){Fn(begin);});
            Concurrency::structured_task_group t;
            t.run(t1);
            t.run_and_wait([&](){Fn(begin+stride);});
        }
        return;
    }
    else{
        UnsignedType halfCount = numItems / 2;
        IndexType mid = begin + halfCount * stride;
        {
            //push one,
            auto t1 = make_task([&](){parallel_for_worker(mid,stride,numItems-halfCount,Fn);});
            Concurrency::structured_task_group t;
            t.run(t1);

            //call recursively
            t.run_and_wait([&](){parallel_for_worker(begin, stride, halfCount,Fn);});
        }

        return;
    }

};

template <typename IndexType, class Func>
void recursive_parallel_for(IndexType begin,IndexType end,IndexType stride, Func Fn){
    if(end - begin == 0 )
        return;
    _Trace_ppl_function(PPLParallelForEventGuid, _TRACE_LEVEL_INFORMATION, CONCRT_EVENT_START);
    parallel_for_worker(begin,stride,((end-begin)/stride),Fn);
    _Trace_ppl_function(PPLParallelForEventGuid, _TRACE_LEVEL_INFORMATION, CONCRT_EVENT_END);
};

//template< class _Function >
//class CustomTask
//{
//    CustomTask(
//        _Function func,
//
//protected:
//    event _done;
//}

//
// a generic agent that takes a function pointer and call it from run()
//
template< class _Function = TaskProc >
class CustomAgent : public agent
{
public:
	CustomAgent(
            _Function newTaskProc, 
            LPVOID arg 
            ) : m_pFunction( newTaskProc ),
                m_arg( arg )
	{ };

    void SetArgument( 
        LPVOID arg 
        )
    {
        m_arg = arg;
    };

    agent_status wait( unsigned int _Timeout = COOPERATIVE_TIMEOUT_INFINITE )
    {
        return agent::wait( this, _Timeout );
    }

protected:
	_Function m_pFunction;
    LPVOID   m_arg;

	void run()
	{
		m_pFunction(m_arg);

		done();
	};
};

