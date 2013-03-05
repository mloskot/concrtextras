/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* Protfolio.cpp
*
* Contains the Excel add-in part of the Constrained Stock Protfolio determination using Differential Evolution
* This part of the code is directly reachable from Excel, it calls into NativeParallelProtfolioAnalysis.DLL
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>

#include <math.h>

#include <ppl.h>
using namespace Concurrency;

#include <random>
using namespace std;
using namespace std::tr1;


#include "portfolio.h"

#include "..\NativeParallelPortfolioAnalysis\PortfolioTracker.h"

#include "Commons.h"

/////////////////////////////////////////////////////////////////////////////////
/////
//
//  Common functions
//

/// Convert date string of formate MM/dd/yyyy to DATE
DATE
ParseDate(
    __in LPSTR lpFormatedDate
    )
{
    const CHAR tokens[] = "\\/.";
    SYSTEMTIME st = { 0 };
    CHAR * nextToken = NULL;
    CHAR * stringPointer = lpFormatedDate;
    stringPointer = strtok_s( stringPointer, tokens, &nextToken );
    
    if( stringPointer != NULL )
    {
        st.wMonth = atoi( stringPointer );
    }
    
    stringPointer = strtok_s( NULL, tokens, &nextToken );
    if( stringPointer != NULL )
    {
        st.wDay = atoi( stringPointer );
    }
    
    stringPointer = strtok_s( NULL, tokens, &nextToken );
    if( stringPointer != NULL )
    {
        st.wYear = atoi( stringPointer );
    }
    
    DATE retVal = 0.0;
    SystemTimeToVariantTime( &st, &retVal );
    return retVal;
}

LPVOID
ReadFile(
    __in LPCTSTR lpFileName
    )
{
    // CreateFile, FILE_FLAG_OVERLAPPED 
    HANDLE hFile = CreateFile(
        lpFileName,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        0,
        NULL );
        
    BYTE *pBuffer = NULL;
        
    if( hFile != INVALID_HANDLE_VALUE )
    {
        DWORD fileSize = GetFileSize( hFile, NULL ), readSize = 0;
        assert( fileSize != 0 );
        pBuffer = ( BYTE * ) malloc( fileSize + 1 );
        
        ReadFile(
            hFile,
            ( LPVOID ) pBuffer,
            fileSize,
            &readSize,
            NULL );

        BOOL Failed = ( readSize != fileSize );
        Failed |= ( readSize == 0 );
            
        if( Failed )
        {
            // failed
            free( pBuffer );
            pBuffer = NULL;

            throw std::exception( "read size error" );
        }
        else
        {
            pBuffer[ readSize ] = '\0';
        }
        
        ::CloseHandle( hFile );
    }
    
    return pBuffer;
}

