/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* PortfolioTracker.cpp
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

#include "PortfolioTracker.h"

//-------------------------------------------------------------------------------------------------
// PortfolioHistory
//------------------------------------------------------------------------------------------------

PortfolioHistory::
PortfolioHistory()
{
}

/// Represents the performance history of a portfolio. 
PortfolioHistory::
PortfolioHistory(
    __in MyVector( double ) &prices 
    ) : m_Prices( prices ),
        m_LogReturns( prices.size() - 1, 0.0 )
{
    MyVector( double )::const_iterator p;
    MyVector( double )::iterator l;
    for( p = m_Prices.begin(), l = m_LogReturns.begin();
         p != m_Prices.end() - 1;
         ++p, ++l )
    {
        if( IN_CUT_OFF( *p, MY_EPSILON ) || IN_CUT_OFF( *( p + 1 ), MY_EPSILON ) )
        {
            (*l) = 0.0;
        }
        else
        {
            (*l) = log( *( p + 1 ) / *p ); 
        }
    };
}

//-------------------------------------------------------------------------------------------------
// PortfolioTracker
//------------------------------------------------------------------------------------------------
PortfolioTracker::
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
    ) : m_data( data ),
        m_maxContributors( maxContributors ),
        m_numIterations( numIterations ),
        m_numPortfolios( numPortfolios ),
        m_mutationFactor( mutationFactor ),
        m_crossoverRate( crossoverRate ),
        m_swapProbability( swapProbability ),
        m_asParallel( asParallel ),
        m_degreeOfParallelism( degreeOfParallelism ),
        m_canceled( 0 ),
        m_callback( NULL ),
		m_latestResultQueue( latestResultQueue ),
        m_algorithm(
            NumInstruments(), // numChromosomes = NumInstruments
            m_numPortfolios,   // numIndividuals = numPortfolios
            m_mutationFactor,
            m_crossoverRate,
            m_asParallel,
            m_degreeOfParallelism )
{ 
    m_algorithm.SetTracker( this );
	m_indexHistory = PortfolioHistory( m_data.m_Index.m_Prices );
}

PortfolioTracker::
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
    ) : m_data( data ),
        m_maxContributors( maxContributors ),
        m_numIterations( numIterations ),
        m_numPortfolios( numPortfolios ),
        m_mutationFactor( mutationFactor ),
        m_crossoverRate( crossoverRate ),
        m_swapProbability( swapProbability ),
        m_asParallel( asParallel ),
        m_degreeOfParallelism( degreeOfParallelism ),
        m_canceled( 0 ),
        m_callback( callback ),
		m_latestResultQueue( NULL ),
        m_algorithm(
            NumInstruments(), // numChromosomes = NumInstruments
            m_numPortfolios,   // numIndividuals = numPortfolios
            m_mutationFactor,
            m_crossoverRate,
            m_asParallel,
            m_degreeOfParallelism )
{ 
    m_algorithm.SetTracker( this );
	m_indexHistory = PortfolioHistory( m_data.m_Index.m_Prices );
}

/// Performs the search for an optimum tracking portfolio using a generated population of 
/// individuals as the starting point.
void 
PortfolioTracker::
Track()
{
	g_eng.seed( RANDOMIZATIONSEED );

	return Track( m_algorithm.Create() );
}
    
