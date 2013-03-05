//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: ConsoleWriter.h
//
//--------------------------------------------------------------------------
#pragma once
#include <agents.h>
#include "Payload.h"

using namespace Concurrency;

//
// Agents receives messages on an unbounded_buffer and prints information
// to the console.
//
class ConsoleWriter : public agent
{
public:
    ConsoleWriter(unbounded_buffer<Payload *> *pFoundBuffer)
        : m_pFoundBuffer(pFoundBuffer) {}

protected:
    void run()
    {
        Payload *pPayload;
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO consoleScreenBufferInfo;
        GetConsoleScreenBufferInfo(hConsole, &consoleScreenBufferInfo);
        WORD originalAttributes = consoleScreenBufferInfo.wAttributes;

        //
        // Keep receiving messages to print to the console until
        // the END_MSG is received. Note this will ConcRT aware
        // block until a message is avaliable in the buffer.
        //
        while((int)(pPayload = receive(m_pFoundBuffer)) != END_MSG)
        {
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY); 
            printf("%ls:%lu:", pPayload->m_pFileName, (unsigned long)pPayload->m_lineNumber);
            SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
            printf(":%ls\n", pPayload->m_pLine);
            delete pPayload;
        }
        SetConsoleTextAttribute(hConsole, originalAttributes);
        done();
    }
private:
    unbounded_buffer<Payload *> *m_pFoundBuffer;
};