/// Read the close price from a CSV index file.
void
ReadIndexFile(
    __in SymbolPrices &symbolPrices,
    __in LPCSTR dataFolder,
    __in DATE start,
    __in DATE end
    )
{
    // build the symbol data file name
    CHAR lpmbFileName[_MAX_PATH];
    _makepath_s(
        lpmbFileName, // generated path
        NULL,       // drive
        dataFolder, // folder
        symbolPrices.m_Name, // filename
        "csv" );      // extension ( .CSV )

#ifdef _UNICODE
    size_t converted = 0;
    TCHAR lpFileName[_MAX_PATH] = _T( "" );
    mbstowcs_s( &converted, lpFileName, lpmbFileName, _MAX_PATH );
#else
    TCHAR * lpFileName = lpmbFileName;
#endif // _UNICODE
    
    CHAR *pBuffer = ( CHAR * ) ReadFile( lpFileName );
    CHAR *filePointer = pBuffer;
    const CHAR tokens[] = ";,\r\n\t";
    CHAR * nextToken = NULL;
    
    if( pBuffer != NULL )
    {
        // read stock close prices and filter them according to start, and end
        // 
        // DATE,OPEN,HIGH,LOW,CLOSE,VOLUME
        // 9/30/2008,29.39,30.1,28.73,29.66,4151200
        // 9/29/2008,30.37,30.59,28.64,29.07,5177100
        // 9/26/2008,30.78,30.99,30.14,30.93,4511400
        //
        filePointer = strstr( pBuffer, "VOLUME\r\n" );
        filePointer = strtok_s( filePointer, tokens, &nextToken );
        filePointer = strtok_s( NULL, tokens, &nextToken ); // ignore the headers by putting a null just after it
        while( filePointer != NULL )
        {
            DATE thisDay = ParseDate( filePointer );

            filePointer = strtok_s( NULL, tokens, &nextToken ); // skip open
            filePointer = strtok_s( NULL, tokens, &nextToken ); // skip high
            filePointer = strtok_s( NULL, tokens, &nextToken ); // skip low

            filePointer = strtok_s( NULL, tokens, &nextToken ); // close
            
            if( ( thisDay >= start ) && ( thisDay <= end ) ) // difftime
            {
                symbolPrices.m_Prices.push_back( strtod( filePointer, NULL ) );
            }

            filePointer = strtok_s( NULL, tokens, &nextToken ); // volume
            filePointer = strtok_s( NULL, tokens, &nextToken ); // Next
        }
        

        free( pBuffer );
    }
}

/// Read the weights, and close prices of all seleceted symbols.
void
ReadWeightFile(
    __in SymbolPricesPackage &symbolPricesPackage,
    __in LPCSTR dataFolder,
    __in DATE start,
    __in DATE end,
    __in const bool parallel = true
    )
{
    // build the symbol data file name
    CHAR lpmbFileName[_MAX_PATH];
    _makepath_s(
        lpmbFileName, // generated path
        NULL,       // drive
        dataFolder, // folder
        "weights", // filename
        "csv" );   // extension ( .CSV )
#ifdef _UNICODE
    size_t converted = 0;
    TCHAR lpFileName[_MAX_PATH] = _T( "" );
    mbstowcs_s( &converted, lpFileName, lpmbFileName, _MAX_PATH );
#else
    TCHAR * lpFileName = lpmbFileName;
#endif // _UNICODE

    CHAR *pBuffer = ( CHAR * ) ReadFile( lpFileName );
    CHAR *filePointer = pBuffer;
    const CHAR tokens[] = ";,\r\n\t";
    CHAR * nextToken = NULL;
    
    if( pBuffer != NULL )
    {
        // read stock close prices and filter them according to start, and end
        // 
        // SYMBOL,    WEIGHTPCT
        // SYM1,    0.0925838825414557
        // SYM2,    0.10264376245634
        // SYM2,    1.04909841422837
        //
        filePointer = strstr( pBuffer, "WEIGHTPCT\r\n" );
        filePointer = strtok_s( filePointer, tokens, &nextToken );

        filePointer = strtok_s( NULL, tokens, &nextToken ); // ignore the headers by putting a null just after it

		int symbolIndex = 0;

        while( filePointer != NULL )
        {
            SymbolPrices thisSymbol;
            
            strcpy_s< MAX_SYMBOL_NAME >( thisSymbol.m_Name, filePointer );

            filePointer = strtok_s( NULL, tokens, &nextToken ); // weight
            thisSymbol.m_Weight = strtod( filePointer, NULL );

            symbolPricesPackage.m_Members.push_back( thisSymbol );
                
            filePointer = strtok_s( NULL, tokens, &nextToken ); // Next
        }

        free( pBuffer );
        
        if( parallel )
        {
            parallel_for_each( symbolPricesPackage.m_Members.begin(), symbolPricesPackage.m_Members.end(), [=]( SymbolPrices &thisSymbol)
            {
                ReadIndexFile(
                    thisSymbol,
                    dataFolder,
                    start,
                    end );
            });
        }
        else
        {
            for_each( symbolPricesPackage.m_Members.begin(), symbolPricesPackage.m_Members.end(), [=]( SymbolPrices &thisSymbol)
            {
                ReadIndexFile(
                    thisSymbol,
                    dataFolder,
                    start,
                    end );
            });

        }
    }
}

