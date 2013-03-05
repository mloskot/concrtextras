// ==++== 
//
// Copyright (c) 2010 Microsoft Corporation.  All rights reserved.
//
// File: Smith_Waterman.h
//
// Contains declaration for Smith_Waterman class.
// Basically, we build a matrix and populate it in parallel, calculating and recording winners 
// based on cells to the left, top and upper-left diagonal to the current cell
// Therefore, given a diagonal, it has no inter-dependencies and hence can be parallelized.
// When we are doing database searches, it is more efficient if we were to use parallel_for which
// uses structured_task_groups under the covers. 
//
// ==--==

#pragma once

#include "stdafx.h"
#include "ScoringMatrix.h"
#include "Alignment.h"
#include "Result.h"


using namespace std;
using namespace Concurrency;

#pragma warning(disable:4018)
#pragma warning(disable:4267)

class Smith_Waterman
{
public:
    /// <summary>
    /// Ctor used when aligning a Query
    /// </summary>
    ///
    /// <param name="subject">The subject to align<param>
    /// <param name="query">The query to align the subject with<param>
    /// <param name="matrix">The scoring matrix to use<param>
    Smith_Waterman(string subject, string query, ScoringMatrix* pMatrix)
        :m_subjectLength(subject.length()),
        m_queryLength(query.length()),
        m_pMatrix(pMatrix)
    {
        m_pSubject = new char[m_subjectLength+1];
        strcpy_s(m_pSubject, m_subjectLength+1, subject.c_str());
        m_pQuery = new char[m_queryLength+1];
        strcpy_s(m_pQuery, m_queryLength+1, query.c_str());

        m_ppScoreMatrix = new volatile Cell*[m_subjectLength];
        for(int i=0; i <  m_subjectLength; ++i)
        {
            m_ppScoreMatrix[i] = new volatile Cell[m_queryLength];
        }
    }

    ~Smith_Waterman()
    {
        delete [] m_pSubject;
        delete [] m_pQuery;

        for (int i = 0; i < m_subjectLength;  ++i)
        {
            delete m_ppScoreMatrix[i];
        }
        delete [] m_ppScoreMatrix;
    }

    /// <summary>
    /// On analysis one of the bottlenecks around parallelism was around
    /// HeapLock (contention). The approach has been revised to minimize allocation/reallocation
    /// by object reuse.
    /// </summary>
    ///
    /// <param name="query">The query string from parsing the db<param>
    void SetQuery(string query)
    {
        if (query.length() > m_maxQueryLength)
        {
            //Free memory and reallocate
            m_maxQueryLength = query.length();
            delete [] m_pQuery;
            m_pQuery = new char[m_maxQueryLength+1];
            for (int i = 0; i < m_subjectLength;  ++i)
            {
                delete m_ppScoreMatrix[i];
                m_ppScoreMatrix[i] = new volatile Cell[m_maxQueryLength];
            }
        }

        m_queryLength = query.length();
        memset(m_pQuery, '\0', (m_queryLength+1) * sizeof(char));
        strcpy_s(m_pQuery, m_queryLength+1, query.c_str());

        for (int i = 0; i < m_subjectLength;  ++i)
        {
            //use placement new if Init values of Cell ctor changes
            memset((void*)m_ppScoreMatrix[i], '\0', m_queryLength * sizeof(volatile Cell));
        }

        assert(m_maxQueryLength >= m_queryLength);
    }


    /// <summary>
    /// Used when aligning Query
    /// This calls Popoluate followed by Traceback
    /// </summary>
    ///
    /// <param name="gap_open">static cost to open gap<param>
    /// <param name="gap_extn">static cost to extend gap<param>
    /// <param name="fRecursiveTraceback">Trace all paths/mutations<param>
    Result Align(int gap_open, int gap_extn, const bool fRecursiveTraceback=false)
    {
        return TraceBack(Populate(gap_open, gap_extn), fRecursiveTraceback);
    }

