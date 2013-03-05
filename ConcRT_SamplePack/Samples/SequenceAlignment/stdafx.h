// ==++== 
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// File: stdafx.h
//
// Include files for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
// ==--==

#pragma once

#include <windows.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <utility>
#include <assert.h>
#include <algorithm>
#include <limits.h>
#include <unordered_map>
#include <time.h>
#include <concrtrm.h>
#include <ppl.h>
#include <agents.h>
#include <concurrent_queue.h>
#include "..\..\concrtextras\ppltasks.h"

static const unsigned int MAX_QUERY_LENGTH_INIT_ALLOC=5000;

static const unsigned int DIAG = 0x1;
static const unsigned int TOP = 0x2;
static const unsigned int LEFT = 0x4;
static const unsigned int DONE = 0x8;

static const char IDENTITY = '|';
static const char SIMILARITY = ':';
static const char GAP = ' ';
static const char MISMATCH = '.';

#define MAX3(x1, x2, x3) max(max(x1, x2), x3)
#define MAX4(x1, x2, x3, x4) max(max(x1, x2), max(x3, x4))

using namespace std;
using namespace Concurrency;
using namespace Concurrency::samples;

/// <summary>
/// This is used by Smith_Waterman algorithm, to build a matrix of cells
/// </summary>
struct Cell
{
    Cell()
        :m_localAlignScore_E(0),
        m_localAlignScore_F(0),
        m_maxScore(0),
        m_direction(0)
    {
    }

    int m_localAlignScore_E;
    int m_localAlignScore_F;
    int m_maxScore;
    unsigned char m_direction;
};

/// <summary>
/// This is a temporary structured used by Smith Waterman algorithm to 
/// communicate winning score and the row and column corrosponding to the
/// winning score. 
/// Based on the score, it may (or may not) be worthwhile to
/// backtrack, in order to get the aligned sequence
/// </summary>
class Element
{
public:
    Element() :row(0), col(0), score(0)
    {
    }

    Element(int r, int c, int s) :row(r), col(c), score(s)
    {
    }
    void Set(int r, int c, int s)
    {
        row = r;
        col = c;
        score = s;
    }
    const int GetRow() { return row; }
    const int GetColumn() { return col; }
    const int GetScore() { return score; }
private:
    int row;
    int col;
    int score;
};


