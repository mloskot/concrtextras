/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* DifferentialEvolution.cpp
*
* implementation of differential evolution algorithem
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

#include "DifferentialEvolution.h"

//-------------------------------------------------------------------------------------------------
// Individual
//------------------------------------------------------------------------------------------------

Individual::
Individual()
{ };

        
Individual::
Individual(
    __in const int chromosomeSize
	) : m_Chromosomes( chromosomeSize ),
		m_Fitness( defaultFitness )
{ }

Individual::
Individual(
    __in const Individual& source
	) : m_Fitness( source.m_Fitness )
{
    m_Chromosomes.assign( 
        source.m_Chromosomes.begin(), 
        source.m_Chromosomes.end() );
};


Individual::
Individual(
    MyVector( double ) &chromosomes,
    double fitness
	) : m_Fitness( fitness )
{
    m_Chromosomes.assign( 
        chromosomes.begin(), 
        chromosomes.end() );
}

bool 
Individual::
operator == ( Individual &rhs )
{
    if( m_Fitness != rhs.m_Fitness)
    {
        return false;
    }
    
    return equal< MyVector( double )::const_iterator >( 
        m_Chromosomes.begin(), 
        m_Chromosomes.end(), 
        rhs.m_Chromosomes.begin() );
}

const double Individual::defaultFitness = 0.0;
const double Individual::defaultChromosome = 0.0;

//-------------------------------------------------------------------------------------------------
// Population
//------------------------------------------------------------------------------------------------

Population::
Population(
    ) : m_Diversity( 0.0 ),
        m_Optimum( uninitialized )
{
}

Population::
Population(
    __in const Population& source
    ) : m_Individuals( source.m_Individuals ),
        m_Diversity( source.m_Diversity ),
        m_Optimum( source.m_Optimum )
{
}

Population::
Population(
    __in const size_t Population_Size, 
    __in const size_t Chromosome_Size
    ) : m_Individuals( Population_Size, Individual( Chromosome_Size ) ),
        m_Diversity( 0.0 ),
        m_Optimum( uninitialized )
{ };

/// Given a population and an individual that is not a member of the population, migrate the
/// individual into a random position in the population (taking care not to overwrite the 
///  current optimum individual).
void 
Population::
Migrate( 
    __in const Individual w 
	)
{
    //if p.Optimum.IsNone         then invalidArg "p" "The provided population does not include an optimum individual."
    //if p.Individuals.Length < 2 then invalidArg "p" "The provided population does not have enough individuals."
    
    // randomly select two indexes from population; if the first happens to be the optimum
    // individual then use the second.
    int skipOptimum = ( int ) m_Optimum;

    MyEng &eng = g_eng;
    
	int migrator = random_select_except( eng, 0, m_Individuals.size(), &skipOptimum, 1 );

	m_Individuals[ migrator ] = w;
}


const size_t Population::uninitialized = -1;

//-------------------------------------------------------------------------------------------------
// DifferentialEvolution
//------------------------------------------------------------------------------------------------
DifferentialEvolution::
DifferentialEvolution(
    __in const size_t numChromosomes,
    __in const size_t numIndividuals,
    __in const double mutationFactor,
    __in const double crossoverRate,
    __in const bool   asParallel,
    __in const int    degreeOfParallelism
    ) : m_numChromosomes( numChromosomes ),
        m_numIndividuals( numIndividuals ),
        m_mutationFactor( mutationFactor ),
        m_crossoverRate( crossoverRate ),
        m_asParallel( asParallel ),
        m_degreeOfParallelism( degreeOfParallelism )
{};