/// Performs the search for an optimum tracking portfolio using the provided population of
/// individual as the starting point.
void 
PortfolioTracker::
Track( 
    Population &p 
    )
{
    Population p1( p );
    Population p2( p );
    Population *prev = &p1;
    Population *next = &p2;
	Population *best;

    for( int i = 0 ; i < m_numIterations ; ++i )
    {
        m_algorithm.Evolve( *prev, *next );

        bool needUpdate = ( ( next->m_Optimum != Population::uninitialized ) && (next->m_Optimum != prev->m_Optimum ) );
        needUpdate |= ( ( i + 1 ) == m_numIterations );

		if( i > 0 )
		{
			needUpdate &= (next->m_Individuals[next->m_Optimum ].m_Fitness < prev->m_Individuals[ prev->m_Optimum ].m_Fitness );
		}

        if( needUpdate )
        {
			best = next;
        }
		else
		{
			best = prev;
		}

        Population *temp = prev;
        prev = next;
        next = temp;

		PortfolioTrackerProgressEventArgs * latestResult = NULL;

		if( m_asParallel )
		{
			latestResult = ( PortfolioTrackerProgressEventArgs* ) Alloc( sizeof( PortfolioTrackerProgressEventArgs ) );
			new( reinterpret_cast< void * >( latestResult ) ) PortfolioTrackerProgressEventArgs();
			latestResult->m_Generation = i;

            parallel_invoke(
                [&]() 
    			{ 
    				latestResult->m_Population = *best; 
    			},
                [&]() 
        		{ 
                    if ( best->m_Optimum != Population::uninitialized )
                    {
                        latestResult->m_PopulationBest = TrackHistory( best->m_Individuals[ best->m_Optimum ] );
                    }
                    else
                    {
                        latestResult->m_PopulationBest = PortfolioHistory( MyVector( double )( m_data.m_Index.m_Prices.size(), 0.0 ) );
                    }
        		} );

			if( i + 1 == m_numIterations )
			{
		        m_latestResultLock.lock();
				m_latestResult = *latestResult; 
				m_latestResultLock.unlock();
			}
			
			if( m_latestResultQueue != NULL )
			{
				asend<PortfolioTrackerProgressEventArgs *>(
					m_latestResultQueue, 
					latestResult );
			}
		}
		else
		{
            //
            // Excel can query these values while they are being used here
            // this critical section is to make access to results thread safe
            //
	        m_latestResultLock.lock();

			m_latestResult.m_Generation = i;

            m_latestResult.m_Population = *best;
            if ( best->m_Optimum != Population::uninitialized )
            {
                m_latestResult.m_PopulationBest = TrackHistory( best->m_Individuals[ best->m_Optimum ] );
            }
            else
            {
                m_latestResult.m_PopulationBest = PortfolioHistory( MyVector( double )( m_data.m_Index.m_Prices.size(), 0.0 ) );
            }

			m_latestResultLock.unlock();

	        if( m_callback != NULL )
	        {
	            m_callback( this );
			}
		}

        if( m_canceled != 0 )
        {
            break;
        }
    }
}

/// Return latest result, this function will lock the _latestResult for read
void
PortfolioTracker::
GetLatestResult(
    __out PortfolioTrackerProgressEventArgs &latestResult
    )
{
    m_latestResultLock.lock_read();

    latestResult = m_latestResult;
    
    m_latestResultLock.unlock();
}

/// signal the tracker that no more search is required
void 
PortfolioTracker::
Cancel()
{
    ::InterlockedIncrement( &m_canceled );
}


PortfolioHistory 
PortfolioTracker::
TrackHistory( 
    Individual &w 
    )
{
    MyVector( int ) indexes;

    for( size_t i = 0 ; i < w.m_Chromosomes.size() ; ++i )
    {
        double thisChromosome = w.m_Chromosomes[ i ];

        if( !IN_CUT_OFF( thisChromosome, MY_EPSILON ) )
        {
            indexes.push_back( (int)i );
        }
    }

    const size_t pricesSize = m_data.m_Index.m_Prices.size();
    MyVector( double ) prices( pricesSize, 0.0 );

    if( m_asParallel )
    {
        parallel_for< size_t >( 0, pricesSize, [&]( size_t j )
        {
            {
                double newPrice = 0.0;

                for( size_t i = 0 ; i < indexes.size() ; ++i )
                {
                    int index = indexes[ i ];
                    newPrice += m_data.m_Members[ index ].m_Prices[ j ] * w.m_Chromosomes[ index ];
                }

                prices[ j ] = newPrice;
            }
        } );
    }
    else
    {
        for( size_t j = 0 ; j < pricesSize ; ++j )
        {
            double newPrice = 0.0;

            for( size_t i = 0 ; i < indexes.size() ; ++i )
            {
                int index = indexes[ i ];
                newPrice += m_data.m_Members[ index ].m_Prices[ j ] * w.m_Chromosomes[ index ];
            }

            prices[ j ] =  newPrice;
        }
    }

    return PortfolioHistory( prices );
}

