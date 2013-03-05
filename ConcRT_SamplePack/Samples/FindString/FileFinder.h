//--------------------------------------------------------------------------
// 
//  Copyright (c) Microsoft Corporation.  All rights reserved. 
// 
//  File: FileFinder.h
//
//--------------------------------------------------------------------------
#pragma once
#include <agents.h>

using namespace Concurrency;

//
// This agent searches a directory for given file types. Sends a message
// to an unbounded_buffer for each file found.
//
class FileFinder : public agent
{
public:
    FileFinder(const wchar_t *pFileTypes, unbounded_buffer<wchar_t *> *pFileNames)
        : m_pFileTypes(pFileTypes), m_pFileNames(pFileNames), m_sizeFileTypes(wcslen(pFileTypes)) {}
protected:
    void run()
    {
        wchar_t currentDirectory[MAX_FILE_NAME] = L".\\";
        FindFilesRecursively(currentDirectory);

        // Send END_MSG to signal done processing.
        asend(m_pFileNames, (wchar_t *)END_MSG);
        done();
    }
private:

    //
    // Looks in specified directory for matching files.
    //
    void FindFiles(wchar_t *pDirectory)
    {
        WIN32_FIND_DATA findFileData;
        const size_t directoryNameSize = wcslen(pDirectory);
        if(pDirectory[directoryNameSize - 1] != '\\')
        {
            wcscat_s(pDirectory, MAX_FILE_NAME, L"\\");
        }
        wcscat_s(pDirectory, MAX_FILE_NAME, m_pFileTypes);
        HANDLE hFind = FindFirstFile(pDirectory, &findFileData);
        pDirectory[wcslen(pDirectory) - m_sizeFileTypes] = '\0';
        if(hFind != INVALID_HANDLE_VALUE)
        {
            BOOL moreFiles = true;
            wchar_t *pFileName;
            while(moreFiles)
            {
                // Copy file name, receiver will be responsible for cleanup.
                pFileName = new wchar_t[MAX_FILE_NAME];
                wcscpy_s(pFileName, MAX_FILE_NAME, pDirectory);
                wcscat_s(pFileName, MAX_FILE_NAME, findFileData.cFileName);

                //
                // Send the filename to the unbounded_buffer.
                //
                asend(m_pFileNames, pFileName);
                moreFiles = FindNextFile(hFind, &findFileData);
            }
            FindClose(hFind);
        }
    }

    //
    // Recursively searches each directory for matching files.
    //
    void FindFilesRecursively(wchar_t *pDirectory)
    {
        // Look for files in this directory matching first.
        FindFiles(pDirectory);

        // Now recursively look in all directories.
        WIN32_FIND_DATA findFileData;
        wcscat_s(pDirectory, MAX_FILE_NAME, L"*");
        HANDLE hFind = FindFirstFile(pDirectory, &findFileData);
        pDirectory[wcslen(pDirectory) - 1] = '\0';
        if(hFind != INVALID_HANDLE_VALUE)
        {
            BOOL moreFiles = true;
            while(moreFiles)
            {
                // If directory recursively search it.
                if(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY 
                    && wcscmp(findFileData.cFileName, L".") && wcscmp(findFileData.cFileName, L".."))
                {
                    wcscat_s(pDirectory, MAX_FILE_NAME, findFileData.cFileName);
                    FindFilesRecursively(pDirectory);
                    pDirectory[wcslen(pDirectory) - wcslen(findFileData.cFileName) - 1] = '\0';
                }
                moreFiles = FindNextFile(hFind, &findFileData);
            }
            FindClose(hFind);
        }
    }

    const wchar_t *m_pFileTypes;
    const size_t m_sizeFileTypes;
    unbounded_buffer<wchar_t *> *m_pFileNames;
};