/// Performs mutation for individual i of population p. Mutation is achieved by selecting three
/// individuals (or donors) of the population other than the individual at i and mutating their 
/// chromosomes. Mutation of donors a, b, c is defined by a + MF( b - c ) where MF is the
/// mutation factor.
Individual
DifferentialEvolution::
Mutate( 
    __in Population &p,
    __in const int i
    )
{
	// will store the mutation individuals, a in 1, b in 2, and c in 3
	int individuals_ids[] = { i, i, i, i };

    MyEng &eng = g_eng;

    individuals_ids[ 1 ] = random_select_except( eng, 0, p.m_Individuals.size() - 1, individuals_ids, 1 ); // donor a, skip i
	individuals_ids[ 2 ] = random_select_except( eng, 0, p.m_Individuals.size() - 1, individuals_ids, 2 ); // donor b, skip i, a
	individuals_ids[ 3 ] = random_select_except( eng, 0, p.m_Individuals.size() - 1, individuals_ids, 3 ); // donor c, skip i, a, b
    
	int numChromosomes = p.m_Individuals[ i ].m_Chromosomes.size();
    Individual x( numChromosomes );
    Individual &a = p.m_Individuals[ individuals_ids[ 1 ] ];
    Individual &b = p.m_Individuals[ individuals_ids[ 2 ] ];
    Individual &c = p.m_Individuals[ individuals_ids[ 3 ] ];

	for( int j = 0 ; j < numChromosomes ; ++j )
	{
		x.m_Chromosomes[ j ] = a.m_Chromosomes[ j ] + m_mutationFactor * ( b.m_Chromosomes[ j ] - c.m_Chromosomes[ j ] );
	}

	return x;
}

/// Performs recombination of the mutated individual with the corresponding individual of the
/// previous generation. Recombination is achieved by copying individual chromosomes from the
/// mutated individual back to the original with probability equal to the crossover rate.    
void 
DifferentialEvolution::
Recombine( 
    __in Population &p,
    __in const int i, 
    __in Individual &x 
    )
{
    Individual &m = p.m_Individuals[ i ];

    MyEng &eng = g_eng;
    
    for( size_t j = 0 ; j < m_numChromosomes ; ++j )
	{
        bool crossover = ( ( IntRand( eng, 0, m_numChromosomes ) == j ) || ( DoubleRand( eng ) <= m_crossoverRate ) );
		if( crossover )
        {
            x.m_Chromosomes[ j ] = m.m_Chromosomes[ j ];
        }
	}
}

/// Performs reparation of the recombined individual.
void 
DifferentialEvolution::
Repair( 
    __in Individual &x 
    )
{
	ReparationStrategy( x );
}

/// Performs selection of the individuals with improved fitness. Each repaired individual is
/// compared against its corresponding individual from the previous generation. If the repaired
/// individual is better (that is, has a lower fitness value), it is propagated to the new
/// generation. If it is worse, the individual from the previous generation is propagated.
void 
DifferentialEvolution::
Select( 
    __in Population &p,
    __in const int i, 
    __in Individual &x 
    )
{
	Individual &y = p.m_Individuals[ i ];

	if( m_asParallel )
	{
		parallel_invoke(
			[&]() { EvaluationStrategy( x ); },
			[&]() { EvaluationStrategy( y ); } );
	}
	else
	{
		EvaluationStrategy( x );
		EvaluationStrategy( y );
	}
	
	if( x.m_Fitness < y.m_Fitness )
	{
		y = x; // copy the indvidual with optimal fitness
	}
}