/// reads index symbols, prices, and weights, then returns this data packaged in SymbolPricesPackage
void *
ReadData(
    __in LPCSTR dataFolder,
    __in DATE start,
    __in DATE end,
    __in const bool parallel
    )
{
    SymbolPricesPackage *retVal = new SymbolPricesPackage();

    // read the S&P 500 INDEX (INX)
    strcpy_s< MAX_SYMBOL_NAME >( retVal->m_Index.m_Name, "INX" );
    retVal->m_Index.m_Weight = 1.0; // set to 0 for now caller may set the correct values
    ReadIndexFile( 
        retVal->m_Index, 
        dataFolder, 
        start, 
        end );

    // read the weights.csv, and load symbol prices too
    ReadWeightFile(
        *retVal,
        dataFolder,
        start,
        end,
        parallel );
    
    // remove partial data
    if ( retVal->m_Index.m_Prices.size() == 0 )
    {
        delete retVal;
        retVal = NULL;
    }
    else
    {
        for( 
            MyVector( SymbolPrices )::iterator i = retVal->m_Members.begin(); 
            i != retVal->m_Members.end(); 
            ++i )
        {
            if( i->m_Prices.size() < retVal->m_Index.m_Prices.size() )
            {
                //retVal->m_Index.m_Prices.resize( i->m_Prices.size() ); // shrink the index to the available size
                i->m_Prices.resize( retVal->m_Index.m_Prices.size(), 0.0 );
            }
        }
    }
    
    return (void*) retVal;
}

/////////////////////////////////////////////////////////////////////////////////
/////
//
//  InstanceData implementation
//
InstanceData *
InstanceData::
NewInstance(
    __in const LPCSTR dataFolder,
    __in const DATE start,
    __in const DATE end,
    __in const bool parallel,
    __in const __int32 row,
    __in const __int32 col,
    __in const __int32 data_row,
    __in const __int32 data_col,
    __in const __int32 maxContributors, 
    __in const __int32 numIterations, 
    __in const __int32 numPortfolios, 
    __in const double mutationFactor, 
    __in const double crossoverRate, 
    __in const double swapProbability,
    __in __int32 maxConcurrency
    )
{
    DWORD test_start = ::GetTickCount();

    SymbolPricesPackage * pSPP = (SymbolPricesPackage *) ReadData(
        dataFolder,
        start,
        end,
        parallel );
        
	DWORD time = ::GetTickCount() - test_start;

    InstanceData * retVal = NULL;

    if( pSPP != NULL)
    {
        retVal = NewInstance(
            pSPP,
            start,
            end,
            parallel,
            row,
            col,
            data_row,
            data_col,
            maxContributors, 
            numIterations, 
            numPortfolios, 
            mutationFactor, 
            crossoverRate, 
            swapProbability,
            maxConcurrency );

        retVal->m_fileReadTime = time;
    }

    return retVal;
}

InstanceData *
InstanceData::
NewInstance(
    __in void * pSPP,
    __in const DATE start,
    __in const DATE end,
    __in const bool parallel,
    __in const __int32 row,
    __in const __int32 col,
    __in const __int32 data_row,
    __in const __int32 data_col,
    __in const __int32 maxContributors, 
    __in const __int32 numIterations, 
    __in const __int32 numPortfolios, 
    __in const double mutationFactor, 
    __in const double crossoverRate, 
    __in const double swapProbability,
    __in __int32 maxConcurrency
    )
{
    InstanceData * retVal = new InstanceData(
        pSPP,
        start,
        end,
        parallel,
        row,
        col,
        data_row,
        data_col,
        maxContributors, 
        numIterations, 
        numPortfolios, 
        mutationFactor, 
        crossoverRate, 
        swapProbability,
        maxConcurrency );

    //
    // save the ptrdiff_t as the id for that agent
    // this id is how Excel identify this session
    //
    retVal->m_id = (DWORD)(s_AllInstances.push_back( retVal ) - s_AllInstances.begin());

    return retVal;
}

