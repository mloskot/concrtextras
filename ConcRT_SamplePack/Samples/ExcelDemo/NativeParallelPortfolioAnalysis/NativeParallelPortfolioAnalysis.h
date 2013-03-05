/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* NativeParallelPortfolioAnalysis.h
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the NATIVEPARALLELPORTFOLIOANALYSIS_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// NATIVEPARALLELPORTFOLIOANALYSIS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef NATIVEPARALLELPORTFOLIOANALYSIS_API
	#ifdef NATIVEPARALLELPORTFOLIOANALYSIS_EXPORTS
		#define NATIVEPARALLELPORTFOLIOANALYSIS_API __declspec(dllexport)
        #define NATIVEPARALLELPORTFOLIOANALYSIS_EXTERN 
	#else
		#define NATIVEPARALLELPORTFOLIOANALYSIS_API __declspec(dllimport)
        #define NATIVEPARALLELPORTFOLIOANALYSIS_EXTERN extern
	#endif
#endif

#include <array>
#include <random>
using namespace std::tr1;

#include <ppl.h>
using namespace Concurrency;

#include "..\..\..\ConcRTExtras\concrt_extras.h"
using namespace Concurrency::samples;

#include "Utility.h"
#include "DifferentialEvolution.h"
#include "PortfolioTracker.h"