    /// <summary>
    /// Used when aligning Query
    /// This calls Popoluate followed by Traceback
    /// </summary>
    ///
    /// <param name="gap_open">static cost to open gap<param>
    /// <param name="gap_extn">static cost to extend gap<param>
    Element Populate(int gap_open, int gap_extn) 
    {
        //Also Refer http://www.eecg.toronto.edu/~tsa/papers/TaAb97.pdf
        //for the benifits of chunking this up
        const int chunkSize = min(m_subjectLength, m_queryLength) / GetProcessorCount();
        vector<vector<task<void>>> vv;
        vector<task<void>> v;
        combinable<Element> elem;
        auto ChunkTask = [&](const int& thisRow, const int& thisCol)
        {
            if ((thisRow == 0) || (thisCol == 0))
                return;

            //Calc local alignment (match/mismatch) score: H(i-1, j-1) + SimilarityScore(A[i], B[j])
            const int localAlignScore_H = m_ppScoreMatrix[thisRow-1][thisCol-1].m_maxScore + m_pMatrix->GetScore(m_pSubject[thisRow-1], m_pQuery[thisCol-1]);

            //Calc deletion score
            m_ppScoreMatrix[thisRow][thisCol].m_localAlignScore_E = max((m_ppScoreMatrix[thisRow-1][thisCol].m_localAlignScore_E - gap_extn), (m_ppScoreMatrix[thisRow-1][thisCol].m_maxScore - gap_open));

            //Calc insertion score
            m_ppScoreMatrix[thisRow][thisCol].m_localAlignScore_F = max((m_ppScoreMatrix[thisRow][thisCol-1].m_localAlignScore_F - gap_extn), (m_ppScoreMatrix[thisRow][thisCol-1].m_maxScore - gap_open));

            //Calculate matrix value h(i,j) = max{(h(i-1, j-1) + Z[A[i], B[j]]), e(i,j), f(i,j), 0}
            m_ppScoreMatrix[thisRow][thisCol].m_maxScore = MAX4(localAlignScore_H, m_ppScoreMatrix[thisRow][thisCol].m_localAlignScore_E, m_ppScoreMatrix[thisRow][thisCol].m_localAlignScore_F, 0);

            // Determine the traceback direction
            ( m_ppScoreMatrix[thisRow][thisCol].m_maxScore == localAlignScore_H) ? m_ppScoreMatrix[thisRow][thisCol].m_direction += DIAG : __noop;
            ( m_ppScoreMatrix[thisRow][thisCol].m_maxScore == m_ppScoreMatrix[thisRow][thisCol].m_localAlignScore_E) ? m_ppScoreMatrix[thisRow][thisCol].m_direction += TOP: __noop;
            ( m_ppScoreMatrix[thisRow][thisCol].m_maxScore == m_ppScoreMatrix[thisRow][thisCol].m_localAlignScore_F) ? m_ppScoreMatrix[thisRow][thisCol].m_direction += LEFT : __noop;

            // Set the traceback start at the current elem i, j and score
            //Using a combinable here to avoid taking a lock (comparisons happens with the local copy)
            if (m_ppScoreMatrix[thisRow][thisCol].m_maxScore > elem.local().GetScore())             
                elem.local().Set(thisRow, thisCol, m_ppScoreMatrix[thisRow][thisCol].m_maxScore);
        };

        for (int x = 0; x < m_subjectLength; x += chunkSize)         
        {
            v.clear();
            for (int y = 0; y < m_queryLength; y += chunkSize)
            {
                if ((x == 0) && (y == 0))
                {
                    //This is (0,0) and has no dependency
                    //hence just fire off the task that would work the corrosponding chunks
                    v.push_back(task<void>([&,x,y]() {
                        for(int thisRow0=x; thisRow0 <  x + min(m_subjectLength - x, chunkSize); ++thisRow0)
                            for(int thisCol0=y; thisCol0 <  y + min(m_queryLength - y, chunkSize); ++thisCol0)
                                ChunkTask(thisRow0, thisCol0);                        
                    }));
                }
                else if (x == 0)
                {
                    //This is (0, y) and is dependent on (0, y-1)
                    //hence use continuations off a task
                    v.push_back(v[v.size() - 1].continue_with([&,x,y]() {
                        for(int thisRow0=x; thisRow0 <  x + min(m_subjectLength - x, chunkSize); ++thisRow0)
                            for(int thisCol0=y; thisCol0 <  y + min(m_queryLength - y, chunkSize); ++thisCol0)
                                ChunkTask(thisRow0, thisCol0);
                    }));
                }
                else if (y == 0)
                {
                    //This is (x, 0) and is dependent on (x-1, 0)
                    //hence use continuations off a task
                    v.push_back(vv[vv.size() - 1][0].continue_with([&,x,y]() {
                        for(int thisRow0=x; thisRow0 <  x + min(m_subjectLength - x, chunkSize); ++thisRow0)
                            for(int thisCol0=y; thisCol0 <  y + min(m_queryLength - y, chunkSize); ++thisCol0)
                                ChunkTask(thisRow0, thisCol0);
                    }));
                }
                else
                {
                    //This is (x, y) and is dependent on (x-1, y), (x-1, y-1), (x, y-1) (order consistent with the && stmt below)
                    //hence use continuations of a join (when_all using operator &&)
                    v.push_back((vv[vv.size() - 1][v.size()] && vv[vv.size() - 1][v.size() - 1] && v[v.size() - 1]).continue_with([&,x,y]() {
                        for(int thisRow0=x; thisRow0 <  x + min(m_subjectLength - x, chunkSize); ++thisRow0)
                            for(int thisCol0=y; thisCol0 <  y + min(m_queryLength - y, chunkSize); ++thisCol0)
                                ChunkTask(thisRow0, thisCol0);
                    }));
                }
            }
            vv.push_back(v);
        }
        //Wait for the last task (m_subjectLength-1, m_queryLength-1) to finish
        vv[vv.size() - 1][vv[0].size() - 1].wait();
               
        return elem.combine([](Element& _left, Element& _right) -> Element { return ((_left.GetScore() > _right.GetScore()) ? _left : _right); });
    }