InstanceData::
InstanceData(
    __in void * pSPP,
    __in const DATE start,
    __in const DATE end,
    __in const bool parallel,
    __in const __int32 row,
    __in const __int32 col,
    __in const __int32 data_row,
    __in const __int32 data_col,
    __in const __int32 maxContributors, 
    __in const __int32 numIterations, 
    __in const __int32 numPortfolios, 
    __in const double mutationFactor, 
    __in const double crossoverRate, 
    __in const double swapProbability,
    __in __int32 maxConcurrency
    ) : m_tracker( NULL ),
        m_id( 0 ),
        m_fileReadTime( 0 ),
        m_portfolioTime( 0 ),
        m_data( reinterpret_cast< SymbolPricesPackage * > ( pSPP ) ),
        m_lastReportedIndex( 0 ),
        m_row( row ),
        m_col( col ),
        m_data_row( data_row ),
        m_data_col( data_col ),
        m_trackerAgent( NULL ),
		m_latestResultQueue( NULL )
{
    if( maxConcurrency == -1 )
    {
        SYSTEM_INFO si = { 0 };
        GetSystemInfo( &si );
        maxConcurrency = si.dwNumberOfProcessors;
    }
    
	if (parallel)
	{
		m_latestResultQueue = new unbounded_buffer<PortfolioTrackerProgressEventArgs *>();
	    m_tracker = new PortfolioTracker(
	        *m_data,
	        maxContributors,
	        numIterations,
	        numPortfolios,
	        mutationFactor,
	        crossoverRate,
	        swapProbability,
	        parallel,
	        maxConcurrency,
			m_latestResultQueue );
	}
	else
	{
	    m_tracker = new PortfolioTracker(
	        *m_data,
	        maxContributors,
	        numIterations,
	        numPortfolios,
	        mutationFactor,
	        crossoverRate,
	        swapProbability,
	        parallel,
	        maxConcurrency,
			InstanceData::InstanceDataTrackerCallback );
	}

    m_trackerAgent = new CustomAgent< TaskProc >( InstanceData::StartTracker, this );
    m_trackerAgent->start();
}

InstanceData::
~InstanceData()
{
    delete m_data;
    delete m_tracker;

    m_trackerAgent->cancel();
    m_trackerAgent->wait();
    delete m_trackerAgent;
}

void 
InstanceData::
UpdateExcel( DWORD id )
{
    s_AllInstances[ id ]->AppendExcelResult();
}

void 
InstanceData::
Cancel( DWORD id )
{
    s_AllInstances[ id ]->m_tracker->Cancel();
    s_AllInstances[ id ]->m_trackerAgent->cancel();
}

BOOL
InstanceData::
IsDone( DWORD id )
{
    return ( s_AllInstances[ id ]->m_trackerAgent->status() == agent_done );
}

void
InstanceData::
Delete( DWORD id )
{
    Cancel( id );
    InstanceData * instance = s_AllInstances[ id ];

    // make InstanceData at ( id ) unuseable
    s_AllInstances[ id ] = NULL;

    delete instance;
}

DWORD 
InstanceData::
GetCalcTime( 
    DWORD id 
    )
{
    return s_AllInstances[ id ]->m_portfolioTime;
}

DWORD 
InstanceData::
GetFileReadTime( 
    DWORD id 
    )
{
    return s_AllInstances[ id ]->m_fileReadTime;
}

InstanceData * 
InstanceData::
FindInstanceByTracker( PortfolioTracker* tracker )
{
    InstanceData * instance = NULL;
    for( 
        concurrent_vector< InstanceData* >::const_iterator i = s_AllInstances.begin() ; 
        i != s_AllInstances.end() ; 
        ++i )
    {
        if( ( *i ) != NULL)
        {
            if( ( *i )->m_tracker == tracker)
            {
                instance = *i;
                break;
            }
        }
    }
    
    return instance;
}

