/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* PortfolioTracker.h
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

#ifndef _PORTFOLIO_TRACKER_H_
#define _PORTFOLIO_TRACKER_H_

#define MAX_SYMBOL_NAME 6

//-------------------------------------------------------------------------------------------------
// Position
//------------------------------------------------------------------------------------------------

struct Position
{
    CHAR   m_Symbol[ MAX_SYMBOL_NAME ];
    double m_Weight;
};

//-------------------------------------------------------------------------------------------------
// Portfolio
//------------------------------------------------------------------------------------------------

struct Portfolio 
{
    MyVector( double )    m_ValueHistory;
    MyVector( double )    m_DiffOfLogReturns;
    double                m_TrackingError;
};

//-------------------------------------------------------------------------------------------------
// PortfolioHistory
//------------------------------------------------------------------------------------------------

/// Represents the performance history of a portfolio. 
struct PortfolioHistory 
{ 
    NATIVEPARALLELPORTFOLIOANALYSIS_API
    PortfolioHistory();

	PortfolioHistory(
		__in MyVector( double ) &prices 
		);

    /// Gets the history of prices for a portfolio.
    MyVector( double ) m_Prices; 
    
    /// Gets the history of log returns (that is, log(s[n+1] / s[n])) for a portfolio.
    MyVector( double ) m_LogReturns;
};

//-------------------------------------------------------------------------------------------------
// SymbolWeight
//------------------------------------------------------------------------------------------------

/// Represents the name of an instrument and its weight within an index.
struct SymbolWeight 
{
    CHAR   m_Name[ MAX_SYMBOL_NAME ];
    double m_Weight;
};

//-------------------------------------------------------------------------------------------------
// SymbolPrices
//------------------------------------------------------------------------------------------------

/// Represents the name of an instrument, its weights within an index, and its price history.
struct SymbolPrices 
{
    CHAR   m_Name[ MAX_SYMBOL_NAME ];
    double m_Weight;
    MyVector( double ) m_Prices; 
};

//-------------------------------------------------------------------------------------------------
// SymbolPricesPackage
//------------------------------------------------------------------------------------------------

/// Represents the package of index and index member data used for regression.
struct SymbolPricesPackage
{
    SymbolPrices             m_Index;
    MyVector( SymbolPrices ) m_Members;
};

//-------------------------------------------------------------------------------------------------
// PortfolioTrackerProgressEventArgs
//------------------------------------------------------------------------------------------------

/// Represents the results of a population generation. this is needed for the call back event to update the status
struct PortfolioTrackerProgressEventArgs
{ 
    /// Gets the one-based generation number.
    int m_Generation;
    
    /// Gets the current population.
    Population m_Population;
    
    /// Gets the PortfolioHistory of Population.Optimum.
    PortfolioHistory m_PopulationBest;
};

//-------------------------------------------------------------------------------------------------
// PortfolioTracker
//------------------------------------------------------------------------------------------------

// this call back will be called every generation
class PortfolioTracker;
typedef void(*PortfolioTrackerCallback)( PortfolioTracker* thisInstance );

/// Represents the methodology for searching for an optimal tracking portfolio for an index under
/// the provided constraints. An index is a weighted portfolio of instruments. A tracking 
/// portfolio is a portfolio based on the index where only a subset of all the instruments have
/// non-zero weights. The weights are manipulated in the tracking portfolio to model the 
/// performance of the index as closely as possible.
class PortfolioTracker
{
private:
    SymbolPricesPackage &m_data;
    int                 m_maxContributors;
    int                 m_numIterations;
    int                 m_numPortfolios;
    double              m_mutationFactor;
    double              m_crossoverRate;
    double              m_swapProbability;
    bool                m_asParallel;
    int                 m_degreeOfParallelism;
    const PortfolioTrackerCallback m_callback;
    
public:
    NATIVEPARALLELPORTFOLIOANALYSIS_API 
	PortfolioTracker(
		__in SymbolPricesPackage &data, 
		__in const int maxContributors, 
		__in const int numIterations, 
		__in const int numPortfolios, 
		__in const double mutationFactor, 
		__in const double crossoverRate, 
		__in const double swapProbability, 
		__in const bool asParallel, 
		__in const int degreeOfParallelism,
        __in const PortfolioTrackerCallback callback
		);

    NATIVEPARALLELPORTFOLIOANALYSIS_API 
	PortfolioTracker(
		__in SymbolPricesPackage &data, 
		__in const int maxContributors, 
		__in const int numIterations, 
		__in const int numPortfolios, 
		__in const double mutationFactor, 
		__in const double crossoverRate, 
		__in const double swapProbability, 
		__in const bool asParallel, 
		__in const int degreeOfParallelism,
        __in unbounded_buffer<PortfolioTrackerProgressEventArgs *> *latestResultQueue
		);

    /// Performs the search for an optimum tracking portfolio using a generated population of 
    /// individuals as the starting point.
    /// this function will lock the _latestResult for write
    void 
    NATIVEPARALLELPORTFOLIOANALYSIS_API 
    Track();

