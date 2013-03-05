//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: FileReader.h
//
//--------------------------------------------------------------------------
#pragma once
#include <agents.h>
#include <fstream>
#include <iostream>
#include "Payload.h"
#include <string>

using namespace Concurrency;
using namespace std;

//
// This agent receives file names from an unbounded_buffer and searches each file
// for a specified string. Each found line is sent to an output buffer.
//
class FileReader : public agent
{
public:
    FileReader(unbounded_buffer<wchar_t *> *pFileNames, const wchar_t *pSearchString, unbounded_buffer<Payload *> *pFoundBuffer)
        : m_pFileNames(pFileNames), m_pSearchString(pSearchString), m_pFoundBuffer(pFoundBuffer) {}
protected:
    void run()
    {
        wchar_t *pFileName;

        //
        // Repeatedly pull filename messages out of the unbounded_buffer 
        // to parse until the END_MSG is reached. Note this will ConcRT 
        // aware block until a message is avaliable in the buffer.
        //
        size_t currentLineNum;
        const size_t lineSize = 2000;
        wchar_t line[lineSize];
        while((int)(pFileName = receive(m_pFileNames)) != END_MSG)
        {
            currentLineNum = 1;
            wifstream inputFile;
            inputFile.open(pFileName, ifstream::in);
            while(inputFile.good() && inputFile.getline(line, lineSize))
            {
                if(wcsstr(line, m_pSearchString) != NULL)
                {
                    //
                    // Create a new message payload and send it to the 
                    // unbounded_buffer for the ConsoleWriter to receive 
                    // from.
                    //
                    asend(m_pFoundBuffer, new Payload(pFileName, wcsnlen(pFileName, MAX_FILE_NAME)+1, currentLineNum, line, wcsnlen(line, lineSize)+1));
                }

                ++currentLineNum;
                line[0] = '\0';
            }
            inputFile.close();
            delete pFileName;
        }

        // Resend the END_MSG for any other FileReaders.
        asend(m_pFileNames, (wchar_t *)END_MSG);
        done();
    }

private:
    unbounded_buffer<wchar_t *> *m_pFileNames;
    const wchar_t *m_pSearchString;
    unbounded_buffer<Payload *> *m_pFoundBuffer;
};