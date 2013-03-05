/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* DifferentialEvolution.h
*
* implementation of differential evolution algorithem
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"

#pragma once

//-------------------------------------------------------------------------------------------------
// Individual
//------------------------------------------------------------------------------------------------

/// Represents an individual of the population being searched by differential evolution. An 
/// individual is comprised of an array of chromosomes (also known as weights) and its fitness.
struct Individual
{ 
    Individual();
    
    Individual(
        __in const int chromosomeSize
        );
    
    Individual(
        __in const Individual& source
        );
    
    Individual(
        MyVector( double ) &chromosomes,
        double fitness
        );
    
    // 
    // chromosomes represent the feature that we are optimizing
    //
    MyVector( double ) m_Chromosomes; 

    //
    // stores the error, or how fit is the individual to the solution
    // the closer the individual to that the feature the most likly he is 
    // going to get picked up during selection
    // 
    double m_Fitness;
    
    bool operator == ( Individual &rhs );
    bool operator != ( Individual &rhs ) { return !( operator == ( rhs ) ); };

    static const double defaultChromosome; // 0.0
    static const double defaultFitness; // 0.0
};

//-------------------------------------------------------------------------------------------------
// Population
//------------------------------------------------------------------------------------------------

/// Represents a population of individuals. A population is comprised of an array of individuals,
/// a measure of the population's diversity (in other words, how different the individuals are
/// from each other), and the population's current optimal individual.

struct Population
{ 
    NATIVEPARALLELPORTFOLIOANALYSIS_API
    Population();

    NATIVEPARALLELPORTFOLIOANALYSIS_API
    Population(
        __in const Population& source
        );
        
    Population(
        __in const size_t Population_Size, 
        __in const size_t Chromosome_Size
        );
    
	/// Given a population and an individual that is not a member of the population, migrate the
    /// individual into a random position in the population (taking care not to overwrite the 
    ///  current optimum individual).
    void Migrate( 
        __in const Individual w 
		);

    //
    // a list of all individuals in the population
    //
    MyVector( Individual ) m_Individuals; 

    //
    // a measure of how different the individuals are, as we approch the 
    // best solution the diversity will grow smaller
    //
    double                 m_Diversity; 

    //
    // index of the optimal individual, the one that best fit the solution (lowest _Fitness)
    //
    size_t                 m_Optimum;
    
    static const size_t uninitialized; // -1
};

//-------------------------------------------------------------------------------------------------
// DifferentialEvolution
//------------------------------------------------------------------------------------------------

/// Encapsulates the algorithm of differential evolution. From Wikipedia: 
///
///    Differential Evolution (DE) is a method of mathematical optimization of multidimensional 
///    functions and belongs to the class of evolution strategy optimizers. DE finds the global 
///    minimum of a multidimensional, multimodal (i.e. exhibiting more than one minimum) function 
///    with good probability.
///
/// This class is abstracted from the specifics of any problem domain. Those specifics are 
/// introduced via strategy methods.

class NATIVEPARALLELPORTFOLIOANALYSIS_API DifferentialEvolution
{
public:
    DifferentialEvolution(
        __in const size_t numChromosomes,
        __in const size_t numIndividuals,
        __in const double mutationFactor,
        __in const double crossoverRate,
        __in const bool   asParallel,
        __in const int    degreeOfParallelism
        );

    //
    // number of chromozomes each individual will carry
    //
    const size_t m_numChromosomes;

    //
    // number of individuals in a pobulation
    //
    const size_t m_numIndividuals;

    //
    // ration by which new individuals mutate new chromosome = a + MF( b - c )
    // where a, b, & c are chromozom donnoers
    //
    const double m_mutationFactor;

    //
    // how much chromosomes will cross from previous generation to the new one
    //
    const double m_crossoverRate;

    //
    // enable parallel code path
    //
    const bool   m_asParallel;

    //
    // limit the degree of parallelizem
    //
    const int    m_degreeOfParallelism;

	//---------------------------------------------------------------------------------------------
	// abstract methods
	//---------------------------------------------------------------------------------------------

	/// Represents the strategy used to generate new individuals to seed evolution.
	virtual Individual GenerationStrategy( int ) = 0;

	/// Represents the strategy used to modify individuals after evolution to ensure that the
	/// individuals continue to observe all constraints.
	virtual void ReparationStrategy( Individual& ) = 0;

	/// Represents the strategy used to evaluate the fitness on an individual.
	virtual void EvaluationStrategy( Individual& ) = 0;

    //---------------------------------------------------------------------------------------------
    // instance methods
    //---------------------------------------------------------------------------------------------

    /// Creates a seed population for differential evolution.                
    Population Create();
        
    /// Evolves a population one generation.    
    Population Evolve( __in Population &p );
        
    /// Evolves a population one generation.    
    void Evolve( __in Population &p, __inout Population &newPopulation );
        
protected:
    
    //---------------------------------------------------------------------------------------------
    // instance methods
    //---------------------------------------------------------------------------------------------

    /// Performs mutation for individual i of population p. Mutation is achieved by selecting three
    /// individuals (or donors) of the population other than the individual at i and mutating their 
    /// chromosomes. Mutation of donors a, b, c is defined by a + MF( b - c ) where MF is the
    /// mutation factor.
	Individual Mutate( 
		__in Population &p,
		__in const int i
		);
    
    /// Performs recombination of the mutated individual with the corresponding individual of the
    /// previous generation. Recombination is achieved by copying individual chromosomes from the
    /// mutated individual back to the original with probability equal to the crossover rate.    
    void Recombine( 
		__in Population &p, 
		__in const int i, 
		__in Individual &x 
		);

    /// Performs reparation of the recombined individual.
    void Repair( 
		__in Individual &x 
		);
        
    /// Performs selection of the individuals with improved fitness. Each repaired individual is
    /// compared against its corresponding individual from the previous generation. If the repaired
    /// individual is better (that is, has a lower fitness value), it is propagated to the new
    /// generation. If it is worse, the individual from the previous generation is propagated.
    void Select( 
		__in Population &p, 
		__in const int i, 
		__in Individual &x
		);
    
    /// Analyzes the new population and calculates the population diversity and the individual with
    /// optimal fitness. Diversity is defined as the average standard deviation of each chromosome
    /// across all individuals.   
    void Measure( __in Population &p );
};