//////////////////////////////////////////////////////////////////////////////
/////
//      Results and display
//


void 
InstanceData::
AppendExcelResult()
{
	bool update_results = false;

    if( m_lastReportedIndex == 0 )
    {
        //
        // Display the Index Data
        //

        DWORD startRow = m_data_row;
        
        for_each( m_data->m_Index.m_Prices.begin(), m_data->m_Index.m_Prices.end(), [&]( double &retVal )
        {
            #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
            xlSetDouble( (short)startRow, (short)m_data_col + 0, retVal );
            #else
            #ifdef _DEBUG
            printf( "IndexPrices = %f \r\n", retVal );
            #endif // _DEBUG
            #endif // CONCRTDEMOADDIN_EXPORTS

            startRow++;
        } );
    }

	PortfolioTrackerProgressEventArgs *lastResult = NULL;

	if( m_tracker->AsParallel() )
	{
		if( m_latestResultQueue )
		{
			PortfolioTrackerProgressEventArgs *retVal = NULL;
			while( 
				try_receive< PortfolioTrackerProgressEventArgs * >( 
					m_latestResultQueue, 
					retVal ) )
			{
				if( !update_results )
				{
					update_results = true;
				}

				if( lastResult!= NULL )
				{
					Free( lastResult );
				}
	            
				DWORD startRow = m_row + retVal->m_Generation;

	            #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
	            xlSetInt(    (short)startRow, (short)m_col + 0, retVal->m_Generation );
	            xlSetDouble( (short)startRow, (short)m_col + 1, retVal->m_Population.m_Diversity );
	            xlSetDouble( (short)startRow, (short)m_col + 2, retVal->m_Population.m_Individuals[ retVal->m_Population.m_Optimum ].m_Fitness );
	            #else
	            #ifdef _DEBUG
	            printf( 
	                "Generation = %d,\tDiversity = %f,\tFitness = %f \r\n", 
	                retVal->m_Generation,
	                retVal->m_Population.m_Diversity,
	                retVal->m_Population.m_Individuals[ retVal->m_Population.m_Optimum ].m_Fitness );
	            #endif // _DEBUG
	            #endif // CONCRTDEMOADDIN_EXPORTS
	            
	            ++m_lastReportedIndex;

				lastResult = retVal;
			}
		}
	}
	else
	{
		if( m_lastReportedIndex < (size_t)m_tracker->NumIterations()) // last element ASSERT( instance->m_results.size() == _tracker->NumIterations() )
	    {
	        // starting from _lastReportedIndex we will loop until we reach the end of _results

	        m_results_lock.lock_read();
	        for_each( m_results.begin() + m_lastReportedIndex, m_results.end(), [&]( PortfolioTrackerProgressEventArgs &retVal )
	        {
				if( !update_results )
				{
					update_results = true;
				}

				DWORD startRow = m_row + retVal.m_Generation;

	            #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
	            xlSetInt(    (short)startRow, (short)m_col + 0, retVal.m_Generation );
	            xlSetDouble( (short)startRow, (short)m_col + 1, retVal.m_Population.m_Diversity );
	            xlSetDouble( (short)startRow, (short)m_col + 2, retVal.m_Population.m_Individuals[ retVal.m_Population.m_Optimum ].m_Fitness );
	            #else
	            #ifdef _DEBUG
	            printf( 
	                "Generation = %d,\tDiversity = %f,\tFitness = %f \r\n", 
	                retVal.m_Generation,
	                retVal.m_Population.m_Diversity,
	                retVal.m_Population.m_Individuals[ retVal.m_Population.m_Optimum ].m_Fitness );
	            #endif // _DEBUG
	            #endif // CONCRTDEMOADDIN_EXPORTS
	            
	            ++m_lastReportedIndex;
	        });

            if(m_results.size() > 0)
            {
			    lastResult = &(m_results.back());
            }

	        m_results_lock.unlock();
	    }
	}

	if( update_results )
	{
        //
        // Display the Index Data
        //
        MyVector( double ) &MyPrices = lastResult->m_PopulationBest.m_Prices;
		MyVector( double ) &Weights = lastResult->m_Population.m_Individuals[ lastResult->m_Population.m_Optimum ].m_Chromosomes;
		
        DWORD startRow = m_data_row;

        //
        // This critical section to protect Excel, as the UI is not thread safe
        //
        critical_section cs_excel_ui;
		
		// Excel only update cells if call is from the main thread
		if( m_tracker->AsParallel() )
		{
		parallel_invoke(
			[&]()
			{
				for( size_t i = 0 ; i < MyPrices.size() ; i++ )
		        {
                    cs_excel_ui.lock();

		            #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
		            xlSetDouble( (short)(m_data_row + i), (short)(m_data_col + 1), MyPrices[ i ] );
		            #else
		            #ifdef _DEBUG
		            printf( "Pest Prices = %f \r\n", MyPrices[ i ] );
		            #endif // _DEBUG
		            #endif // CONCRTDEMOADDIN_EXPORTS

                    cs_excel_ui.unlock();
				}
			},
			[&]()
			{
				if ( m_lastReportedIndex >= (size_t)m_tracker->NumIterations() ) // false ) // 
				{
					for( size_t i = 0 ; i < Weights.size() ; i++ )
					{
                        cs_excel_ui.lock();
                        #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
			            xlSetDouble( (short)(m_data_row + i), (short)(m_data_col + 6), Weights[ i ] );
			            #else
			            #ifdef _DEBUG
			            printf( "Weights = %f \r\n", Weights[ i ] );
			            #endif // _DEBUG
			            #endif // CONCRTDEMOADDIN_EXPORTS
                        cs_excel_ui.unlock();
					}
				}
			} );
			Free( lastResult );
		}
		else
		{
			for( size_t i = 0 ; i < MyPrices.size() ; i++ )
	        {
	            #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
	            xlSetDouble( (short)(m_data_row + i), (short)(m_data_col + 1), MyPrices[ i ] );
	            #else
	            #ifdef _DEBUG
	            printf( "Pest Prices = %f \r\n", MyPrices[ i ] );
	            #endif // _DEBUG
	            #endif // CONCRTDEMOADDIN_EXPORTS
			}
			if ( m_lastReportedIndex >= (size_t)m_tracker->NumIterations() ) // false ) // 
			{
				for( size_t i = 0 ; i < Weights.size() ; i++ )
				{
		            #ifdef CONCRTDEMOADDIN_EXPORTS // enable only when build for Excel addin
		            xlSetDouble( (short)(m_data_row + i), (short)(m_data_col + 6), Weights[ i ] );
		            #else
		            #ifdef _DEBUG
		            printf( "Weights = %f \r\n", Weights[ i ] );
		            #endif // _DEBUG
		            #endif // CONCRTDEMOADDIN_EXPORTS
				}
			}
		}
	}
}

