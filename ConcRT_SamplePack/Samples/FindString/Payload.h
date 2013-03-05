//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: Payload.h
//
//--------------------------------------------------------------------------
#pragma once

// Used to signal final message and processing can stop.
#define END_MSG -1

// Max size used for any file name.
#define MAX_FILE_NAME MAX_PATH

class Payload
{
public:
    Payload(const wchar_t *pFileName, const size_t fileNameSize, const size_t lineNumber, const wchar_t *pLine, const size_t lineSize)
        : m_lineNumber(lineNumber)
    {
        m_pLine = new wchar_t[lineSize];
        wcscpy_s(m_pLine, lineSize, pLine);
        m_pFileName = new wchar_t[fileNameSize];
        wcscpy_s(m_pFileName, fileNameSize, pFileName);
    }
    ~Payload()
    {
        delete []m_pLine;
        delete []m_pFileName;
    }
    wchar_t *m_pFileName;
    const size_t m_lineNumber;
    wchar_t *m_pLine;
private:
    Payload & operator=(const Payload &);
};