//---------------------------------------------------------------------------------------------
// Algorithm
//---------------------------------------------------------------------------------------------

/// Generates a tracking portfolio (also known as an Individual) of m members for an index of 
/// length n. 
Individual 
PortfolioTracker::
Algorithm::
GenerationStrategy( 
    int i 
    )
{
    /// When i > 0, the generated portfolio is comprised of m normalized random weights 
    /// sprinkled into an zeroed array of length n. When i = 0, the generated portfolio is a 
    /// repaired form of the index weights. As m approaches n, the repaired version of the 
    /// index weights should be reasonably close to optimum.
    Individual retVal( (int)m_numChromosomes );

	bool existing = false;
    MyEng &eng = g_eng; // eng_combinable.local(existing); // g_eng;
	if( !existing )
	{
		eng.seed( RANDOMIZATIONSEED );
	}
    
    if( i == 0 )
    {
        MyVector( SymbolPrices )::const_iterator sp;
        MyVector( double )::iterator c;

        for( sp = m_thisPortfolioTracker->m_data.m_Members.begin(),  c = retVal.m_Chromosomes.begin();
             sp != m_thisPortfolioTracker->m_data.m_Members.end();
             ++sp, ++c )
        {
            *c = (*sp).m_Weight;
        }

        Normalize( retVal.m_Chromosomes );

        ReparationStrategy( retVal );
    }
    else
    {
        //
        // randomly pick _thisPortfolioTracker->_maxContributors from _numChromosomes, execluding the ones you picked before
        //
        int *randomChromosomes = new int[m_thisPortfolioTracker->m_maxContributors];
        for( int i = 0 ; i < m_thisPortfolioTracker->m_maxContributors ; ++i )
        {
            randomChromosomes[ i ] = random_select_except( eng, 0, (int)m_numChromosomes - 1, randomChromosomes, i );
            retVal.m_Chromosomes[ randomChromosomes[ i ] ] = DoubleRand( eng );
        }

        Normalize( retVal.m_Chromosomes );

        delete randomChromosomes;
    }

    return retVal;
}
            
void 
PortfolioTracker::
Algorithm::
ReparationStrategy( 
    Individual& v
    )
{
    /// Repairs an individual tracking portfolio such that it observes all constraints. For 
    /// example, after mutation, there is likely to be more instruments with non-zero weights than
    /// is allowed. The specific reparation strategy here is to take the top n non-zero weights, 
    /// normalize them, and then zero out the all other weights. This reparation strategy also 
    /// implements the swap operator as explained above (see permute).

    MyVector( double ) sortedChromosomes( m_thisPortfolioTracker->m_maxContributors );
    partial_sort_copy( 
        v.m_Chromosomes.begin(), 
        v.m_Chromosomes.end(), 
        sortedChromosomes.begin(), 
        sortedChromosomes.end(), 
        [=]( double a, double b) -> bool { return ( a > b ); } );

    double nth = sortedChromosomes.back();

    replace_if( 
        v.m_Chromosomes.begin(), 
        v.m_Chromosomes.end(), 
        bind2nd(less<double>(), nth), 0.0 ); // this line produced bug:Dev10:728731 [=]( double value ) -> bool { return value < nth; } , 0.0 );
}

void 
PortfolioTracker::
Algorithm::
EvaluationStrategy( 
    Individual& v
    )
{
    /// Evaluates the fitness for a specific individual. The fitness is used by the differential
    /// evolution algorithm to pick those individuals that will advance to the next generation. The
    /// calculation we use for fitness is tracking error. Tracking error is defined as the
    /// root-mean-square of differences between the log returns of the index and the individual.

    PortfolioHistory indexHistory = m_thisPortfolioTracker->IndexHistory();
    PortfolioHistory trackHistory = m_thisPortfolioTracker->TrackHistory( v );

    double trackingError = TrackingError( indexHistory.m_LogReturns, trackHistory.m_LogReturns );

    v.m_Fitness = trackingError;
}