void 
InstanceData::
InstanceDataTrackerCallback( PortfolioTracker* tracker )
{
    // find index and append it to results
    InstanceData * instance = FindInstanceByTracker( tracker );

	if( !instance->m_tracker->AsParallel() )
	{
	    PortfolioTrackerProgressEventArgs retVal;
	    tracker->GetLatestResult( retVal );

	    instance->m_results_lock.lock();

	    instance->m_results.push_back( retVal );
	    
	    instance->m_results_lock.unlock();
	}
}

void 
InstanceData::
StartTracker( LPVOID pvIntance )
{
    InstanceData* instance = reinterpret_cast< InstanceData* >( pvIntance );

    DWORD test_start = ::GetTickCount();
        
    instance->m_tracker->Track();

    instance->m_portfolioTime = ::GetTickCount() - test_start;
}

void 
InstanceData::
wait(
    unsigned __int32 timeout
    ) 
{ 
    m_trackerAgent->wait( timeout ); 
};

concurrent_vector< InstanceData* > InstanceData::s_AllInstances;

/////////////////////////////////////////////////////////////////////////////////
/////
//
//  Portfolio wrappers
//

/// For unit testing
void __stdcall StockPortfolio_Imp(
    __inout PortfolioTrackerProgressEventArgs &retVal,
    __in void * pvSPP,
    __in const DATE start,
    __in const DATE end,
    __in const bool parallel,
    __in const __int32 maxContributors, 
    __in const __int32 numIterations, 
    __in const __int32 numPortfolios, 
    __in const double mutationFactor, 
    __in const double crossoverRate, 
    __in const double swapProbability,
    __in __int32 maxConcurrency
    )
{
    InstanceData *instance = InstanceData::NewInstance(
        pvSPP,
        start,
        end,
        parallel,
        0,
        0,
        0,
        0,
        maxContributors, 
        numIterations, 
        numPortfolios, 
        mutationFactor, 
        crossoverRate, 
        swapProbability,
        maxConcurrency );
    
    instance->wait();
    instance->GetTracker()->GetLatestResult( retVal );
    InstanceData::UpdateExcel( instance->GetId() );
    InstanceData::Delete( instance->GetId() );
}

