//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: parallel_any_of.cpp
//
//
//--------------------------------------------------------------------------
#include "..\..\concrtextras\ppl_extras.h"
#include "..\..\concrtextras\concrt_extras.h"

#include <ppl.h>
#include <concrt.h>
#include <concrtrm.h>
#include <vector>
#include <windows.h>
#include <algorithm>
#include <iostream>
#include <concurrent_vector.h>
#include <numeric>

using namespace ::std;
using namespace ::Concurrency;
using namespace ::Concurrency::samples;
template <class Func>
int time_call(Func&& f)
{
    int begin = GetTickCount();
    f();
    return GetTickCount() - begin;
}
bool is_carmichael( int n) {
    if (n < 2) { return false; }

    int k = n;

    for (int i = 2; i <= k / i; ++i) {
        if (k % i == 0) {
            if ((k / i) % i == 0) { return false; }
            if ((n - 1) % (i - 1) != 0) { return false; }
            k /= i;
            i = 1;
        }
    }

    return k != n && (n - 1) % (k - 1) == 0;
}
int main()
{
    //enable tracing for the profiler
    Concurrency::EnableTracing();

    int numItems = 2500000;
    int numProcs = GetProcessorCount();

    concurrent_vector<vector<int>*> vectors;
    event e;
    parallel_for(0,numProcs,1,[&](int i)
    {
        vector<int>* v = new vector<int>();
        for(int j = numItems/numProcs*i; j < numItems/numProcs * (i + 1); ++j)
            v->push_back(j);
        vectors.push_back(v);
    });
    cout << "counting the carmichael numbers" << endl;
    //execute in serial
    vector<int> sums(numProcs);
    int sum;

    int serialTime = time_call([&](){
        transform(vectors.begin(),vectors.end(),sums.begin(),[](vector<int>* v)->int
        {   
            return count_if(v->begin(),v->end(),is_carmichael);
        });

        sum = accumulate(sums.begin(),sums.end(),0);
    });
    cout << serialTime << " ms, num carmichaels " << sum << endl;

    //execute the parallel version on a lightweight task
    schedule_task([&](){
        sum = 0;
        int parallelTime = time_call([&](){
            samples::parallel_transform(vectors.begin(),vectors.end(),sums.begin(),[](vector<int>* v)->int
            {   
                return count_if(v->begin(),v->end(),is_carmichael);
            });

            sum = samples::parallel_reduce(sums.begin(),sums.end(),0);
        });
        cout << parallelTime << " ms, num carmichaels " << sum << endl;

        sum = 0;
        int parallelTime2 = time_call([&](){
            samples::parallel_transform(vectors.begin(),vectors.end(),sums.begin(),[](vector<int>* v)->int
            {   
                return parallel_count_if(v->begin(),v->end(),is_carmichael);
            });

            sum = samples::parallel_reduce(sums.begin(),sums.end(),0);
        });
        cout << parallelTime2 << " ms, num carmichaels " << sum << endl;
        e.set();
    });
    e.wait();
    return 0;
}

