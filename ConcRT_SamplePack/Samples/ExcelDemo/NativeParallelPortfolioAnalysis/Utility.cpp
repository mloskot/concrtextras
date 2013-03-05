/***
* ==++==
*
* Copyright (c) Microsoft Corporation.  All rights reserved.
*
* ==--==
* =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*
* Utility.cpp
*
* =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
****/

#include "stdafx.h"
#include "Utility.h"

//-------------------------------------------------------------------------------------------------
// Randomization
//------------------------------------------------------------------------------------------------

combinable< MyEng > eng_combinable;
MyEng g_eng;

///
/// returns randoum double from 0.0 - 1.0
///
double 
DoubleRand(
    MyEng& eng
    )
{
	uniform_real< double > dist( 0.0, 1.0 );

    return dist( eng );
}

///
/// returns randoum int from min - max
///
int 
IntRand(
    MyEng& eng,
    __in const int min, 
    __in const int max
    )
{
	uniform_int< int > dist( min, max );

    return dist( eng );
}

/// selects a random integer from min - max execluding lelments in skip
int 
random_select_except(
    MyEng& eng,
    __in const int min, 
    __in const int max, 
    __in const int skip[], 
    __in const int skipSize
    )
{
	if ( skipSize <= 0 )
	{
		return IntRand( eng, min, max );
	}

	uniform_int< int > dist( min, max );

	int retVal;

	bool repeat = false;
	do
	{
		// get a new randome int
		retVal = dist( eng );
		
		// assume it is unique
		repeat = false;

		for( int i = 0 ; i < skipSize ; ++i )
		{
			if( skip[ i ] == retVal )
			{
				// not unique, get a new value
				repeat = true;
				break;
			}
		}

	} while ( repeat );
	
	return retVal;
}


//-------------------------------------------------------------------------------------------------
// Math functions
//------------------------------------------------------------------------------------------------

void Normalize( 
	MyVector( double ) &v 
	)
{
	double sum = 0.0;
	for_each( v.begin(), v.end(), [&]( double &i ) { sum += i; } );
	for_each( v.begin(), v.end(), [&]( double &i ) { i /= sum; } );
}

double 
RootMeanSquare ( 
	MyVector( double ) &v 
	)
{
	double retVal = 0.0;

	for_each( v.begin(), v.end(), [&]( double & val ) { retVal += ( val * val ); });

	return sqrt( retVal / v.size() );
}

double 
TrackingError ( 
	MyVector( double ) &v,
	MyVector( double ) &r 
	)
{
	double retVal = 0.0;

	for( MyVector( double )::const_iterator iv = v.begin(), ir = r.begin();
	     iv != v.end(); 
		 ++iv, ++ir )
	{
		double error = *ir - *iv;
		retVal += error * error;
	}

	return sqrt( retVal / v.size() );
}   
