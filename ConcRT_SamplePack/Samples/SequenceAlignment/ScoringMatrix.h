// ==++== 
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// File: ScoringMatrix.h
//
// Contains declaration for ScoringMatrix class.
//
// ==--==

#pragma once

#include "stdafx.h"
#include "BLOSUM62.h"

using namespace std;

/// <summary>
/// Scoring Matrix used for static match/mismatch scores
/// </summary>
class ScoringMatrix
{
public:
    ScoringMatrix()
    {
        unsigned int ctr = 0;
        m_map.insert(make_pair('A', ctr++)); 
        m_map.insert(make_pair('R', ctr++)); 
        m_map.insert(make_pair('N', ctr++)); 
        m_map.insert(make_pair('D', ctr++)); 
        m_map.insert(make_pair('C', ctr++)); 
        m_map.insert(make_pair('Q', ctr++)); 
        m_map.insert(make_pair('E', ctr++)); 
        m_map.insert(make_pair('G', ctr++)); 
        m_map.insert(make_pair('H', ctr++)); 
        m_map.insert(make_pair('I', ctr++)); 
        m_map.insert(make_pair('L', ctr++)); 
        m_map.insert(make_pair('K', ctr++)); 
        m_map.insert(make_pair('M', ctr++)); 
        m_map.insert(make_pair('F', ctr++)); 
        m_map.insert(make_pair('P', ctr++)); 
        m_map.insert(make_pair('S', ctr++)); 
        m_map.insert(make_pair('T', ctr++)); 
        m_map.insert(make_pair('W', ctr++)); 
        m_map.insert(make_pair('Y', ctr++)); 
        m_map.insert(make_pair('V', ctr++)); 
        m_map.insert(make_pair('B', ctr++)); 
        m_map.insert(make_pair('Z', ctr++)); 
        m_map.insert(make_pair('X', ctr++)); 
        m_map.insert(make_pair('*', ctr++));    

        assert(ctr == MATRIX_SIZE);
    }

    const int GetScore(const char& a, const char& b) 
    {
        auto iter1 = m_map.find(a);
        auto iter2 = m_map.find(b);
        int aVal = iter1->second;
        int bVal = iter2->second;
        if ((aVal < 0)||(aVal > 23))
        {
            aVal = 23;
        }
        if ((bVal < 0)||(bVal > 23))
        {
            bVal = 23;
        }
        return BLOSUM62[aVal][bVal];
    }

private:
    unordered_map<int, char> m_map;
};