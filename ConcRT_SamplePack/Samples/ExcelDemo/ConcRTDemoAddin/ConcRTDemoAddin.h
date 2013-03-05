/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* ConcRTDemoAddin.h
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MONTECARLO_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MONTECARLO_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.

#ifndef CONCRTDEMOADDIN_API
    #ifdef CONCRTDEMOADDIN_EXPORTS
        #define CONCRTDEMOADDIN_API __declspec(dllexport)
        #define CONCRTDEMOADDIN_EXTERN 
    #else
        #define CONCRTDEMOADDIN_API __declspec(dllimport)
        #define CONCRTDEMOADDIN_EXTERN extern
    #endif
#endif

#pragma once

#include "MonteCarlo.h"
#include "Portfolio.h"