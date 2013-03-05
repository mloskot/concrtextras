/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* MonteCarlo.h
*
* Monte Carlo simulation of bay off value of asset1, and asset2 given
* correlation, annualised vol., and expiration
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "ConcRTDemoAddin.h"

#ifndef _MONTECARLO_H_
#define _MONTECARLO_H_

CONCRTDEMOADDIN_API double __stdcall PayOff_Imp(
	double *average,
	double *stddev,
	DWORD trials = 1000000,
	double asset1 = 4.0, 
	double annualised_vol1 = 0.3, 
	double asset2 = 5.0, 
	double annualised_vol2 = 0.25,
	double correlation = 0.5,
	double expiration = 3.0
	);
CONCRTDEMOADDIN_API double __stdcall PayOff_Parallel_Imp(
	double *average,
	double *stddev,
	DWORD trials = 1000000,
	double asset1 = 4.0, 
	double annualised_vol1 = 0.3, 
	double asset2 = 5.0, 
	double annualised_vol2 = 0.25,
	double correlation = 0.5,
	double expiration = 3.0
	);

#endif // _MONTECARLO_H_