    //---------------------------------------------------------------------------------------------
    // events
    //---------------------------------------------------------------------------------------------

    /// Return latest result, this function will lock the _latestResult for read
    void 
    NATIVEPARALLELPORTFOLIOANALYSIS_API 
    GetLatestResult(
        __out PortfolioTrackerProgressEventArgs &latestResult
        );

    /// signal the tracker that no more search is required
    void 
    NATIVEPARALLELPORTFOLIOANALYSIS_API 
    Cancel();

    //---------------------------------------------------------------------------------------------
    // instance properties
    //---------------------------------------------------------------------------------------------

    /// Gets the SymbolPricesPackage used by the PortfolioTracker for regression.
    SymbolPricesPackage &Data() { return m_data; }
    
    /// Gets the maximum number of index members that can contributed to the tracking portfolio.
    /// For example, if you want to create a tracking portfolio of the S&P 500 with fifty stocks,
    /// this would be set to fifty.
    int MaxContributors() { return m_maxContributors; }
    
    /// Gets the total number of index members. For example, the S&P 500 has 500 members.
    int NumInstruments() { return (int)m_data.m_Members.size(); }
    
    /// Gets the total number of iterations to perform in search of the optimum tracking portfolio.
    int NumIterations() { return m_numIterations; }
    
    /// Gets the total number of portfolios to include in each population. Also known as the 
    /// population size within the differential evolution code.
    int NumPortfolios() { return m_numPortfolios; }
    
    /// Gets the mutation factor for differential evolution. See DifferentialEvolution.
    double MutationFactor() { return m_mutationFactor; }  //0.3

    /// Gets the crossover rate for differential evolution. See DifferentialEvolution.
    double CrossoverRate() { return m_crossoverRate; }  //0.8

    /// Gets the swap probability. See permute above.
    double SwapProbability() { return m_swapProbability; }  //0.1
    
    /// Gets whether or not the algorithm should leverage parallelism.  
    bool AsParallel() { return m_asParallel; }  //true
    
    /// Gets the desired override for degree of parallelism. When set to any value < 1, the 
    /// algorithm will use the default degree of parallelism, usually the processor count.
    int DegreeOfParallelism() { return m_degreeOfParallelism; }
    
    /// Gets the PortfolioHistory for the index being tracked.
	PortfolioHistory &IndexHistory() { return m_indexHistory; }    

private:

	PortfolioHistory m_indexHistory;

    //---------------------------------------------------------------------------------------------
    // events
    //---------------------------------------------------------------------------------------------

    /// reader writer lock to protect the shared _latestResult
    reader_writer_lock m_latestResultLock;

    /// shared between GetLatestResult( read ), and Track( write )
    PortfolioTrackerProgressEventArgs m_latestResult;

	//
	unbounded_buffer<PortfolioTrackerProgressEventArgs *> *m_latestResultQueue;

    LONG volatile m_canceled;
    
    //---------------------------------------------------------------------------------------------
    // instance methods
    //---------------------------------------------------------------------------------------------

    /// Calculates the PortfolioHistory for a particular individual in the population. Each individual
    /// has different weights for the various members of the index being tracked. This method
    /// multiplies the weight for each member against its price at time <c>t</c> and then takes the 
    /// sum to get the prices of the individual at time <c>t</c>. This is performed for all times 
    /// <c>t</c>.
    PortfolioHistory TrackHistory( Individual &w );

    /// Performs the search for an optimum tracking portfolio using the provided population of
    /// individual as the starting point.
    void Track( Population &p );

    //---------------------------------------------------------------------------------------------
    // Algorithm
    //---------------------------------------------------------------------------------------------

    /// Gets the differential evolution implementation being used.
	class NATIVEPARALLELPORTFOLIOANALYSIS_API Algorithm : public DifferentialEvolution
    {
	public:
	    // The DifferentialEvolution class is an abstract class. Here we use the F# object 
	    // expression syntax to generate an anonymous derivation.
	    Algorithm(
	        __in const size_t numChromosomes,
	        __in const size_t numIndividuals,
	        __in const double mutationFactor,
	        __in const double crossoverRate,
	        __in const bool   asParallel,
	        __in const int    degreeOfParallelism
	        ) : DifferentialEvolution( numChromosomes, // numChromosomes = NumInstruments()
	            					   numIndividuals, // numIndividuals = _numPortfolios
							           mutationFactor,
							           crossoverRate,
							           asParallel,
							           degreeOfParallelism )
	    {};
		
		virtual Individual GenerationStrategy( int i );
            
		virtual void ReparationStrategy( Individual& );
            
		virtual void EvaluationStrategy( Individual& );

		void SetTracker ( PortfolioTracker *portfolioTracker ) { m_thisPortfolioTracker = portfolioTracker; }
	private:
		PortfolioTracker *m_thisPortfolioTracker;
    };

	Algorithm m_algorithm;
};

#endif // _PORTFOLIO_TRACKER_H_