LONG __stdcall StockPortfolio(
    __in const LPCSTR dataFolder,
    __in const DATE start,
    __in const DATE end,
    __in const __int32 row,
    __in const __int32 col,
    __in const __int32 data_row,
    __in const __int32 data_col,
    __in const __int32 maxContributors, 
    __in const __int32 numIterations, 
    __in const __int32 numPortfolios, 
    __in const double mutationFactor, 
    __in const double crossoverRate, 
    __in const double swapProbability
    )
{
    InstanceData *instance = InstanceData::NewInstance(
        dataFolder,
        start,
        end,
        false,
        row,
        col,
        data_row,
        data_col,
        maxContributors, 
        numIterations, 
        numPortfolios, 
        mutationFactor, 
        crossoverRate, 
        swapProbability,
        -1 );

    return instance->GetId();
}

LONG __stdcall StockPortfolio_Parallel(
    __in const LPCSTR dataFolder,
    __in const DATE start,
    __in const DATE end,
    __in const __int32 row,
    __in const __int32 col,
    __in const __int32 data_row,
    __in const __int32 data_col,
    __in const __int32 maxContributors, 
    __in const __int32 numIterations, 
    __in const __int32 numPortfolios, 
    __in const double mutationFactor, 
    __in const double crossoverRate, 
    __in const double swapProbability,
    __in __int32 maxConcurrency
    )
{
    InstanceData *instance = InstanceData::NewInstance(
        dataFolder,
        start,
        end,
        true,
        row,
        col,
        data_row,
        data_col,
        maxContributors, 
        numIterations, 
        numPortfolios, 
        mutationFactor, 
        crossoverRate, 
        swapProbability,
        maxConcurrency );

    return instance->GetId();
}

BOOL
StockPortfolio_Update(
    LONG id
    )
{
    InstanceData::UpdateExcel( id );
    return TRUE;
}

BOOL
StockPortfolio_Delete(
    LONG id
    )
{
    InstanceData::Delete( id );
    return TRUE;
}

BOOL
StockPortfolio_Cancel(
    LONG id
    )
{
    InstanceData::Cancel( id );
    return TRUE;
}


BOOL
StockPortfolio_IsDone(
    LONG id
    )
{
    return InstanceData::IsDone( id );
}

LONG
StockPortfolio_GetCalcTime(
    LONG id
    )
{
    return InstanceData::GetCalcTime( id );
}

LONG
StockPortfolio_GetFileReadTime(
    LONG id
    )
{
    return InstanceData::GetFileReadTime( id );
}

