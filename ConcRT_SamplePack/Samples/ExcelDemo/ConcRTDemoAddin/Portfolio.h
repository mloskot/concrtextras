/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* Protfolio.h
*
* Contains the Excel add-in part of the Constrained Stock Protfolio determination using Differential Evolution
* This part of the code is directly reachable from Excel, it calls into NativeParallelProtfolioAnalysis.DLL
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "ConcRTDemoAddin.h"
#include <vector>

#include <agents.h>
using namespace Concurrency;

#pragma warning( push )
#pragma warning( disable: 4251 ) 

#ifndef _PORTFOLIO_H_
#define _PORTFOLIO_H_

#include "..\NativeParallelPortfolioAnalysis\PortfolioTracker.h"

/// start executing the protfolio simulation
CONCRTDEMOADDIN_API void __stdcall StockPortfolio_Imp(
    __inout PortfolioTrackerProgressEventArgs &retVal,
    __in void * pSPP,
    __in const DATE start,
    __in const DATE end,
    __in const bool parallel = true,
    __in const __int32 maxContributors = 125, 
    __in const __int32 numIterations = 400, 
    __in const __int32 numPortfolios = 200, 
    __in const double mutationFactor = 0.3, 
    __in const double crossoverRate = 0.8, 
    __in const double swapProbability = 0.1,
    __in __int32 maxConcurrency = -1
	);

CONCRTDEMOADDIN_API
void *
ReadData(
    __in LPCSTR dataFolder,
    __in DATE start,
    __in DATE end,
    __in const bool parallel = true
    );


class InstanceData
{
public:
    static InstanceData* NewInstance( 
        __in const LPCSTR dataFolder,
        __in const DATE start,
        __in const DATE end,
        __in const bool parallel = true,
        __in const __int32 row = 20,
        __in const __int32 col = 1,
        __in const __int32 data_row = 20,
        __in const __int32 data_col = 5,
        __in const __int32 maxContributors = 125, 
        __in const __int32 numIterations = 400, 
        __in const __int32 numPortfolios = 200, 
        __in const double mutationFactor = 0.3, 
        __in const double crossoverRate = 0.8, 
        __in const double swapProbability = 0.1,
        __in __int32 maxConcurrency = -1
        );

    static InstanceData* NewInstance( 
        __in void * pSPP,
        __in const DATE start,
        __in const DATE end,
        __in const bool parallel = true,
        __in const __int32 row = 20,
        __in const __int32 col = 1,
        __in const __int32 data_row = 20,
        __in const __int32 data_col = 5,
        __in const __int32 maxContributors = 125, 
        __in const __int32 numIterations = 400, 
        __in const __int32 numPortfolios = 200, 
        __in const double mutationFactor = 0.3, 
        __in const double crossoverRate = 0.8, 
        __in const double swapProbability = 0.1,
        __in __int32 maxConcurrency = -1
        );

    virtual ~InstanceData();

    //
    // given a handler, this function will query the latest results and
    // write them to Excel staring from cell (row, col), and will report best individual
    // history tracking starting from cell (data_row, data_col)
    //
    static void UpdateExcel( DWORD id );

    //
    // cancel search for solution
    //
    static void Cancel( DWORD id );

    //
    // delete intermediate data used for search
    //
    static void Delete( DWORD id );

    //
    // check if this search is completed
    //
    static BOOL IsDone( DWORD id );

    //
    // report calcuations time in ms
    //
    static DWORD GetCalcTime( DWORD id );

    //
    // report file read time in ms
    //
    static DWORD GetFileReadTime( DWORD id );

    //---------------------------------------------------------------------------------------------
    // instance properties
    //---------------------------------------------------------------------------------------------

    PortfolioTracker * GetTracker() { return m_tracker; };

    DWORD GetId() { return m_id; };

    void wait( 
        unsigned __int32 _Timeout = COOPERATIVE_TIMEOUT_INFINITE 
        );

private:
    InstanceData(
        __in void * pSPP,
        __in const DATE start,
        __in const DATE end,
        __in const bool parallel = true,
        __in const __int32 row = 20,
        __in const __int32 col = 1,
        __in const __int32 data_row = 20,
        __in const __int32 data_col = 5,
        __in const __int32 maxContributors = 125, 
        __in const __int32 numIterations = 400, 
        __in const __int32 numPortfolios = 200, 
        __in const double mutationFactor = 0.3, 
        __in const double crossoverRate = 0.8, 
        __in const double swapProbability = 0.1,
        __in __int32 maxConcurrency = -1
        );

    /// Uniqe ID for that instance, this is the handle returned by the addin function
    DWORD m_id;

    /// The data read from files
    SymbolPricesPackage *m_data;

    /// tracker used to search for best portfolio
    PortfolioTracker *m_tracker;

    /// a vector to store all instances, this is how the add in
    /// tracks executing agents, and communicate with Excel through 
    /// the instance id, which is the index to this vector
    static concurrent_vector< InstanceData* > s_AllInstances;

    /// search s_AllInstances for tracker, and returns the ID
    static InstanceData * FindInstanceByTracker( PortfolioTracker* tracker );

    DWORD m_fileReadTime;
    DWORD m_portfolioTime;


    //////////////////////////////////////////////////////////////////////////////
    /////
    //      Results and display
    //

    /// unbounded buffer for parallel version
	unbounded_buffer<PortfolioTrackerProgressEventArgs *> *m_latestResultQueue;

    /// vector of all the results found in serial version
    static void InstanceDataTrackerCallback( PortfolioTracker* tracker );
    concurrent_vector< PortfolioTrackerProgressEventArgs > m_results;
    reader_writer_lock m_results_lock;
    size_t m_lastReportedIndex;

    // row and col for diversity and fitness
    const DWORD m_row;
    const DWORD m_col;

    // row and col for best fit tracking history
    const DWORD m_data_row;
    const DWORD m_data_col;

    void AppendExcelResult();

    // worker agent for this instance
    CustomAgent< TaskProc > *m_trackerAgent;

    static void StartTracker( LPVOID instance );
};

#endif // _PORTFOLIO_H_

#pragma warning( pop )
