#include "stdafx.h"

#define BUFSIZE 1024
int DownloadUrl(LPTSTR szUrl)
{
    TCHAR szTempFileName[MAX_PATH];
    TCHAR lpTempPathBuffer[MAX_PATH];
    char  chBuffer[BUFSIZE]; 

    // Generate a temporary file name
    DWORD dwRetVal = GetTempPath(MAX_PATH, lpTempPathBuffer);
    if (dwRetVal > MAX_PATH || (dwRetVal == 0))
    {
        return -1;
    }

    UINT uRetVal = GetTempFileName(lpTempPathBuffer, TEXT("DEMO"), 0, szTempFileName);
    if (uRetVal == 0)
    {
        return -1;
    }

    HRESULT h = URLDownloadToFile(NULL, szUrl, szTempFileName, BINDF_GETNEWESTVERSION, NULL);

    HANDLE hFile = CreateFile(szTempFileName,               // file name 
                       GENERIC_READ,          // open for reading 
                       0,                     // do not share 
                       NULL,                  // default security 
                       OPEN_EXISTING,         // existing file only 
                       FILE_ATTRIBUTE_NORMAL, // normal file 
                       NULL);                 // no template 
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        return -1;
    }

    DWORD dwBytesRead = 0;
    DWORD dwBytesReadTotal = 0;

    do
    {
        if (!ReadFile(hFile, chBuffer, BUFSIZE, &dwBytesRead, NULL)) 
        {
            return -1;
        }
        dwBytesReadTotal += dwBytesRead;
    }
    while (dwBytesRead == BUFSIZE); // Continues until the whole file is processed

    DeleteFile(szTempFileName);

    return dwBytesReadTotal;
}

