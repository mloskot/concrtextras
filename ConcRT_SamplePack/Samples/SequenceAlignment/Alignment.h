// ==++== 
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// File: Alignment.h
//
// Contains declaration for Alignment class.
//
// ==--==

#pragma once

#include "stdafx.h"

using namespace std;

/// <summary>
/// Aligned subject and query with markup
/// </summary>
class Alignment
{
public:
    Alignment(string alignedSubject, string markup, string alignedQuery)
        :m_alignedSubject(alignedSubject.rbegin(), alignedSubject.rend()),
        m_markup(markup.rbegin(), markup.rend()),
        m_alignedQuery(alignedQuery.rbegin(), alignedQuery.rend())
    {
    }

    const string& GetSubject() const
    {
        return m_alignedSubject;
    }

    const string& GetQuery() const
    {
        return m_alignedQuery;
    }

    const string& GetMarkup() const
    {
        return m_markup;
    }

private:
    string m_alignedSubject;
    string m_markup;
    string m_alignedQuery;
};