/// Analyzes the new population and calculates the population diversity and the individual with
/// optimal fitness. Diversity is defined as the average standard deviation of each chromosome
/// across all individuals.   
void 
DifferentialEvolution::
Measure(
    __in Population &p
    )
{
    //
    // no need to parallelize this function as it is always called from parallelized callers
    //
	if( m_asParallel )
	{
		combinable< double > sum_of_sd_comb( [=](){ return 0.0; } );

        parallel_for< size_t >( 0, m_numChromosomes, [&]( size_t chromosome )
        {
            double &local_sum_of_sd = sum_of_sd_comb.local();
            double sum = 0.0, sum_of_squares = 0.0;

            for( size_t individual = 0 ; individual < m_numIndividuals ; ++individual )
            {
                Individual &x = p.m_Individuals[ individual ];
                sum += x.m_Chromosomes[ chromosome ];
                sum_of_squares += ( x.m_Chromosomes[ chromosome ] * x.m_Chromosomes[ chromosome ] );

                // pick the optimum individual in the first loop
                if( chromosome == 0 )
                {
                    // No need for protection here, this will execute only once
                    if( p.m_Optimum == Population::uninitialized )
                    {
                        p.m_Optimum = individual;
                    }
                    else
                    {
                        if( p.m_Individuals[ p.m_Optimum ].m_Fitness > x.m_Fitness )
                        {
                            p.m_Optimum = individual;
                        }
                    }
                }
            }
            
            local_sum_of_sd += sqrt( sum_of_squares - sum * sum / ( double ) m_numChromosomes ) / ( ( double ) m_numChromosomes - 1 );
        } );
        
        double sum_of_sd = sum_of_sd_comb.combine(std::plus<double>());
        p.m_Diversity = sum_of_sd / ( double ) m_numChromosomes;
	}
	else
	{
		double sum_of_sd = 0.0;
        
		for( size_t chromosome = 0 ; chromosome < m_numChromosomes ; ++chromosome )
        {
            double sum = 0.0, sum_of_squares = 0.0;
            for( size_t individual = 0 ; individual < m_numIndividuals ; ++individual )
            {
                Individual &x = p.m_Individuals[ individual ];
                sum += x.m_Chromosomes[ chromosome ];
                sum_of_squares += ( x.m_Chromosomes[ chromosome ] * x.m_Chromosomes[ chromosome ] );

                // pick the optimum individual in the first loop
                if( chromosome == 0 )
                {
                    if( p.m_Optimum == Population::uninitialized )
                    {
                        p.m_Optimum = individual;
                    }
                    else
                    {
                        if( p.m_Individuals[ p.m_Optimum ].m_Fitness > x.m_Fitness )
                        {
                            p.m_Optimum = individual;
                        }
                    }
                }
            }
            
            sum_of_sd += sqrt( sum_of_squares - sum * sum / ( double ) m_numChromosomes ) / ( ( double ) m_numChromosomes - 1 );
        }
        
        p.m_Diversity = sum_of_sd / ( double ) m_numChromosomes;
	}
}

/// Creates a seed population for differential evolution.                
Population 
DifferentialEvolution::
Create()
{
    Population p( m_numIndividuals, m_numChromosomes );
    
	if( m_asParallel )
	{
	    parallel_for< size_t >( 0, m_numIndividuals, [&]( size_t i)
	    {
	        Individual x( GenerationStrategy( ( int )i ) );
	        EvaluationStrategy( x );
	        p.m_Individuals[ i ] = x;
	    } );
	}
	else
	{
	    for( size_t i = 0 ; i < m_numIndividuals ; ++i )
	    {
	        Individual x( GenerationStrategy( ( int )i ) );
	        EvaluationStrategy( x );
	        p.m_Individuals[ i ] = x;
	    }
	}
    
    return p;
}

/// Evolves a population one generation.    
Population 
DifferentialEvolution::
Evolve(
    __in Population &p
    )
{
    Population newPopulation( p );
    Evolve( p, newPopulation);
    return newPopulation;
}

void 
DifferentialEvolution::
Evolve(
    __in Population &p,
    __inout Population &newPopulation
    )
{
 	if( m_asParallel )
	{
        parallel_for< size_t >( 0, m_numIndividuals, [&]( size_t i )
        {
            Individual y = Mutate( p, i );
            
            Recombine( p, i, y );
            Repair( y );
            Select( newPopulation, i, y );
        } );
    }
    else
    {
        for( size_t i = 0 ; i < m_numIndividuals ; ++i )
        {
            Individual y = Mutate( p, i );
            
            Recombine( p, i, y );
            Repair( y );
            Select( newPopulation, i, y );
        }
    }
    
    Measure( newPopulation );
}