// ==++== 
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// File: Result.h
//
// Contains declaration for Result class.
//
// ==--==

#pragma once

#include "stdafx.h"
#include "Alignment.h"

/// <summary>
/// Result structure which contains the given subject and query, and alignments and their attributes
/// </summary>
struct Result
{
    Result()
        :m_score(0), m_identity(0), m_similarity(0), m_gaps(0)
    {
    }
    Result(string subject, string query, int score)
        :m_subject(subject), m_query(query), m_score(score), m_identity(0), m_similarity(0), m_gaps(0)
    {
    }

    string m_subject;
    string m_query;
    vector<Alignment> m_alignments;
    int m_score;

    int m_identity;
    int m_similarity;
    int m_gaps;
};