    /// <summary>
    /// Used when Walkback the matrix from the winning node
    /// </summary>
    ///
    /// <param name="elem">Winning element<param>
    /// <param name="fRecursiveTraceback">Trace all paths/mutations<param>
    Result TraceBack(Element elem, const bool fRecursiveTraceback)
    {
        int row = elem.GetRow();
        int col = elem.GetColumn();
        Result result(m_pSubject, m_pQuery, elem.GetScore());
        NodeWalk(result, row, col, fRecursiveTraceback);
        return result;
    }

private:

    /// <summary>
    /// Walk back from the winning node based on 
    /// direction set during populating matrix
    /// </summary>
    ///
    /// <param name="result">computed Result value<param>
    /// <param name="row">The row from where nodewalk needs to be performed<param>
    /// <param name="col">The col from where nodewalk needs to be performed<param>
    /// <param name="fRecursiveTraceback">Trace all paths/mutations<param>
    /// <param name="fFirstPass">Oneshot effective only when fRecursiveTraceback is true<param>
    /// <param name="alignedSubject">reversed subject string, value determined by the direction<param>
    /// <param name="markup">relation between alignedSubject and alignedQuery chars<param>
    /// <param name="alignedQuery">reversed query string, value determined by the direction<param>
    void NodeWalk(Result& result, int row, int col, const bool fRecursiveTraceback, const bool fFirstPass=true, string alignedSubject="", string markup="", string alignedQuery="")
    {
        unsigned char trace;
        bool fTerminate=false;
        if (fFirstPass)
        {
            alignedSubject.push_back(m_pSubject[row]);
            alignedQuery.push_back(m_pQuery[col]);

            if (m_pSubject[row] == m_pQuery[col])
            {
                markup.push_back(IDENTITY);
                result.m_identity++;
                result.m_similarity++;
            }
            else if (m_pMatrix->GetScore(m_pSubject[row], m_pQuery[col]) > 0)
            {
                markup.push_back(SIMILARITY);
                result.m_similarity++;
            }
            else
            {
                markup.push_back(MISMATCH);
            }
        }

        while(!fTerminate)
        {
            if((m_ppScoreMatrix[row][col].m_direction & DIAG) != 0)
            {
                trace = DIAG;
                alignedSubject.push_back(m_pSubject[row-1]);
                alignedQuery.push_back(m_pQuery[col-1]);
                if (m_pSubject[row-1] == m_pQuery[col-1])
                {
                    markup.push_back(IDENTITY);
                    result.m_identity++;
                    result.m_similarity++;
                }
                else if (m_pMatrix->GetScore(m_pSubject[row-1], m_pQuery[col-1]) > 0)
                {
                    markup.push_back(SIMILARITY);
                    result.m_similarity++;
                }
                else
                {
                    markup.push_back(MISMATCH);
                }

                if (m_ppScoreMatrix[row][col].m_direction != trace)
                {
                    //More than 1 path exists
                    m_ppScoreMatrix[row][col].m_direction -= DIAG;
                    (fRecursiveTraceback) ? NodeWalk(result, row, col, true, false, alignedSubject, markup, alignedQuery) : __noop;
                }
                --row;
                --col;
                continue;
            }

            if((m_ppScoreMatrix[row][col].m_direction & TOP) != 0)
            {
                trace = TOP;
                alignedSubject.push_back(m_pSubject[row-1]);
                alignedQuery.push_back('-');
                markup.push_back(GAP);
                result.m_gaps++;
                if (m_ppScoreMatrix[row][col].m_direction != trace)
                {
                    //More than 1 path exists
                    m_ppScoreMatrix[row][col].m_direction -= TOP;
                    (fRecursiveTraceback) ? NodeWalk(result, row, col, true, false, alignedSubject, markup, alignedQuery) : __noop;
                }
                --row;
                continue;
            }

            if((m_ppScoreMatrix[row][col].m_direction & LEFT) != 0)
            {
                trace = LEFT;
                alignedSubject.push_back('-');
                alignedQuery.push_back(m_pQuery[col-1]);
                markup.push_back(GAP);
                result.m_gaps++;
                if (m_ppScoreMatrix[row][col].m_direction != trace)
                {
                    //More than 1 path exists
                    m_ppScoreMatrix[row][col].m_direction -= LEFT;
                    (fRecursiveTraceback) ? NodeWalk(result, row, col, true, false, alignedSubject, markup, alignedQuery) : __noop;
                }
                --col;
                continue;
            }

            if(m_ppScoreMatrix[row][col].m_direction == 0)
            {
                //Terminating condition
                result.m_alignments.push_back(Alignment(alignedSubject, markup, alignedQuery));
                fTerminate=true;
            }
            else
            {
                throw;
            }
        }
    }

private:
    char* m_pSubject;
    char* m_pQuery;
    const unsigned int m_subjectLength;
    unsigned int m_queryLength;
    volatile Cell** m_ppScoreMatrix;
    unsigned int m_maxQueryLength;
    ScoringMatrix* m_pMatrix;
};

#pragma warning(default:4267)
#pragma warning(default:4018)

