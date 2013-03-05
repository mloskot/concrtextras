/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* MonteCarlo.cpp
*
* Monte Carlo simulation of bay off value of asset1, and asset2 given
* correlation, annualised vol., and expiration
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>

#include <math.h>

#include <random>
using namespace std;
using namespace std::tr1;


#include "MonteCarlo.h"
#include "Commons.h"

double __stdcall PayOff(
	DWORD trials,
	double asset1, 
	double annualised_vol1, 
	double asset2, 
	double annualised_vol2,
	double correlation,
	double expiration
	)
{
	DWORD start = ::GetTickCount();

	double average = 0.0;
	double stddev = 0.0;

	PayOff_Imp(
		&average,
		&stddev,
		trials,
		asset1, 
		annualised_vol1, 
		asset2, 
		annualised_vol2,
		correlation,
		expiration );

	DWORD time = ::GetTickCount() - start;

    #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
	xlSetDouble(7, 1, average);
	xlSetDouble(8, 1, stddev);
	xlSetInt(9, 1, time);
    #endif

	return average;
}

double __stdcall PayOff_Parallel(
	DWORD trials,
	double asset1, 
	double annualised_vol1, 
	double asset2, 
	double annualised_vol2,
	double correlation,
	double expiration
	)
{
	DWORD start = ::GetTickCount();

	double average = 0.0;
	double stddev = 0.0;

	PayOff_Parallel_Imp(
		&average,
		&stddev,
		trials,
		asset1, 
		annualised_vol1, 
		asset2, 
		annualised_vol2,
		correlation,
		expiration);

	DWORD time = ::GetTickCount() - start;

    #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
	xlSetDouble(7, 1, average);
	xlSetDouble(8, 1, stddev);
	xlSetInt(9, 1, time);
    #endif

	return average;
}

#pragma region Implementation 

double __stdcall PayOff_Imp(
	double *average,
	double *stddev,
	DWORD trials, 
	double asset1, 
	double annualised_vol1, 
	double asset2, 
	double annualised_vol2,
	double correlation,
	double expiration
	)
{
	double expire_sqrt = sqrt( expiration );

	double payoff_sum_of_squares = 0.0;
	double payoff_sum = 0.0;

    std::tr1::normal_distribution<double> dist(0, 1); 
    std::tr1::ranlux64_base_01 eng; 

	for (DWORD i = 0 ; i < trials; i++)
	{
		double correlated1 = dist(eng);
		double correlated2 = correlation * correlated1 + sqrt( 1 - correlation * correlation ) * dist(eng);

		double fwd_price1 = asset1 * exp( annualised_vol1 * correlated1 * expire_sqrt );
		double fwd_price2 = asset2 * exp( annualised_vol2 * correlated2 * expire_sqrt );

		double payoff = max( fwd_price1 - fwd_price2, 0.0);

		//
		// update statistics
		//
		payoff_sum_of_squares += ( payoff * payoff );
		payoff_sum += payoff;
	}

	*average = payoff_sum / (double)trials;
    *stddev = sqrt( payoff_sum_of_squares - payoff_sum * payoff_sum / (double)trials) / ((double)trials - 1);

	return *average;
}

double __stdcall PayOff_Parallel_Imp(
	double *average,
	double *stddev,
	DWORD trials,
	double asset1, 
	double annualised_vol1, 
	double asset2, 
	double annualised_vol2,
	double correlation,
	double expiration
	)
{
	double expire_sqrt = sqrt( expiration );

	LONG volatile work_sum = 0L;

	combinable<double> combinable_payoff_sum_of_squares( [](){ return 0.0; } );
	combinable<double> combinable_payoff_sum( [](){ return 0.0; } );
	combinable<std::tr1::normal_distribution<double>> combinable_dist( [](){ return std::tr1::normal_distribution<double>(0, 1); } ); 
	combinable<std::tr1::ranlux64_base_01> combinable_eng;

	SYSTEM_INFO SI;
	GetSystemInfo(&SI);

	const DWORD work_chunk = max( SI.dwNumberOfProcessors, 4 );

	Concurrency::parallel_for<DWORD>( 0, work_chunk, [&](DWORD i)
	{
		DWORD this_loop_trials = trials / work_chunk;
		std::tr1::normal_distribution<double> &dist = combinable_dist.local();
		double &payoff_sum_of_squares = combinable_payoff_sum_of_squares.local();
		double &payoff_sum = combinable_payoff_sum.local();
        std::tr1::ranlux64_base_01 &eng = combinable_eng.local();

		for (DWORD i = 0 ; i < this_loop_trials ; i++)
		{
			double correlated1 = dist(eng);
			double correlated2 = correlation * correlated1 + sqrt( 1 - correlation * correlation ) * dist(eng);

			double fwd_price1 = asset1 * exp( annualised_vol1 * correlated1 * expire_sqrt );
			double fwd_price2 = asset2 * exp( annualised_vol2 * correlated2 * expire_sqrt );

			double payoff = max( fwd_price1 - fwd_price2, 0.0 );

			//
			// update statistics
			//
        		payoff_sum_of_squares += ( payoff * payoff );
        		payoff_sum += payoff;
		}
	});

	double payoff_sum_of_squares = combinable_payoff_sum_of_squares.combine(std::plus<double>());
	double payoff_sum = combinable_payoff_sum.combine(std::plus<double>());

	*average = payoff_sum / (double)trials;
    *stddev = sqrt( payoff_sum_of_squares - payoff_sum * payoff_sum / (double)trials) / ((double)trials - 1);

	return *average;
}

#pragma endregion