//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: FindString.cpp
//
//--------------------------------------------------------------------------
#define UNICODE
#include <windows.h>
#include "Payload.h"
#include "FileFinder.h"
#include "FileReader.h"
#include "ConsoleWriter.h"

using namespace Concurrency;

int wmain(int argc, wchar_t* argv[])
{
    if(argc != 3)
    {
        printf("Usage: FindString.exe StringToSearchFor FilesToSearch(*.txt)\n");
        return 1;
    }
    wchar_t *pFilePattern = argv[2];
    wchar_t *pStringPattern = argv[1];

    // Start time measurement.
    LARGE_INTEGER freq = {0};
    LARGE_INTEGER start = {0}, end = {0};
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    unbounded_buffer<wchar_t *> fileBuffer;
    unbounded_buffer<Payload *> foundBuffer;

    // Agent to find files.
    FileFinder fileFinder(pFilePattern, &fileBuffer);
    fileFinder.start();

    // Agents to handle parsing files.
    unsigned int numAgents = CurrentScheduler::Get()->GetNumberOfVirtualProcessors();
    // On UMS we can oversubscribe to possibly get some benefit due to I/O.
    if(CurrentScheduler::Get()->GetPolicy().GetPolicyValue(SchedulerKind) == UmsThreadDefault)
    {
        numAgents = CurrentScheduler::Get()->GetNumberOfVirtualProcessors() * 2;
    }
    FileReader **ppFileReaderAgents = new FileReader*[numAgents];
    for(unsigned int i = 0; i < numAgents; ++i)
    {
        ppFileReaderAgents[i] = new FileReader(&fileBuffer, pStringPattern, &foundBuffer);
        ppFileReaderAgents[i]->start();
    }

    // Agent to handle writing to the console.
    ConsoleWriter consoleWriter(&foundBuffer);
    consoleWriter.start();

    // Wait for agents to finish.
    agent::wait(&fileFinder);
    for(unsigned int i = 0; i < numAgents; ++i)
    {
        agent::wait(ppFileReaderAgents[i]);
        delete ppFileReaderAgents[i];
    }
    delete [] ppFileReaderAgents;

    // Now that all other agents have finished signal the ConsoleWriter
    // it can finish.
    send(foundBuffer, (Payload *)END_MSG);
    agent::wait(&consoleWriter);

    // End time measurement and print results.
    QueryPerformanceCounter(&end);
    LONGLONG divideMs = freq.QuadPart / (LONGLONG)1000;
    LONGLONG total = (end.QuadPart - start.QuadPart) / divideMs;
    printf("Total execution time:%i", total);

    return 0;
}